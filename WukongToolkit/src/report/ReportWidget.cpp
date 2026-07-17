#include "report/ReportWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTextEdit>
#include <QDateEdit>
#include <QTimer>
#include <QCheckBox>
#include <QPainter>
#include <QSplitter>
#include <QLabel>
#include <QDateTime>
#include <QFile>
#include <QRandomGenerator>
#include <QApplication>

// ============================================================================
// 内联图表绘制控件 — 用于 m_chartWidget 的 paintEvent
// ============================================================================
class ChartPainterWidget : public QWidget
{
public:
    explicit ChartPainterWidget(QWidget* parent = nullptr)
        : QWidget(parent) {}

    void setDrawCallback(std::function<void(QPainter&, const QRect&)> cb)
    {
        m_callback = std::move(cb);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (m_callback) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            m_callback(painter, rect());
        }
    }

private:
    std::function<void(QPainter&, const QRect&)> m_callback;
};

// ============================================================================
// 构造
// ============================================================================
ReportWidget::ReportWidget(QWidget* parent)
    : QWidget(parent)
    , m_typeCombo(nullptr)
    , m_deviceCombo(nullptr)
    , m_timeRangeCombo(nullptr)
    , m_startDateEdit(nullptr)
    , m_endDateEdit(nullptr)
    , m_generateBtn(nullptr)
    , m_previewEdit(nullptr)
    , m_exportFormatCombo(nullptr)
    , m_exportBtn(nullptr)
    , m_autoReportCheck(nullptr)
    , m_autoTypeCombo(nullptr)
    , m_historyTable(nullptr)
    , m_chartWidget(nullptr)
    , m_autoTimer(nullptr)
{
    setupUI();
    setupConnections();

    Logger::instance().info(QStringLiteral("ReportCenter"),
        QStringLiteral("报表中心初始化完成"));
}

// ============================================================================
// UI 构建
// ============================================================================
void ReportWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 通用样式 lambda ──
    auto styleCombo = [](QComboBox* combo, int minWidth = 160) {
        combo->setMinimumWidth(minWidth);
        combo->setStyleSheet(QStringLiteral(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        ));
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString(QStringLiteral(
                "QPushButton {"
                "  background-color: %1; color: white;"
                "  border: none; padding: 8px 20px; border-radius: 4px;"
                "  font-size: 13px; font-weight: bold;"
                "}"
                "QPushButton:hover { background-color: %2; }"
                "QPushButton:disabled { background-color: #5C5C5C; }"
            )).arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(QStringLiteral(
            "QTableWidget {"
            "  background-color: #1E1F22; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; font-size: 12px;"
            "  gridline-color: #2C2D30;"
            "}"
            "QTableWidget::item { padding: 3px 6px; }"
            "QTableWidget::item:selected { background-color: #3C3F41; }"
            "QHeaderView::section {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  border: none; border-bottom: 2px solid #3C3F41;"
            "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
            "}"
        ));
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
    };

    auto styleDateEdit = [](QDateEdit* edit) {
        edit->setCalendarPopup(true);
        edit->setStyleSheet(QStringLiteral(
            "QDateEdit {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QDateEdit::drop-down { border: none; }"
            "QDateEdit QCalendarWidget {"
            "  background-color: #25262B; color: #DCDCDC;"
            "}"
        ));
    };

    auto styleGroupBox = [](QGroupBox* group) {
        group->setStyleSheet(QStringLiteral(
            "QGroupBox {"
            "  color: #409EFF; font-size: 13px; font-weight: bold;"
            "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
            "  padding-top: 16px;"
            "}"
            "QGroupBox::title {"
            "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
            "}"
        ));
    };

    // ────────────────────────────────────────────────────────────────────
    // 顶部配置区
    // ────────────────────────────────────────────────────────────────────
    auto* configGroup = new QGroupBox(QStringLiteral("报表配置"));
    styleGroupBox(configGroup);
    auto* configLayout = new QHBoxLayout(configGroup);
    configLayout->setSpacing(12);

    // 报表类型
    configLayout->addWidget(new QLabel(QStringLiteral("报表类型:")));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({
        QStringLiteral("巡检报告"),
        QStringLiteral("告警报告"),
        QStringLiteral("性能报告"),
        QStringLiteral("设备资产报告"),
        QStringLiteral("配置变更报告"),
        QStringLiteral("网络健康报告"),
        QStringLiteral("日报"),
        QStringLiteral("周报"),
        QStringLiteral("月报")
    });
    styleCombo(m_typeCombo, 140);
    configLayout->addWidget(m_typeCombo);

    // 设备选择
    configLayout->addWidget(new QLabel(QStringLiteral("设备:")));
    m_deviceCombo = new QComboBox();
    m_deviceCombo->addItems({
        QStringLiteral("全部设备"),
        QStringLiteral("Core-Switch-01"),
        QStringLiteral("Core-Router-01"),
        QStringLiteral("FW-Cluster-01"),
        QStringLiteral("Agg-Switch-03"),
        QStringLiteral("AP-Controller-01")
    });
    styleCombo(m_deviceCombo, 150);
    configLayout->addWidget(m_deviceCombo);

    // 时间范围
    configLayout->addWidget(new QLabel(QStringLiteral("时间范围:")));
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->addItems({
        QStringLiteral("最近24小时"),
        QStringLiteral("最近7天"),
        QStringLiteral("最近30天"),
        QStringLiteral("本月"),
        QStringLiteral("上月"),
        QStringLiteral("自定义")
    });
    styleCombo(m_timeRangeCombo, 130);
    configLayout->addWidget(m_timeRangeCombo);

    // 自定义日期
    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-7));
    m_startDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    styleDateEdit(m_startDateEdit);
    m_startDateEdit->setVisible(false);
    configLayout->addWidget(new QLabel(QStringLiteral("从:")));
    configLayout->addWidget(m_startDateEdit);

    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    styleDateEdit(m_endDateEdit);
    m_endDateEdit->setVisible(false);
    configLayout->addWidget(new QLabel(QStringLiteral("到:")));
    configLayout->addWidget(m_endDateEdit);

    // 生成按钮
    m_generateBtn = new QPushButton(QStringLiteral("生成报表"));
    styleButton(m_generateBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));
    configLayout->addWidget(m_generateBtn);

    configLayout->addStretch();
    mainLayout->addWidget(configGroup);

    // ────────────────────────────────────────────────────────────────────
    // 中部：预览区 + 统计图表 (Splitter)
    // ────────────────────────────────────────────────────────────────────
    auto* middleSplitter = new QSplitter(Qt::Horizontal, this);
    middleSplitter->setStyleSheet(QStringLiteral(
        "QSplitter::handle { background-color: #3C3F41; width: 2px; }"
    ));

    // 预览区
    auto* previewGroup = new QGroupBox(QStringLiteral("报表预览"));
    styleGroupBox(previewGroup);
    auto* previewLayout = new QVBoxLayout(previewGroup);

    m_previewEdit = new QTextEdit();
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setPlaceholderText(QStringLiteral("\u70B9\u51FB\u300C\u751F\u6210\u62A5\u8868\u300D\u6309\u94AE\u67E5\u770B\u62A5\u8868\u5185\u5BB9..."));
    m_previewEdit->setStyleSheet(QStringLiteral(
        "QTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 13px;"
        "  padding: 8px;"
        "}"
    ));
    previewLayout->addWidget(m_previewEdit);

    middleSplitter->addWidget(previewGroup);

    // 统计图表区
    auto* chartGroup = new QGroupBox(QStringLiteral("统计图表"));
    styleGroupBox(chartGroup);
    auto* chartLayout = new QVBoxLayout(chartGroup);

    auto* chartPainter = new ChartPainterWidget();
    chartPainter->setMinimumSize(280, 200);
    chartPainter->setStyleSheet(QStringLiteral(
        "background-color: #1E1F22; border: 1px solid #3C3F41; border-radius: 4px;"
    ));
    chartPainter->setDrawCallback([this](QPainter& painter, const QRect& r) {
        drawChart(painter, r);
    });
    m_chartWidget = chartPainter;
    chartLayout->addWidget(m_chartWidget);

    middleSplitter->addWidget(chartGroup);
    middleSplitter->setStretchFactor(0, 3);
    middleSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(middleSplitter, 1);

    // ────────────────────────────────────────────────────────────────────
    // 底部导出控制区
    // ────────────────────────────────────────────────────────────────────
    auto* exportGroup = new QGroupBox(QStringLiteral("导出与定时"));
    styleGroupBox(exportGroup);
    auto* exportLayout = new QHBoxLayout(exportGroup);
    exportLayout->setSpacing(12);

    exportLayout->addWidget(new QLabel(QStringLiteral("导出格式:")));
    m_exportFormatCombo = new QComboBox();
    m_exportFormatCombo->addItems({
        QStringLiteral("HTML"),
        QStringLiteral("Markdown"),
        QStringLiteral("CSV"),
        QStringLiteral("PDF")
    });
    styleCombo(m_exportFormatCombo, 120);
    exportLayout->addWidget(m_exportFormatCombo);

    m_exportBtn = new QPushButton(QStringLiteral("导出报表"));
    styleButton(m_exportBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));
    exportLayout->addWidget(m_exportBtn);

    exportLayout->addSpacing(24);

    m_autoReportCheck = new QCheckBox(QStringLiteral("定时报表"));
    m_autoReportCheck->setStyleSheet(QStringLiteral(
        "QCheckBox { color: #DCDCDC; font-size: 13px; }"
        "QCheckBox::indicator {"
        "  width: 16px; height: 16px;"
        "  border: 1px solid #3C3F41; border-radius: 3px;"
        "  background-color: #25262B;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #409EFF; border-color: #409EFF;"
        "}"
    ));
    exportLayout->addWidget(m_autoReportCheck);

    m_autoTypeCombo = new QComboBox();
    m_autoTypeCombo->addItems({
        QStringLiteral("日报"),
        QStringLiteral("周报"),
        QStringLiteral("月报")
    });
    styleCombo(m_autoTypeCombo, 100);
    m_autoTypeCombo->setEnabled(false);
    exportLayout->addWidget(m_autoTypeCombo);

    exportLayout->addStretch();
    mainLayout->addWidget(exportGroup);

    // ────────────────────────────────────────────────────────────────────
    // 报表历史
    // ────────────────────────────────────────────────────────────────────
    auto* historyGroup = new QGroupBox(QStringLiteral("报表历史"));
    styleGroupBox(historyGroup);
    auto* historyLayout = new QVBoxLayout(historyGroup);

    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("类型"),
        QStringLiteral("设备"),
        QStringLiteral("格式"),
        QStringLiteral("文件名")
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_historyTable->verticalHeader()->setVisible(false);
    styleTable(m_historyTable);

    historyLayout->addWidget(m_historyTable);
    mainLayout->addWidget(historyGroup, 1);
}

// ============================================================================
// 信号槽连接
// ============================================================================
void ReportWidget::setupConnections()
{
    connect(m_generateBtn, &QPushButton::clicked,
            this, &ReportWidget::onGenerateReport);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &ReportWidget::onExport);
    connect(m_autoReportCheck, &QCheckBox::toggled,
            this, &ReportWidget::onToggleAutoReport);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportWidget::onTypeChanged);
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportWidget::onTimeRangeChanged);
}

// ============================================================================
// 报表类型变更
// ============================================================================
void ReportWidget::onTypeChanged(int index)
{
    Q_UNUSED(index)
    // 根据报表类型调整某些选项（如设备选择是否可见）
    Logger::instance().debug(QStringLiteral("ReportCenter"),
        QString(QStringLiteral("报表类型切换: %1")).arg(m_typeCombo->currentText()));
}

// ============================================================================
// 时间范围变更 — 控制自定义日期控件可见性
// ============================================================================
void ReportWidget::onTimeRangeChanged(int index)
{
    bool isCustom = (index == 5); // "自定义" = index 5
    m_startDateEdit->setVisible(isCustom);
    m_endDateEdit->setVisible(isCustom);
}

// ============================================================================
// 生成报表
// ============================================================================
void ReportWidget::onGenerateReport()
{
    QString reportType = m_typeCombo->currentText();
    QString body;

    if (reportType == QStringLiteral("巡检报告")) {
        body = generateInspectionReport();
        m_currentReportTitle = QStringLiteral("网络巡检报告");
    } else if (reportType == QStringLiteral("告警报告")) {
        body = generateAlarmReport();
        m_currentReportTitle = QStringLiteral("告警分析报告");
    } else if (reportType == QStringLiteral("性能报告")) {
        body = generatePerformanceReport();
        m_currentReportTitle = QStringLiteral("性能监控报告");
    } else if (reportType == QStringLiteral("设备资产报告")) {
        body = generateAssetReport();
        m_currentReportTitle = QStringLiteral("设备资产报告");
    } else if (reportType == QStringLiteral("配置变更报告")) {
        body = generateConfigChangeReport();
        m_currentReportTitle = QStringLiteral("配置变更报告");
    } else if (reportType == QStringLiteral("网络健康报告")) {
        body = generateHealthReport();
        m_currentReportTitle = QStringLiteral("网络健康报告");
    } else if (reportType == QStringLiteral("日报")) {
        body = generateDailyReport();
        m_currentReportTitle = QStringLiteral("网络运维日报");
    } else if (reportType == QStringLiteral("周报")) {
        body = generateWeeklyReport();
        m_currentReportTitle = QStringLiteral("网络运维周报");
    } else if (reportType == QStringLiteral("月报")) {
        body = generateMonthlyReport();
        m_currentReportTitle = QStringLiteral("网络运维月报");
    }

    m_currentReportBody = body;
    QString html = generateHtmlTemplate(m_currentReportTitle, body);
    m_previewEdit->setHtml(html);

    // 刷新图表
    m_chartWidget->update();

    Logger::instance().info(QStringLiteral("ReportCenter"),
        QString(QStringLiteral("报表已生成: %1")).arg(m_currentReportTitle));
}

// ============================================================================
// 导出报表
// ============================================================================
void ReportWidget::onExport()
{
    if (m_currentReportBody.isEmpty()) {
        QMessageBox::information(this,
            QStringLiteral("提示"),
            QStringLiteral("请先生成报表。"));
        return;
    }

    QString format = m_exportFormatCombo->currentText();
    QString html = generateHtmlTemplate(m_currentReportTitle, m_currentReportBody);

    if (format == QStringLiteral("HTML")) {
        exportToHtml(html);
    } else if (format == QStringLiteral("Markdown")) {
        exportToMarkdown(m_currentReportBody);
    } else if (format == QStringLiteral("CSV")) {
        exportToCsv(m_currentReportBody);
    } else if (format == QStringLiteral("PDF")) {
        QMessageBox::information(this,
            QStringLiteral("提示"),
            QStringLiteral("PDF 导出需要安装额外依赖，当前暂不支持。\n请使用 HTML 格式导出后在浏览器中打印为 PDF。"));
    }
}

// ============================================================================
// 定时报表开关
// ============================================================================
void ReportWidget::onToggleAutoReport(bool enabled)
{
    m_autoTypeCombo->setEnabled(enabled);

    if (enabled) {
        if (!m_autoTimer) {
            m_autoTimer = new QTimer(this);
            connect(m_autoTimer, &QTimer::timeout, this, &ReportWidget::onGenerateReport);
        }

        // 根据类型设置定时器间隔（模拟：实际应按 cron 调度）
        int intervalMs = 60000; // 默认1分钟用于演示
        QString autoType = m_autoTypeCombo->currentText();
        if (autoType == QStringLiteral("日报")) {
            intervalMs = 60000; // 演示用1分钟
        } else if (autoType == QStringLiteral("周报")) {
            intervalMs = 120000; // 演示用2分钟
        } else if (autoType == QStringLiteral("月报")) {
            intervalMs = 300000; // 演示用5分钟
        }

        m_autoTimer->start(intervalMs);

        Logger::instance().info(QStringLiteral("ReportCenter"),
            QString(QStringLiteral("定时报表已启动，类型: %1")).arg(autoType));
    } else {
        if (m_autoTimer) {
            m_autoTimer->stop();
        }

        Logger::instance().info(QStringLiteral("ReportCenter"),
            QStringLiteral("定时报表已停止"));
    }
}

// ============================================================================
// 报表生成方法 — 各类型报表（模拟数据）
// ============================================================================

QString ReportWidget::generateInspectionReport()
{
    QString device = m_deviceCombo->currentText();
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    auto* rng = QRandomGenerator::global();
    int totalChecks = 50;
    int passed = totalChecks - rng->bounded(0, 5);
    double passRate = (double)passed / totalChecks * 100.0;
    int score = 85 + rng->bounded(0, 15);

    QString body;
    body += QStringLiteral("<h2>设备巡检结果</h2>");
    body += QString(QStringLiteral("<p>巡检设备: %1 | 巡检时间: %2</p>")).arg(device, timeStr);

    body += QStringLiteral("<table>"
        "<tr><th>序号</th><th>设备名称</th><th>IP 地址</th><th>检查项</th><th>状态</th><th>备注</th></tr>");

    struct InspItem {
        QString name;
        QString ip;
        QString checkItem;
        bool ok;
        QString remark;
    };

    QList<InspItem> items = {
        {QStringLiteral("Core-Switch-01"), QStringLiteral("10.0.1.1"), QStringLiteral("CPU 使用率"), true, QStringLiteral("23%")},
        {QStringLiteral("Core-Switch-01"), QStringLiteral("10.0.1.1"), QStringLiteral("内存使用率"), true, QStringLiteral("45%")},
        {QStringLiteral("Core-Switch-01"), QStringLiteral("10.0.1.1"), QStringLiteral("端口状态"), true, QStringLiteral("全部 Up")},
        {QStringLiteral("Core-Switch-01"), QStringLiteral("10.0.1.1"), QStringLiteral("生成树状态"), true, QStringLiteral("正常")},
        {QStringLiteral("Core-Router-01"), QStringLiteral("10.0.0.1"), QStringLiteral("CPU 使用率"), true, QStringLiteral("35%")},
        {QStringLiteral("Core-Router-01"), QStringLiteral("10.0.0.1"), QStringLiteral("OSPF 邻居"), true, QStringLiteral("Full")},
        {QStringLiteral("Core-Router-01"), QStringLiteral("10.0.0.1"), QStringLiteral("BGP 会话"), true, QStringLiteral("Established")},
        {QStringLiteral("FW-Cluster-01"), QStringLiteral("10.0.0.254"), QStringLiteral("HA 状态"), true, QStringLiteral("Active/Standby")},
        {QStringLiteral("FW-Cluster-01"), QStringLiteral("10.0.0.254"), QStringLiteral("会话数"), false, QStringLiteral("超阈值 85%")},
        {QStringLiteral("Agg-Switch-03"), QStringLiteral("10.0.2.3"), QStringLiteral("链路聚合"), true, QStringLiteral("正常")},
        {QStringLiteral("Agg-Switch-03"), QStringLiteral("10.0.2.3"), QStringLiteral("VLAN 配置"), true, QStringLiteral("一致")},
        {QStringLiteral("AP-Controller-01"), QStringLiteral("10.0.3.1"), QStringLiteral("AP 在线数"), true, QStringLiteral("48/48")},
    };

    int idx = 1;
    for (const auto& item : items) {
        QString statusColor = item.ok ? QStringLiteral("#67C23A") : QStringLiteral("#F56C6C");
        QString statusText = item.ok ? QStringLiteral("✓ 通过") : QStringLiteral("✗ 异常");
        body += QString(QStringLiteral(
            "<tr>"
            "<td>%1</td><td>%2</td><td>%3</td><td>%4</td>"
            "<td style=\"color:%5; font-weight:bold;\">%6</td>"
            "<td>%7</td>"
            "</tr>"))
            .arg(idx++)
            .arg(item.name, item.ip, item.checkItem)
            .arg(statusColor, statusText, item.remark);
    }
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h2>巡检统计</h2>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>数值</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>检查项总数</td><td>%1</td></tr>"
        "<tr><td>通过数量</td><td style=\"color:#67C23A;\">%2</td></tr>"
        "<tr><td>异常数量</td><td style=\"color:#F56C6C;\">%3</td></tr>"
        "<tr><td>通过率</td><td>%4%</td></tr>"
        "<tr><td>综合评分</td><td style=\"color:#409EFF; font-size:18px;\">%5 / 100</td></tr>"))
        .arg(totalChecks).arg(passed).arg(totalChecks - passed)
        .arg(passRate, 0, 'f', 1).arg(score);

    body += QStringLiteral("</table>");

    return body;
}

QString ReportWidget::generateAlarmReport()
{
    QString device = m_deviceCombo->currentText();
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    auto* rng = QRandomGenerator::global();
    int totalAlarms = 128;
    int critical = rng->bounded(5, 15);
    int warning = rng->bounded(20, 40);
    int info = totalAlarms - critical - warning;
    int emergency = rng->bounded(1, 5);

    QString body;
    body += QStringLiteral("<h2>告警统计分析</h2>");
    body += QString(QStringLiteral("<p>统计范围: %1 | 统计时间: %2</p>")).arg(device, timeStr);

    body += QStringLiteral("<h3>告警级别分布</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>级别</th><th>数量</th><th>占比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td style=\"color:#F56C6C;\">Emergency</td><td>%1</td><td>%2%</td></tr>"
        "<tr><td style=\"color:#E6A23C;\">Critical</td><td>%3</td><td>%4%</td></tr>"
        "<tr><td style=\"color:#E6DB5C;\">Warning</td><td>%5</td><td>%6%</td></tr>"
        "<tr><td style=\"color:#409EFF;\">Info</td><td>%7</td><td>%8%</td></tr>"))
        .arg(emergency).arg(emergency * 100.0 / totalAlarms, 0, 'f', 1)
        .arg(critical).arg(critical * 100.0 / totalAlarms, 0, 'f', 1)
        .arg(warning).arg(warning * 100.0 / totalAlarms, 0, 'f', 1)
        .arg(info).arg(info * 100.0 / totalAlarms, 0, 'f', 1);
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>Top 10 告警设备</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>排名</th><th>设备名称</th><th>告警数</th><th>最高级别</th></tr>");

    struct AlarmDevice {
        QString name;
        int count;
        QString topLevel;
    };

    QList<AlarmDevice> topDevices = {
        {QStringLiteral("Core-Switch-01"), 23, QStringLiteral("Emergency")},
        {QStringLiteral("FW-Cluster-01"), 18, QStringLiteral("Critical")},
        {QStringLiteral("Core-Router-01"), 15, QStringLiteral("Critical")},
        {QStringLiteral("Agg-Switch-03"), 12, QStringLiteral("Warning")},
        {QStringLiteral("DC-Power-01"), 10, QStringLiteral("Emergency")},
        {QStringLiteral("Storage-Array-01"), 8, QStringLiteral("Critical")},
        {QStringLiteral("Branch-Router-05"), 7, QStringLiteral("Warning")},
        {QStringLiteral("AP-Controller-01"), 6, QStringLiteral("Warning")},
        {QStringLiteral("Core-Switch-02"), 5, QStringLiteral("Info")},
        {QStringLiteral("VPN-Gateway-01"), 4, QStringLiteral("Info")},
    };

    for (int i = 0; i < topDevices.size(); ++i) {
        const auto& d = topDevices[i];
        QString levelColor = QStringLiteral("#F56C6C");
        if (d.topLevel == QStringLiteral("Critical")) levelColor = QStringLiteral("#E6A23C");
        else if (d.topLevel == QStringLiteral("Warning")) levelColor = QStringLiteral("#E6DB5C");
        else if (d.topLevel == QStringLiteral("Info")) levelColor = QStringLiteral("#409EFF");

        body += QString(QStringLiteral(
            "<tr><td>%1</td><td>%2</td><td>%3</td>"
            "<td style=\"color:%4; font-weight:bold;\">%5</td></tr>"))
            .arg(i + 1).arg(d.name).arg(d.count)
            .arg(levelColor, d.topLevel);
    }
    body += QStringLiteral("</table>");

    body += QString(QStringLiteral(
        "<br><p><strong>告警总数:</strong> %1 | "
        "<strong>已确认:</strong> %2 | "
        "<strong>未确认:</strong> %3 | "
        "<strong>已恢复:</strong> %4</p>"))
        .arg(totalAlarms).arg(rng->bounded(80, 100))
        .arg(rng->bounded(10, 30)).arg(rng->bounded(5, 15));

    return body;
}

QString ReportWidget::generatePerformanceReport()
{
    QString device = m_deviceCombo->currentText();
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    auto* rng = QRandomGenerator::global();

    QString body;
    body += QStringLiteral("<h2>性能监控数据</h2>");
    body += QString(QStringLiteral("<p>监控设备: %1 | 生成时间: %2</p>")).arg(device, timeStr);

    body += QStringLiteral("<h3>CPU 使用率趋势 (最近24小时)</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>时间</th><th>CPU 使用率</th><th>内存使用率</th><th>温度</th></tr>");

    for (int h = 0; h < 24; h += 2) {
        int cpu = rng->bounded(10, 75);
        int mem = rng->bounded(30, 85);
        int temp = rng->bounded(38, 65);

        QString cpuColor = cpu > 70 ? QStringLiteral("#F56C6C") :
                           (cpu > 50 ? QStringLiteral("#E6A23C") : QStringLiteral("#67C23A"));
        QString memColor = mem > 80 ? QStringLiteral("#F56C6C") :
                           (mem > 60 ? QStringLiteral("#E6A23C") : QStringLiteral("#67C23A"));

        body += QString(QStringLiteral(
            "<tr><td>%1:00</td>"
            "<td style=\"color:%2;\">%3%</td>"
            "<td style=\"color:%4;\">%5%</td>"
            "<td>%6°C</td></tr>"))
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(cpuColor).arg(cpu)
            .arg(memColor).arg(mem)
            .arg(temp);
    }
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>接口流量统计 (Top 5)</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>接口</th><th>入流量</th><th>出流量</th><th>总流量</th><th>峰值利用率</th></tr>");

    struct TrafficData {
        QString iface;
        double inGbps;
        double outGbps;
        double peakPct;
    };

    QList<TrafficData> traffics = {
        {QStringLiteral("GigabitEthernet0/0"), 2.3, 1.8, 45.2},
        {QStringLiteral("GigabitEthernet0/1"), 1.5, 3.2, 62.8},
        {QStringLiteral("TenGigabitEthernet1/0"), 4.8, 5.1, 78.3},
        {QStringLiteral("GigabitEthernet0/2"), 0.8, 0.6, 12.5},
        {QStringLiteral("TenGigabitEthernet1/1"), 3.6, 2.9, 55.7},
    };

    for (const auto& t : traffics) {
        QString peakColor = t.peakPct > 80 ? QStringLiteral("#F56C6C") :
                            (t.peakPct > 60 ? QStringLiteral("#E6A23C") : QStringLiteral("#67C23A"));
        body += QString(QStringLiteral(
            "<tr><td>%1</td><td>%2 Gbps</td><td>%3 Gbps</td>"
            "<td>%4 Gbps</td>"
            "<td style=\"color:%5; font-weight:bold;\">%6%</td></tr>"))
            .arg(t.iface).arg(t.inGbps, 0, 'f', 1).arg(t.outGbps, 0, 'f', 1)
            .arg(t.inGbps + t.outGbps, 0, 'f', 1)
            .arg(peakColor).arg(t.peakPct, 0, 'f', 1);
    }
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>关键指标摘要</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>当前值</th><th>平均值</th><th>峰值</th><th>状态</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>CPU 平均使用率</td><td>%1%</td><td>%2%</td><td>%3%</td>"
        "<td style=\"color:#67C23A;\">正常</td></tr>"
        "<tr><td>内存平均使用率</td><td>%4%</td><td>%5%</td><td>%6%</td>"
        "<td style=\"color:#67C23A;\">正常</td></tr>"
        "<tr><td>总流量</td><td>%7 Gbps</td><td>%8 Gbps</td><td>%9 Gbps</td>"
        "<td style=\"color:#67C23A;\">正常</td></tr>"))
        .arg(rng->bounded(20, 40)).arg(rng->bounded(25, 38)).arg(rng->bounded(50, 75))
        .arg(rng->bounded(40, 55)).arg(rng->bounded(42, 52)).arg(rng->bounded(60, 85))
        .arg(rng->bounded(8, 15)).arg(rng->bounded(6, 12)).arg(rng->bounded(15, 20));
    body += QStringLiteral("</table>");

    return body;
}

QString ReportWidget::generateAssetReport()
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    QString body;
    body += QStringLiteral("<h2>设备资产概览</h2>");
    body += QString(QStringLiteral("<p>统计时间: %1</p>")).arg(timeStr);

    body += QStringLiteral("<h3>设备类型分布</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>设备类型</th><th>数量</th><th>占比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>交换机</td><td>32</td><td>38.1%</td></tr>"
        "<tr><td>路由器</td><td>18</td><td>21.4%</td></tr>"
        "<tr><td>防火墙</td><td>8</td><td>9.5%</td></tr>"
        "<tr><td>无线 AP</td><td>48</td><td>57.1%</td></tr>"
        "<tr><td>服务器</td><td>24</td><td>28.6%</td></tr>"
        "<tr><td>负载均衡</td><td>4</td><td>4.8%</td></tr>"
        "<tr><td>存储设备</td><td>6</td><td>7.1%</td></tr>"
        "<tr><td><strong>总计</strong></td><td><strong>84</strong></td><td><strong>100%</strong></td></tr>"));

    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>厂商分布</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>厂商</th><th>数量</th><th>占比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>Cisco</td><td>28</td><td>33.3%</td></tr>"
        "<tr><td>Huawei</td><td>22</td><td>26.2%</td></tr>"
        "<tr><td>H3C</td><td>15</td><td>17.9%</td></tr>"
        "<tr><td>Juniper</td><td>8</td><td>9.5%</td></tr>"
        "<tr><td>Aruba</td><td>6</td><td>7.1%</td></tr>"
        "<tr><td>其他</td><td>5</td><td>6.0%</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>设备状态</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>状态</th><th>数量</th><th>占比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td style=\"color:#67C23A;\">在线</td><td>78</td><td>92.9%</td></tr>"
        "<tr><td style=\"color:#E6A23C;\">维护中</td><td>4</td><td>4.8%</td></tr>"
        "<tr><td style=\"color:#F56C6C;\">离线</td><td>2</td><td>2.4%</td></tr>"));
    body += QStringLiteral("</table>");

    return body;
}

QString ReportWidget::generateConfigChangeReport()
{
    QString device = m_deviceCombo->currentText();
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    QString body;
    body += QStringLiteral("<h2>配置变更记录</h2>");
    body += QString(QStringLiteral("<p>变更范围: %1 | 统计时间: %2</p>")).arg(device, timeStr);

    body += QStringLiteral("<table>"
        "<tr><th>时间</th><th>设备</th><th>变更类型</th><th>操作人</th><th>变更摘要</th></tr>");

    struct ChangeRecord {
        QString time;
        QString device;
        QString type;
        QString operator_;
        QString summary;
    };

    QList<ChangeRecord> changes = {
        {QStringLiteral("2026-07-17 09:15"), QStringLiteral("Core-Switch-01"), QStringLiteral("VLAN"), QStringLiteral("admin"), QStringLiteral("新增 VLAN 200-210")},
        {QStringLiteral("2026-07-17 08:30"), QStringLiteral("FW-Cluster-01"), QStringLiteral("ACL"), QStringLiteral("admin"), QStringLiteral("新增 SSH 白名单规则")},
        {QStringLiteral("2026-07-16 18:00"), QStringLiteral("Core-Router-01"), QStringLiteral("路由"), QStringLiteral("netops"), QStringLiteral("新增静态路由 10.10.0.0/16")},
        {QStringLiteral("2026-07-16 14:20"), QStringLiteral("Agg-Switch-03"), QStringLiteral("端口"), QStringLiteral("netops"), QStringLiteral("启用端口 Gi0/0/24")},
        {QStringLiteral("2026-07-15 10:45"), QStringLiteral("Core-Switch-02"), QStringLiteral("STP"), QStringLiteral("admin"), QStringLiteral("修改 STP 优先级")},
        {QStringLiteral("2026-07-15 09:00"), QStringLiteral("VPN-Gateway-01"), QStringLiteral("VPN"), QStringLiteral("netops"), QStringLiteral("更新 IPSec 隧道配置")},
        {QStringLiteral("2026-07-14 16:30"), QStringLiteral("Core-Router-01"), QStringLiteral("接口"), QStringLiteral("admin"), QStringLiteral("修改接口 MTU 为 9000")},
    };

    for (const auto& c : changes) {
        QString typeColor = (c.type == QStringLiteral("ACL") || c.type == QStringLiteral("VPN"))
            ? QStringLiteral("#F56C6C") : QStringLiteral("#409EFF");
        body += QString(QStringLiteral(
            "<tr><td>%1</td><td>%2</td>"
            "<td style=\"color:%3; font-weight:bold;\">%4</td>"
            "<td>%5</td><td>%6</td></tr>"))
            .arg(c.time, c.device, typeColor, c.type, c.operator_, c.summary);
    }
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>变更统计</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>数值</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>本周变更总数</td><td>7</td></tr>"
        "<tr><td>已审批变更</td><td>7</td></tr>"
        "<tr><td>变更成功率</td><td style=\"color:#67C23A;\">100%</td></tr>"
        "<tr><td>回滚次数</td><td>0</td></tr>"));
    body += QStringLiteral("</table>");

    return body;
}

QString ReportWidget::generateHealthReport()
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    auto* rng = QRandomGenerator::global();
    int overallScore = 85 + rng->bounded(0, 11);

    QString body;
    body += QStringLiteral("<h2>网络健康综合评分</h2>");
    body += QString(QStringLiteral("<p>评估时间: %1</p>")).arg(timeStr);

    // 综合评分大卡片
    QString scoreColor = overallScore >= 90 ? QStringLiteral("#67C23A") :
                         (overallScore >= 80 ? QStringLiteral("#409EFF") :
                          (overallScore >= 70 ? QStringLiteral("#E6A23C") : QStringLiteral("#F56C6C")));
    QString scoreGrade = overallScore >= 90 ? QStringLiteral("优秀") :
                         (overallScore >= 80 ? QStringLiteral("良好") :
                          (overallScore >= 70 ? QStringLiteral("一般") : QStringLiteral("较差")));

    body += QString(QStringLiteral(
        "<div style=\"text-align:center; padding:20px; background-color:#25262B; "
        "border-radius:8px; margin:10px 0;\">"
        "<p style=\"font-size:48px; color:%1; font-weight:bold; margin:0;\">%2</p>"
        "<p style=\"font-size:18px; color:%1; margin:5px 0;\">%3</p>"
        "</div>"))
        .arg(scoreColor).arg(overallScore).arg(scoreGrade);

    body += QStringLiteral("<br><h3>各维度评分</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>评估维度</th><th>评分</th><th>等级</th><th>说明</th></tr>");

    struct Dimension {
        QString name;
        int score;
        QString desc;
    };

    QList<Dimension> dims = {
        {QStringLiteral("设备可用性"), 92, QStringLiteral("设备在线率 98.5%，关键设备无宕机")},
        {QStringLiteral("链路质量"), 88, QStringLiteral("链路带宽利用率合理，延迟正常")},
        {QStringLiteral("安全状态"), 85, QStringLiteral("安全事件已处理，无高危漏洞")},
        {QStringLiteral("配置合规"), 90, QStringLiteral("配置基线合规率 95%")},
        {QStringLiteral("性能指标"), 87, QStringLiteral("CPU/内存使用率在阈值内")},
        {QStringLiteral("冗余保障"), 82, QStringLiteral("主备切换测试正常，备份链路可用")},
    };

    for (const auto& d : dims) {
        QString color = d.score >= 90 ? QStringLiteral("#67C23A") :
                        (d.score >= 80 ? QStringLiteral("#409EFF") :
                         (d.score >= 70 ? QStringLiteral("#E6A23C") : QStringLiteral("#F56C6C")));
        QString grade = d.score >= 90 ? QStringLiteral("优秀") :
                        (d.score >= 80 ? QStringLiteral("良好") :
                         (d.score >= 70 ? QStringLiteral("一般") : QStringLiteral("较差")));
        body += QString(QStringLiteral(
            "<tr><td>%1</td>"
            "<td style=\"color:%2; font-weight:bold;\">%3</td>"
            "<td style=\"color:%2;\">%4</td>"
            "<td>%5</td></tr>"))
            .arg(d.name, color).arg(d.score).arg(grade, d.desc);
    }
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>改进建议</h3><ul>");
    body += QStringLiteral("<li>建议对冗余链路进行定期切换演练</li>");
    body += QStringLiteral("<li>加强对防火墙会话数的监控，设置合理告警阈值</li>");
    body += QStringLiteral("<li>定期更新设备固件版本，修复已知漏洞</li>");
    body += QStringLiteral("</ul>");

    return body;
}

QString ReportWidget::generateDailyReport()
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    auto* rng = QRandomGenerator::global();

    QString body;
    body += QString(QStringLiteral("<h2>网络运维日报</h2>"));
    body += QString(QStringLiteral("<p>日期: %1</p>")).arg(timeStr);

    // 今日概览
    body += QStringLiteral("<h3>今日概览</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>数值</th><th>状态</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>设备在线率</td><td>98.5%</td><td style=\"color:#67C23A;\">正常</td></tr>"
        "<tr><td>今日告警</td><td>%1 条</td><td style=\"color:%2;\">%3</td></tr>"
        "<tr><td>今日变更</td><td>%4 条</td><td style=\"color:#67C23A;\">正常</td></tr>"
        "<tr><td>平均 CPU</td><td>%5%</td><td style=\"color:#67C23A;\">正常</td></tr>"
        "<tr><td>峰值流量</td><td>%6 Gbps</td><td style=\"color:#67C23A;\">正常</td></tr>"))
        .arg(rng->bounded(2, 5))
        .arg(rng->bounded(2, 5) > 3 ? QStringLiteral("#E6A23C") : QStringLiteral("#67C23A"))
        .arg(rng->bounded(2, 5) > 3 ? QStringLiteral("关注") : QStringLiteral("正常"))
        .arg(rng->bounded(0, 3))
        .arg(rng->bounded(20, 40))
        .arg(rng->bounded(5, 12));
    body += QStringLiteral("</table>");

    // 今日告警摘要
    body += QStringLiteral("<br><h3>今日告警摘要</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>时间</th><th>设备</th><th>级别</th><th>描述</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>09:15</td><td>Core-Switch-01</td>"
        "<td style=\"color:#E6A23C;\">Warning</td>"
        "<td>CPU 使用率短暂升高至 82%</td></tr>"
        "<tr><td>11:30</td><td>FW-Cluster-01</td>"
        "<td style=\"color:#409EFF;\">Info</td>"
        "<td>HA 状态切换日志</td></tr>"));
    body += QStringLiteral("</table>");

    // 今日变更
    body += QStringLiteral("<br><h3>今日配置变更</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>时间</th><th>设备</th><th>变更内容</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>09:15</td><td>Core-Switch-01</td><td>新增 VLAN 200-210</td></tr>"
        "<tr><td>08:30</td><td>FW-Cluster-01</td><td>新增 SSH 白名单规则</td></tr>"));
    body += QStringLiteral("</table>");

    return body;
}

QString ReportWidget::generateWeeklyReport()
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));

    QString body;
    body += QString(QStringLiteral("<h2>网络运维周报</h2>"));
    body += QString(QStringLiteral("<p>报告周期: 本周 | 生成日期: %1</p>")).arg(timeStr);

    body += QStringLiteral("<h3>本周概览</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>本周</th><th>上周</th><th>变化</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>设备在线率</td><td>98.5%</td><td>98.2%</td>"
        "<td style=\"color:#67C23A;\">↑ 0.3%</td></tr>"
        "<tr><td>告警总数</td><td>127</td><td>143</td>"
        "<td style=\"color:#67C23A;\">↓ 11.2%</td></tr>"
        "<tr><td>变更总数</td><td>15</td><td>12</td>"
        "<td style=\"color:#E6A23C;\">↑ 25%</td></tr>"
        "<tr><td>平均 CPU</td><td>32%</td><td>35%</td>"
        "<td style=\"color:#67C23A;\">↓ 3%</td></tr>"
        "<tr><td>峰值流量</td><td>12.5 Gbps</td><td>11.8 Gbps</td>"
        "<td style=\"color:#E6A23C;\">↑ 5.9%</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>告警趋势</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>星期</th><th>告警数</th><th>Emergency</th><th>Critical</th><th>Warning</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>周一</td><td>18</td><td>0</td><td>2</td><td>5</td></tr>"
        "<tr><td>周二</td><td>22</td><td>1</td><td>3</td><td>7</td></tr>"
        "<tr><td>周三</td><td>15</td><td>0</td><td>1</td><td>4</td></tr>"
        "<tr><td>周四</td><td>20</td><td>0</td><td>2</td><td>6</td></tr>"
        "<tr><td>周五</td><td>25</td><td>1</td><td>4</td><td>8</td></tr>"
        "<tr><td>周六</td><td>14</td><td>0</td><td>1</td><td>3</td></tr>"
        "<tr><td>周日</td><td>13</td><td>0</td><td>1</td><td>2</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>本周重点工作</h3><ul>");
    body += QStringLiteral("<li>完成核心交换机 VLAN 规划调整</li>");
    body += QStringLiteral("<li>处理防火墙会话数告警，优化连接数限制策略</li>");
    body += QStringLiteral("<li>完成 AP 固件升级，覆盖 48 台设备</li>");
    body += QStringLiteral("<li>新增 2 条 IPSec VPN 隧道</li>");
    body += QStringLiteral("</ul>");

    body += QStringLiteral("<br><h3>下周计划</h3><ul>");
    body += QStringLiteral("<li>核心网络设备固件升级</li>");
    body += QStringLiteral("<li>冗余链路切换演练</li>");
    body += QStringLiteral("<li>网络安全基线检查</li>");
    body += QStringLiteral("</ul>");

    return body;
}

QString ReportWidget::generateMonthlyReport()
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM"));

    QString body;
    body += QString(QStringLiteral("<h2>网络运维月报</h2>"));
    body += QString(QStringLiteral("<p>报告周期: %1 | 生成日期: %2</p>"))
        .arg(timeStr, QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd")));

    body += QStringLiteral("<h3>月度概览</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>指标</th><th>本月</th><th>上月</th><th>环比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>设备在线率</td><td>98.5%</td><td>98.1%</td>"
        "<td style=\"color:#67C23A;\">↑ 0.4%</td></tr>"
        "<tr><td>告警总数</td><td>523</td><td>587</td>"
        "<td style=\"color:#67C23A;\">↓ 10.9%</td></tr>"
        "<tr><td>变更总数</td><td>62</td><td>55</td>"
        "<td style=\"color:#E6A23C;\">↑ 12.7%</td></tr>"
        "<tr><td>平均 CPU</td><td>31%</td><td>34%</td>"
        "<td style=\"color:#67C23A;\">↓ 3%</td></tr>"
        "<tr><td>峰值流量</td><td>15.2 Gbps</td><td>14.1 Gbps</td>"
        "<td style=\"color:#E6A23C;\">↑ 7.8%</td></tr>"
        "<tr><td>可用性 SLA</td><td>99.97%</td><td>99.95%</td>"
        "<td style=\"color:#67C23A;\">↑ 0.02%</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>月度告警统计</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>级别</th><th>数量</th><th>占比</th><th>环比</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td style=\"color:#F56C6C;\">Emergency</td><td>8</td><td>1.5%</td>"
        "<td style=\"color:#67C23A;\">↓ 33%</td></tr>"
        "<tr><td style=\"color:#E6A23C;\">Critical</td><td>45</td><td>8.6%</td>"
        "<td style=\"color:#67C23A;\">↓ 15%</td></tr>"
        "<tr><td style=\"color:#E6DB5C;\">Warning</td><td>156</td><td>29.8%</td>"
        "<td style=\"color:#67C23A;\">↓ 8%</td></tr>"
        "<tr><td style=\"color:#409EFF;\">Info</td><td>314</td><td>60.0%</td>"
        "<td style=\"color:#67C23A;\">↓ 5%</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>月度变更统计</h3>");
    body += QStringLiteral("<table>"
        "<tr><th>变更类型</th><th>数量</th><th>成功率</th></tr>");
    body += QString(QStringLiteral(
        "<tr><td>VLAN 变更</td><td>12</td><td style=\"color:#67C23A;\">100%</td></tr>"
        "<tr><td>ACL/安全策略</td><td>18</td><td style=\"color:#67C23A;\">100%</td></tr>"
        "<tr><td>路由变更</td><td>8</td><td style=\"color:#67C23A;\">100%</td></tr>"
        "<tr><td>端口变更</td><td>15</td><td style=\"color:#E6A23C;\">93.3%</td></tr>"
        "<tr><td>固件升级</td><td>5</td><td style=\"color:#67C23A;\">100%</td></tr>"
        "<tr><td>VPN 变更</td><td>4</td><td style=\"color:#67C23A;\">100%</td></tr>"));
    body += QStringLiteral("</table>");

    body += QStringLiteral("<br><h3>下月计划</h3><ul>");
    body += QStringLiteral("<li>核心网络设备季度维护窗口</li>");
    body += QStringLiteral("<li>网络安全合规审计</li>");
    body += QStringLiteral("<li>灾备链路切换演练</li>");
    body += QStringLiteral("<li>网络扩容规划评估</li>");
    body += QStringLiteral("</ul>");

    return body;
}

// ============================================================================
// HTML 模板
// ============================================================================
QString ReportWidget::generateHtmlTemplate(const QString& title, const QString& body)
{
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    QString html;
    html += QStringLiteral("<!DOCTYPE html><html><head><meta charset=\"UTF-8\">");
    html += QStringLiteral("<style>");
    html += QStringLiteral(
        "body { font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif; "
        "background-color: #1E1F22; color: #DCDCDC; "
        "padding: 20px; margin: 0; }");
    html += QStringLiteral(
        "h1 { color: #409EFF; border-bottom: 2px solid #409EFF; "
        "padding-bottom: 8px; font-size: 22px; }");
    html += QStringLiteral(
        "h2 { color: #409EFF; font-size: 18px; margin-top: 20px; }");
    html += QStringLiteral(
        "h3 { color: #67C23A; font-size: 15px; margin-top: 16px; }");
    html += QStringLiteral(
        "p { line-height: 1.6; color: #DCDCDC; }");
    html += QStringLiteral(
        "table { width: 100%%; border-collapse: collapse; "
        "margin: 10px 0; font-size: 13px; }");
    html += QStringLiteral(
        "th { background-color: #25262B; color: #8C8C8C; "
        "padding: 8px 10px; text-align: left; "
        "border-bottom: 2px solid #3C3F41; font-weight: bold; }");
    html += QStringLiteral(
        "td { padding: 6px 10px; border-bottom: 1px solid #2C2D30; }");
    html += QStringLiteral(
        "tr:nth-child(even) { background-color: #222326; }");
    html += QStringLiteral(
        "tr:hover { background-color: #2C2D30; }");
    html += QStringLiteral(
        "ul { color: #DCDCDC; line-height: 1.8; }");
    html += QStringLiteral(
        "strong { color: #E6A23C; }");
    html += QStringLiteral(
        ".footer { margin-top: 30px; padding-top: 10px; "
        "border-top: 1px solid #3C3F41; color: #8C8C8C; "
        "font-size: 12px; text-align: center; }");
    html += QStringLiteral("</style></head><body>");

    html += QString(QStringLiteral("<h1>%1</h1>")).arg(title);
    html += QString(QStringLiteral("<p style=\"color:#8C8C8C;\">生成时间: %1</p>")).arg(timeStr);
    html += body;

    html += QStringLiteral(
        "<div class=\"footer\">"
        "WukongToolkit 网络工程师工具箱 — 报表中心自动生成<br>"
        "此报告为自动生成，仅供参考。");
    html += QString(QStringLiteral(" 生成时间: %1</div>")).arg(timeStr);

    html += QStringLiteral("</body></html>");

    return html;
}

// ============================================================================
// 导出 HTML
// ============================================================================
void ReportWidget::exportToHtml(const QString& content)
{
    QString defaultName = QString(QStringLiteral("report_%1.html"))
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("导出 HTML 报表"), defaultName,
        QStringLiteral("HTML 文件 (*.html)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QString(QStringLiteral("无法写入文件: %1")).arg(filePath));
        Logger::instance().error(QStringLiteral("ReportCenter"),
            QString(QStringLiteral("HTML 导出失败: %1")).arg(filePath));
        return;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    addHistoryEntry(m_currentReportTitle, m_deviceCombo->currentText(),
        QStringLiteral("HTML"), filePath);

    Logger::instance().info(QStringLiteral("ReportCenter"),
        QString(QStringLiteral("HTML 报表已导出: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("HTML 报表已导出到:\n%1")).arg(filePath));
}

// ============================================================================
// 导出 Markdown
// ============================================================================
void ReportWidget::exportToMarkdown(const QString& content)
{
    QString defaultName = QString(QStringLiteral("report_%1.md"))
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("导出 Markdown 报表"), defaultName,
        QStringLiteral("Markdown 文件 (*.md)"));

    if (filePath.isEmpty()) return;

    // 简单 HTML 到 Markdown 转换
    QString md;
    md += QStringLiteral("# %1\n\n").arg(m_currentReportTitle);
    md += QString(QStringLiteral("> 生成时间: %1\n\n"))
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // 简化转换：将常见 HTML 标签转为 Markdown
    QString markdown = content;
    markdown.replace(QRegularExpression(QStringLiteral("<h2>(.*?)</h2>")),
        QStringLiteral("\n## \\1\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<h3>(.*?)</h3>")),
        QStringLiteral("\n### \\1\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<strong>(.*?)</strong>")),
        QStringLiteral("**\\1**"));
    markdown.replace(QRegularExpression(QStringLiteral("<li>(.*?)</li>")),
        QStringLiteral("- \\1\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<ul>|</ul>")), QStringLiteral(""));
    markdown.replace(QRegularExpression(QStringLiteral("<p>(.*?)</p>")),
        QStringLiteral("\\1\n\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<br\\s*/?>")),
        QStringLiteral("\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<div[^>]*>|</div>")),
        QStringLiteral(""));
    markdown.replace(QRegularExpression(QStringLiteral("<table>")),
        QStringLiteral("\n"));
    markdown.replace(QRegularExpression(QStringLiteral("</table>")),
        QStringLiteral("\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<tr>(.*?)</tr>")),
        QStringLiteral("|\\1|\n"));
    markdown.replace(QRegularExpression(QStringLiteral("<th>(.*?)</th>")),
        QStringLiteral(" **\\1** |"));
    markdown.replace(QRegularExpression(QStringLiteral("<td[^>]*>(.*?)</td>")),
        QStringLiteral(" \\1 |"));
    // 移除残留 HTML 标签
    markdown.replace(QRegularExpression(QStringLiteral("<[^>]+>")), QStringLiteral(""));

    md += markdown;

    md += QStringLiteral("\n\n---\n*WukongToolkit 报表中心自动生成*\n");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QString(QStringLiteral("无法写入文件: %1")).arg(filePath));
        Logger::instance().error(QStringLiteral("ReportCenter"),
            QString(QStringLiteral("Markdown 导出失败: %1")).arg(filePath));
        return;
    }

    QTextStream out(&file);
    out << md;
    file.close();

    addHistoryEntry(m_currentReportTitle, m_deviceCombo->currentText(),
        QStringLiteral("Markdown"), filePath);

    Logger::instance().info(QStringLiteral("ReportCenter"),
        QString(QStringLiteral("Markdown 报表已导出: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("Markdown 报表已导出到:\n%1")).arg(filePath));
}

// ============================================================================
// 导出 CSV
// ============================================================================
void ReportWidget::exportToCsv(const QString& content)
{
    QString defaultName = QString(QStringLiteral("report_%1.csv"))
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("导出 CSV 报表"), defaultName,
        QStringLiteral("CSV 文件 (*.csv)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QString(QStringLiteral("无法写入文件: %1")).arg(filePath));
        Logger::instance().error(QStringLiteral("ReportCenter"),
            QString(QStringLiteral("CSV 导出失败: %1")).arg(filePath));
        return;
    }

    QTextStream out(&file);
    // BOM for Excel UTF-8
    out << QStringLiteral("\xEF\xBB\xBF");

    // 简单 HTML 表格转 CSV
    QString csv;
    csv += QStringLiteral("\"报表标题\",\"%1\"\n").arg(m_currentReportTitle);
    csv += QString(QStringLiteral("\"生成时间\",\"%1\"\n\n"))
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // 提取表格数据
    QRegularExpression tableRe(QStringLiteral("<table>(.*?)</table>"),
        QRegularExpression::DotMatchesEverythingOption);
    auto it = tableRe.globalMatch(content);

    int tableIdx = 0;
    while (it.hasNext()) {
        auto match = it.next();
        QString tableContent = match.captured(1);

        csv += QString(QStringLiteral("\"--- 表格 %1 ---\"\n")).arg(++tableIdx);

        // 提取表头
        QRegularExpression thRe(QStringLiteral("<th[^>]*>(.*?)</th>"));
        auto thIt = thRe.globalMatch(tableContent);
        QStringList headers;
        while (thIt.hasNext()) {
            auto thMatch = thIt.next();
            QString h = thMatch.captured(1).remove(QRegularExpression(QStringLiteral("<[^>]+>"))).trimmed();
            headers.append(h);
        }
        if (!headers.isEmpty()) {
            csv += QStringLiteral("\"") + headers.join(QStringLiteral("\",\"")) + QStringLiteral("\"\n");
        }

        // 提取数据行
        QRegularExpression trRe(QStringLiteral("<tr>(.*?)</tr>"),
            QRegularExpression::DotMatchesEverythingOption);
        auto trIt = trRe.globalMatch(tableContent);
        while (trIt.hasNext()) {
            auto trMatch = trIt.next();
            QString rowContent = trMatch.captured(1);

            if (rowContent.contains(QStringLiteral("<th"))) continue; // 跳过头行

            QRegularExpression tdRe(QStringLiteral("<td[^>]*>(.*?)</td>"));
            auto tdIt = tdRe.globalMatch(rowContent);
            QStringList cells;
            while (tdIt.hasNext()) {
                auto tdMatch = tdIt.next();
                QString cell = tdMatch.captured(1).remove(QRegularExpression(QStringLiteral("<[^>]+>"))).trimmed();
                // CSV 转义
                if (cell.contains(QLatin1Char(',')) || cell.contains(QLatin1Char('"')) || cell.contains(QLatin1Char('\n'))) {
                    cell.replace(QStringLiteral("\""), QStringLiteral("\"\""));
                    cell = QStringLiteral("\"") + cell + QStringLiteral("\"");
                }
                cells.append(cell);
            }
            if (!cells.isEmpty()) {
                csv += cells.join(QStringLiteral(",")) + QStringLiteral("\n");
            }
        }
        csv += QStringLiteral("\n");
    }

    out << csv;
    file.close();

    addHistoryEntry(m_currentReportTitle, m_deviceCombo->currentText(),
        QStringLiteral("CSV"), filePath);

    Logger::instance().info(QStringLiteral("ReportCenter"),
        QString(QStringLiteral("CSV 报表已导出: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("CSV 报表已导出到:\n%1")).arg(filePath));
}

// ============================================================================
// 添加历史记录
// ============================================================================
void ReportWidget::addHistoryEntry(const QString& type, const QString& device,
                                    const QString& format, const QString& filePath)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QString fileName = QFileInfo(filePath).fileName();

    m_historyTable->setItem(row, 0, new QTableWidgetItem(timeStr));
    m_historyTable->setItem(row, 1, new QTableWidgetItem(type));
    m_historyTable->setItem(row, 2, new QTableWidgetItem(device));
    m_historyTable->setItem(row, 3, new QTableWidgetItem(format));
    m_historyTable->setItem(row, 4, new QTableWidgetItem(fileName));

    m_historyTable->scrollToBottom();
}

// ============================================================================
// 绘制柱状图
// ============================================================================
void ReportWidget::drawChart(QPainter& painter, const QRect& rect)
{
    painter.save();

    // 背景
    painter.fillRect(rect, QColor(QStringLiteral("#1E1F22")));

    QRect chartRect = rect.adjusted(40, 20, -20, -40);
    if (chartRect.width() <= 0 || chartRect.height() <= 0) {
        painter.restore();
        return;
    }

    // 模拟数据：各类报表统计
    struct BarData {
        QString label;
        double value;
        QColor color;
    };

    QList<BarData> bars = {
        {QStringLiteral("巡检"), 92.5, QColor(QStringLiteral("#67C23A"))},
        {QStringLiteral("告警"), 128.0, QColor(QStringLiteral("#F56C6C"))},
        {QStringLiteral("性能"), 85.0, QColor(QStringLiteral("#409EFF"))},
        {QStringLiteral("资产"), 84.0, QColor(QStringLiteral("#E6A23C"))},
        {QStringLiteral("变更"), 7.0, QColor(QStringLiteral("#E6DB5C"))},
        {QStringLiteral("健康"), 88.0, QColor(QStringLiteral("#67C23A"))},
    };

    double maxVal = 0;
    for (const auto& b : bars) {
        if (b.value > maxVal) maxVal = b.value;
    }
    maxVal = std::max(maxVal, 1.0);

    double barWidth = (double)chartRect.width() / bars.size() * 0.6;
    double gap = (double)chartRect.width() / bars.size();

    // 标题
    painter.setPen(QColor(QStringLiteral("#8C8C8C")));
    painter.setFont(QFont(QStringLiteral("PingFang SC"), 11));
    painter.drawText(QRect(rect.left(), rect.top(), rect.width(), 20),
        Qt::AlignCenter, QStringLiteral("报表统计概览"));

    // Y 轴刻度
    painter.setPen(QColor(QStringLiteral("#5C5D60")));
    painter.setFont(QFont(QStringLiteral("PingFang SC"), 9));
    for (int i = 0; i <= 4; ++i) {
        double val = maxVal * i / 4.0;
        int y = chartRect.bottom() - (int)(chartRect.height() * i / 4.0);
        painter.drawLine(chartRect.left(), y, chartRect.right(), y);
        painter.drawText(QRect(chartRect.left() - 38, y - 8, 34, 16),
            Qt::AlignRight | Qt::AlignVCenter,
            QString::number(val, 'f', 0));
    }

    // 柱状图
    for (int i = 0; i < bars.size(); ++i) {
        double barH = (bars[i].value / maxVal) * chartRect.height();
        int x = chartRect.left() + (int)(gap * i + (gap - barWidth) / 2.0);
        int y = chartRect.bottom() - (int)barH;
        int w = (int)barWidth;
        int h = (int)barH;

        // 渐变柱体
        QLinearGradient gradient(x, y, x, chartRect.bottom());
        gradient.setColorAt(0.0, bars[i].color.lighter(130));
        gradient.setColorAt(1.0, bars[i].color);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(x, y, w, h, 3, 3);

        // 数值标签
        painter.setPen(QColor(QStringLiteral("#DCDCDC")));
        painter.setFont(QFont(QStringLiteral("PingFang SC"), 9));
        painter.drawText(QRect(x - 5, y - 18, w + 10, 16),
            Qt::AlignCenter, QString::number(bars[i].value, 'f', 1));

        // X 轴标签
        painter.drawText(QRect(x - 5, chartRect.bottom() + 4, w + 10, 20),
            Qt::AlignCenter, bars[i].label);
    }

    painter.restore();
}