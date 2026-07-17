#include "qos/QoSWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QPlainTextEdit>
#include <QRandomGenerator>
#include <QDateTime>
#include <QSplitter>
#include <QFrame>

// ═══════════════════════════════════════════════════════════════════════════════
// DscpChartWidget
// ═══════════════════════════════════════════════════════════════════════════════

DscpChartWidget::DscpChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_ef(0)
    , m_af41(0)
    , m_af31(0)
    , m_af21(0)
    , m_be(0)
{
    setMinimumSize(280, 180);
    setStyleSheet("background-color: #1E1F22; border: 1px solid #3C3F41; border-radius: 4px;");
}

void DscpChartWidget::setData(int ef, int af41, int af31, int af21, int be)
{
    m_ef = ef;
    m_af41 = af41;
    m_af31 = af31;
    m_af21 = af21;
    m_be = be;
    update();
}

void DscpChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int marginLeft = 50;
    int marginRight = 30;
    int marginTop = 30;
    int marginBottom = 40;

    int chartW = w - marginLeft - marginRight;
    int chartH = h - marginTop - marginBottom;

    // Title
    painter.setPen(QColor("#8C8C8C"));
    painter.setFont(QFont("sans-serif", 11, QFont::Bold));
    painter.drawText(QRect(0, 4, w, 20), Qt::AlignCenter, "DSCP 分布");

    // Background
    painter.fillRect(marginLeft, marginTop, chartW, chartH, QColor("#25262B"));

    // Grid lines
    painter.setPen(QPen(QColor("#3C3F41"), 1, Qt::DotLine));
    for (int i = 0; i <= 4; ++i) {
        int y = marginTop + chartH * i / 4;
        painter.drawLine(marginLeft, y, marginLeft + chartW, y);
    }

    // Y-axis labels
    painter.setPen(QColor("#8C8C8C"));
    painter.setFont(QFont("sans-serif", 9));
    for (int i = 0; i <= 4; ++i) {
        int y = marginTop + chartH * i / 4;
        painter.drawText(QRect(0, y - 10, marginLeft - 8, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString("%1%").arg(100 - i * 25));
    }

    // Data
    struct Segment {
        QString label;
        int value;
        QColor color;
    };
    QList<Segment> segments = {
        {"EF",   m_ef,   QColor("#F56C6C")},
        {"AF41", m_af41, QColor("#E6A23C")},
        {"AF31", m_af31, QColor("#F2C94C")},
        {"AF21", m_af21, QColor("#409EFF")},
        {"BE",   m_be,   QColor("#909399")},
    };

    int total = m_ef + m_af41 + m_af31 + m_af21 + m_be;
    if (total == 0) total = 1;

    int barW = qMin(80, chartW / 2 - 20);
    int barX = marginLeft + (chartW - barW) / 2;

    // Stacked bar
    int barTop = marginTop;
    for (const auto& seg : segments) {
        int segH = chartH * seg.value / total;
        if (segH < 1 && seg.value > 0) segH = 1;
        painter.fillRect(barX, barTop, barW, segH, seg.color);
        painter.setPen(QPen(QColor("#1E1F22"), 1));
        painter.drawRect(barX, barTop, barW, segH);
        barTop += segH;
    }

    // Legend
    int legendY = marginTop + chartH + 8;
    int legendX = marginLeft;
    painter.setFont(QFont("sans-serif", 9));
    for (const auto& seg : segments) {
        painter.fillRect(legendX, legendY, 12, 10, seg.color);
        painter.setPen(QColor("#DCDCDC"));
        painter.drawText(QRect(legendX + 16, legendY - 2, 60, 14),
                         Qt::AlignLeft, seg.label);
        legendX += 72;
        if (legendX > marginLeft + chartW - 60) {
            legendX = marginLeft;
            legendY += 16;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// QueueChartWidget
// ═══════════════════════════════════════════════════════════════════════════════

QueueChartWidget::QueueChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(280, 180);
    setStyleSheet("background-color: #1E1F22; border: 1px solid #3C3F41; border-radius: 4px;");
}

void QueueChartWidget::setQueues(const QList<QueueInfo>& queues)
{
    m_queues = queues;
    update();
}

void QueueChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int marginLeft = 70;
    int marginRight = 50;
    int marginTop = 30;
    int marginBottom = 20;

    int chartW = w - marginLeft - marginRight;

    // Title
    painter.setPen(QColor("#8C8C8C"));
    painter.setFont(QFont("sans-serif", 11, QFont::Bold));
    painter.drawText(QRect(0, 4, w, 20), Qt::AlignCenter, "队列利用率");

    if (m_queues.isEmpty()) {
        painter.setPen(QColor("#5C5C5C"));
        painter.setFont(QFont("sans-serif", 10));
        painter.drawText(QRect(marginLeft, marginTop, chartW, h - marginTop - marginBottom),
                         Qt::AlignCenter, "暂无数据");
        return;
    }

    int n = m_queues.size();
    int barH = qMin(28, (h - marginTop - marginBottom) / n - 10);
    int totalBarsH = n * barH + (n - 1) * 8;
    int startY = marginTop + ((h - marginTop - marginBottom) - totalBarsH) / 2;

    // Grid lines
    painter.setPen(QPen(QColor("#3C3F41"), 1, Qt::DotLine));
    for (int i = 0; i <= 4; ++i) {
        int x = marginLeft + chartW * i / 4;
        painter.drawLine(x, marginTop, x, marginTop + totalBarsH + (n > 0 ? barH : 0));
    }

    // X-axis labels
    painter.setPen(QColor("#8C8C8C"));
    painter.setFont(QFont("sans-serif", 8));
    for (int i = 0; i <= 4; ++i) {
        int x = marginLeft + chartW * i / 4;
        painter.drawText(QRect(x - 20, h - marginBottom + 4, 40, 14),
                         Qt::AlignCenter, QString("%1%").arg(i * 25));
    }

    painter.setFont(QFont("sans-serif", 10, QFont::Bold));
    for (int i = 0; i < n; ++i) {
        const auto& q = m_queues[i];
        int y = startY + i * (barH + 8);

        // Queue name
        painter.setPen(QColor("#DCDCDC"));
        painter.drawText(QRect(0, y, marginLeft - 8, barH),
                         Qt::AlignRight | Qt::AlignVCenter, q.name);

        // Background bar
        painter.fillRect(marginLeft, y, chartW, barH, QColor("#25262B"));

        // Value bar
        int val = qMin(q.currentUtilization, 100);
        int fillW = chartW * val / 100;

        QColor barColor;
        if (val < 60) {
            barColor = QColor("#67C23A");
        } else if (val < 80) {
            barColor = QColor("#E6A23C");
        } else {
            barColor = QColor("#F56C6C");
        }

        painter.fillRect(marginLeft, y, fillW, barH, barColor);

        // Percentage text
        painter.setPen(QColor("#FFFFFF"));
        QFont f = painter.font();
        f.setPointSize(9);
        painter.setFont(f);
        painter.drawText(QRect(marginLeft + 4, y, fillW - 8, barH),
                         Qt::AlignLeft | Qt::AlignVCenter, QString("%1%").arg(val));

        // Right-side percentage
        painter.setPen(QColor("#8C8C8C"));
        painter.drawText(QRect(marginLeft + chartW + 4, y, marginRight - 8, barH),
                         Qt::AlignLeft | Qt::AlignVCenter, QString("%1%").arg(val));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// QoSWidget
// ═══════════════════════════════════════════════════════════════════════════════

QoSWidget::QoSWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();

    // Load default interface data
    onInterfaceChanged(0);
}

QoSWidget::~QoSWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void QoSWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──
    auto styleCombo = [](QComboBox* combo, int minWidth = 200) {
        combo->setMinimumWidth(minWidth);
        combo->setStyleSheet(
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
        );
    };

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
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
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString("QPushButton {"
                    "  background-color: %1; color: white;"
                    "  border: none; padding: 8px 20px; border-radius: 4px;"
                    "  font-size: 13px; font-weight: bold;"
                    "}"
                    "QPushButton:hover { background-color: %2; }"
                    "QPushButton:disabled { background-color: #5C5C5C; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleLabel = [](QLabel* label, const QString& color = "#8C8C8C", int fontSize = 12) {
        label->setStyleSheet(
            QString("font-size: %1px; color: %2;").arg(fontSize).arg(color)
        );
    };

    // ── Top: Device + Interface selection + Health score ──
    auto* topGroup = new QGroupBox("QoS 分析中心");
    topGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(12);

    auto* deviceLabel = new QLabel("设备:");
    styleLabel(deviceLabel, "#8C8C8C", 13);
    m_deviceCombo = new QComboBox();
    styleCombo(m_deviceCombo, 180);
    m_deviceCombo->addItems({"Core-Router-01", "Dist-Switch-02", "Access-Switch-03", "Edge-Router-04"});

    auto* ifaceLabel = new QLabel("接口:");
    styleLabel(ifaceLabel, "#8C8C8C", 13);
    m_interfaceCombo = new QComboBox();
    styleCombo(m_interfaceCombo, 180);
    m_interfaceCombo->addItems({"GigabitEthernet0/0/0", "GigabitEthernet0/0/1", "GigabitEthernet0/0/2", "GigabitEthernet0/0/3"});

    m_refreshBtn = new QPushButton("刷新");
    styleButton(m_refreshBtn, "#67C23A", "#85CE61");

    m_exportBtn = new QPushButton("导出报告");
    styleButton(m_exportBtn, "#E6A23C", "#EBB563");

    topLayout->addWidget(deviceLabel);
    topLayout->addWidget(m_deviceCombo);
    topLayout->addWidget(ifaceLabel);
    topLayout->addWidget(m_interfaceCombo);
    topLayout->addWidget(m_refreshBtn);
    topLayout->addStretch();

    // Health score card
    auto* healthFrame = new QFrame();
    healthFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #25262B;"
        "  border: 1px solid #3C3F41;"
        "  border-radius: 6px;"
        "}"
    );
    auto* healthLayout = new QHBoxLayout(healthFrame);
    healthLayout->setContentsMargins(16, 6, 16, 6);

    auto* healthTitleLabel = new QLabel("QoS 健康评分");
    healthTitleLabel->setStyleSheet("font-size: 13px; color: #8C8C8C;");

    m_healthScoreLabel = new QLabel("--");
    m_healthScoreLabel->setStyleSheet("font-size: 28px; color: #67C23A; font-weight: bold;");
    m_healthScoreLabel->setMinimumWidth(60);
    m_healthScoreLabel->setAlignment(Qt::AlignCenter);

    healthLayout->addWidget(healthTitleLabel);
    healthLayout->addWidget(m_healthScoreLabel);

    topLayout->addWidget(healthFrame);
    topLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(topGroup);

    // ── Middle: DSCP chart + Queue chart (left/right) ──
    auto* middleSplitter = new QSplitter(Qt::Horizontal);
    middleSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #3C3F41; width: 2px; }"
    );

    m_dscpChart = new DscpChartWidget();
    m_queueChart = new QueueChartWidget();

    middleSplitter->addWidget(m_dscpChart);
    middleSplitter->addWidget(m_queueChart);
    middleSplitter->setStretchFactor(0, 1);
    middleSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(middleSplitter, 1);

    // ── Metrics row: Latency / Jitter / Packet Loss ──
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(12);

    auto createMetricCard = [&](const QString& title, QLabel*& valueLabel) -> QFrame* {
        auto* frame = new QFrame();
        frame->setStyleSheet(
            "QFrame {"
            "  background-color: #25262B;"
            "  border: 1px solid #3C3F41;"
            "  border-radius: 6px;"
            "}"
        );
        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(14, 8, 14, 8);
        layout->setSpacing(4);

        auto* titleLbl = new QLabel(title);
        titleLbl->setStyleSheet("font-size: 12px; color: #8C8C8C;");

        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet("font-size: 20px; color: #409EFF; font-weight: bold;");

        layout->addWidget(titleLbl);
        layout->addWidget(valueLabel);
        return frame;
    };

    metricsLayout->addWidget(createMetricCard("时延", m_latencyLabel));
    metricsLayout->addWidget(createMetricCard("抖动", m_jitterLabel));
    metricsLayout->addWidget(createMetricCard("丢包率", m_packetLossLabel));

    mainLayout->addLayout(metricsLayout);

    // ── Bottom: QoS Policy Table + Queue Detail Table ──
    auto* bottomSplitter = new QSplitter(Qt::Horizontal);
    bottomSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #3C3F41; width: 2px; }"
    );

    // QoS Policy Table
    auto* policyWidget = new QWidget();
    auto* policyLayout = new QVBoxLayout(policyWidget);
    policyLayout->setContentsMargins(0, 0, 4, 0);
    policyLayout->setSpacing(4);

    auto* policyTitle = new QLabel("QoS 策略");
    policyTitle->setStyleSheet("font-size: 13px; color: #409EFF; font-weight: bold; padding: 4px 0;");

    m_policyTable = new QTableWidget(0, 5);
    m_policyTable->setHorizontalHeaderLabels({"策略名", "接口", "分类器", "行为", "方向"});
    m_policyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_policyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_policyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_policyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_policyTable->setColumnWidth(4, 70);
    m_policyTable->verticalHeader()->setVisible(false);
    styleTable(m_policyTable);

    policyLayout->addWidget(policyTitle);
    policyLayout->addWidget(m_policyTable);

    // Queue Detail Table
    auto* queueWidget = new QWidget();
    auto* queueLayout = new QVBoxLayout(queueWidget);
    queueLayout->setContentsMargins(4, 0, 0, 0);
    queueLayout->setSpacing(4);

    auto* queueTitle = new QLabel("队列详情");
    queueTitle->setStyleSheet("font-size: 13px; color: #409EFF; font-weight: bold; padding: 4px 0;");

    m_queueDetailTable = new QTableWidget(0, 5);
    m_queueDetailTable->setHorizontalHeaderLabels({"队列名", "当前利用率", "峰值利用率", "排队时长", "丢弃包数"});
    m_queueDetailTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_queueDetailTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_queueDetailTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_queueDetailTable->setColumnWidth(3, 100);
    m_queueDetailTable->setColumnWidth(4, 100);
    m_queueDetailTable->verticalHeader()->setVisible(false);
    styleTable(m_queueDetailTable);

    queueLayout->addWidget(queueTitle);
    queueLayout->addWidget(m_queueDetailTable);

    bottomSplitter->addWidget(policyWidget);
    bottomSplitter->addWidget(queueWidget);
    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(bottomSplitter, 1);

    // ── Congestion detection ──
    auto* congestionGroup = new QGroupBox("拥塞检测");
    congestionGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #F56C6C; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* congestionLayout = new QVBoxLayout(congestionGroup);
    congestionLayout->setContentsMargins(4, 8, 4, 4);

    m_congestionText = new QPlainTextEdit();
    m_congestionText->setReadOnly(true);
    m_congestionText->setMaximumBlockCount(200);
    m_congestionText->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1E1F22;"
        "  color: #DCDCDC;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 12px;"
        "  border: 1px solid #3C3F41;"
        "}"
    );
    m_congestionText->setFixedHeight(100);

    congestionLayout->addWidget(m_congestionText);
    mainLayout->addWidget(congestionGroup);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void QoSWidget::setupConnections()
{
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QoSWidget::onDeviceChanged);
    connect(m_interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QoSWidget::onInterfaceChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, &QoSWidget::onRefresh);
    connect(m_exportBtn, &QPushButton::clicked, this, &QoSWidget::onExport);
}

// ─── Device Changed ──────────────────────────────────────────────────────────

void QoSWidget::onDeviceChanged(int index)
{
    Q_UNUSED(index)
    Logger::instance().info("QoS Center",
        QString("设备切换: %1").arg(m_deviceCombo->currentText()));
    analyzeQoS();
}

// ─── Interface Changed ───────────────────────────────────────────────────────

void QoSWidget::onInterfaceChanged(int index)
{
    Q_UNUSED(index)
    Logger::instance().info("QoS Center",
        QString("接口切换: %1").arg(m_interfaceCombo->currentText()));
    analyzeQoS();
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void QoSWidget::onRefresh()
{
    analyzeQoS();
    Logger::instance().info("QoS Center", "手动刷新完成");
}

// ─── Export ──────────────────────────────────────────────────────────────────

void QoSWidget::onExport()
{
    if (m_policyTable->rowCount() == 0 && m_queueDetailTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 QoS 分析报告",
        QString("qos_report_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("QoS Center", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // Header
    stream << "=== QoS 分析报告 ===\n";
    stream << "设备: " << m_deviceCombo->currentText() << "\n";
    stream << "接口: " << m_interfaceCombo->currentText() << "\n";
    stream << "健康评分: " << m_healthScoreLabel->text() << "\n\n";

    // QoS policies
    stream << "=== QoS 策略 ===\n";
    QStringList policyHeaders;
    for (int col = 0; col < m_policyTable->columnCount(); ++col) {
        auto* item = m_policyTable->horizontalHeaderItem(col);
        policyHeaders << (item ? item->text() : QString("列%1").arg(col + 1));
    }
    stream << policyHeaders.join(",") << "\n";

    for (int row = 0; row < m_policyTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_policyTable->columnCount(); ++col) {
            auto* item = m_policyTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // Queue details
    stream << "\n=== 队列详情 ===\n";
    QStringList queueHeaders;
    for (int col = 0; col < m_queueDetailTable->columnCount(); ++col) {
        auto* item = m_queueDetailTable->horizontalHeaderItem(col);
        queueHeaders << (item ? item->text() : QString("列%1").arg(col + 1));
    }
    stream << queueHeaders.join(",") << "\n";

    for (int row = 0; row < m_queueDetailTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_queueDetailTable->columnCount(); ++col) {
            auto* item = m_queueDetailTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // Congestion report
    stream << "\n=== 拥塞检测 ===\n";
    stream << m_congestionText->toPlainText() << "\n";

    file.close();

    int totalRows = m_policyTable->rowCount() + m_queueDetailTable->rowCount();
    Logger::instance().info("QoS Center",
        QString("导出 %1 条记录到: %2").arg(totalRows).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 QoS 分析报告到:\n%1").arg(filePath));
}

// ─── Analyze QoS ─────────────────────────────────────────────────────────────

void QoSWidget::analyzeQoS()
{
    auto* rng = QRandomGenerator::global();

    // ── Simulated DSCP data per interface ──
    int ifaceIdx = m_interfaceCombo->currentIndex();
    struct InterfaceData {
        int ef, af41, af31, af21, be;
        int latency, jitter;
        double packetLoss;
        QStringList queues;
        QList<int> queueUtils;
        QList<int> queuePeaks;
        QList<double> queueTimes;
        QList<int> queueDrops;
    };

    QList<InterfaceData> simData = {
        // GigabitEthernet0/0/0 — normal WAN link
        {15, 20, 25, 15, 25, 8, 2, 0.02,
         {"LLQ-Priority", "CBWFQ-Voice", "CBWFQ-Video", "CBWFQ-Data", "Default"},
         {35, 22, 45, 68, 12},
         {55, 35, 60, 85, 25},
         {0.5, 1.2, 2.8, 8.5, 0.3},
         {0, 0, 12, 45, 0}},
        // GigabitEthernet0/0/1 — lightly loaded
        {10, 15, 20, 25, 30, 3, 1, 0.01,
         {"LLQ-Priority", "CBWFQ-Voice", "CBWFQ-Data", "Default"},
         {15, 18, 40, 8},
         {30, 28, 55, 15},
         {0.2, 0.8, 1.5, 0.1},
         {0, 0, 5, 0}},
        // GigabitEthernet0/0/2 — heavy congestion
        {25, 30, 20, 15, 10, 35, 12, 0.85,
         {"LLQ-Priority", "CBWFQ-Voice", "CBWFQ-Video", "CBWFQ-Data", "Default"},
         {78, 65, 92, 95, 88},
         {85, 72, 98, 100, 95},
         {5.5, 8.2, 25.8, 35.0, 18.5},
         {2, 5, 128, 456, 89}},
        // GigabitEthernet0/0/3 — moderate load
        {12, 18, 22, 20, 28, 12, 4, 0.05,
         {"LLQ-Priority", "CBWFQ-Voice", "CBWFQ-Video", "CBWFQ-Data", "Default"},
         {28, 35, 55, 72, 15},
         {45, 50, 70, 88, 30},
         {0.8, 1.5, 4.2, 10.0, 0.5},
         {0, 0, 8, 22, 0}},
    };

    if (ifaceIdx < 0 || ifaceIdx >= simData.size()) return;

    const auto& data = simData[ifaceIdx];

    // ── Update DSCP chart ──
    m_dscpChart->setData(data.ef, data.af41, data.af31, data.af21, data.be);

    // ── Update queue chart + table ──
    m_queues.clear();
    for (int i = 0; i < data.queues.size(); ++i) {
        QueueInfo qi;
        qi.name = data.queues[i];
        qi.currentUtilization = data.queueUtils[i] + rng->bounded(-5, 6);
        qi.currentUtilization = qBound(0, qi.currentUtilization, 100);
        qi.peakUtilization = data.queuePeaks[i];
        qi.queueTime = data.queueTimes[i] + rng->bounded(-10, 11) / 10.0;
        if (qi.queueTime < 0) qi.queueTime = 0;
        qi.droppedPackets = data.queueDrops[i] + rng->bounded(-3, 4);
        if (qi.droppedPackets < 0) qi.droppedPackets = 0;
        m_queues.append(qi);
    }

    m_queueChart->setQueues(m_queues);

    // Queue detail table
    m_queueDetailTable->setRowCount(0);
    for (const auto& q : m_queues) {
        int row = m_queueDetailTable->rowCount();
        m_queueDetailTable->insertRow(row);

        m_queueDetailTable->setItem(row, 0, new QTableWidgetItem(q.name));

        auto* utilItem = new QTableWidgetItem(QString("%1%").arg(q.currentUtilization));
        if (q.currentUtilization >= 80) {
            utilItem->setForeground(QColor("#F56C6C"));
        } else if (q.currentUtilization >= 60) {
            utilItem->setForeground(QColor("#E6A23C"));
        } else {
            utilItem->setForeground(QColor("#67C23A"));
        }
        m_queueDetailTable->setItem(row, 1, utilItem);

        auto* peakItem = new QTableWidgetItem(QString("%1%").arg(q.peakUtilization));
        if (q.peakUtilization >= 80) {
            peakItem->setForeground(QColor("#F56C6C"));
        } else if (q.peakUtilization >= 60) {
            peakItem->setForeground(QColor("#E6A23C"));
        } else {
            peakItem->setForeground(QColor("#67C23A"));
        }
        m_queueDetailTable->setItem(row, 2, peakItem);

        m_queueDetailTable->setItem(row, 3, new QTableWidgetItem(QString("%1 ms").arg(q.queueTime, 0, 'f', 1)));

        auto* dropItem = new QTableWidgetItem(QString::number(q.droppedPackets));
        if (q.droppedPackets > 50) {
            dropItem->setForeground(QColor("#F56C6C"));
        } else if (q.droppedPackets > 10) {
            dropItem->setForeground(QColor("#E6A23C"));
        }
        m_queueDetailTable->setItem(row, 4, dropItem);
    }

    // ── Update latency / jitter / packet loss ──
    int latency = data.latency + rng->bounded(-2, 3);
    if (latency < 0) latency = 0;
    int jitter = data.jitter + rng->bounded(-1, 2);
    if (jitter < 0) jitter = 0;
    double packetLoss = data.packetLoss + rng->bounded(-5, 6) / 100.0;
    if (packetLoss < 0) packetLoss = 0;

    m_latencyLabel->setText(QString("%1 ms").arg(latency));
    m_jitterLabel->setText(QString("%1 ms").arg(jitter));
    m_packetLossLabel->setText(QString("%1%").arg(packetLoss, 0, 'f', 2));

    if (latency > 20) {
        m_latencyLabel->setStyleSheet("font-size: 20px; color: #F56C6C; font-weight: bold;");
    } else if (latency > 10) {
        m_latencyLabel->setStyleSheet("font-size: 20px; color: #E6A23C; font-weight: bold;");
    } else {
        m_latencyLabel->setStyleSheet("font-size: 20px; color: #67C23A; font-weight: bold;");
    }

    if (jitter > 8) {
        m_jitterLabel->setStyleSheet("font-size: 20px; color: #F56C6C; font-weight: bold;");
    } else if (jitter > 4) {
        m_jitterLabel->setStyleSheet("font-size: 20px; color: #E6A23C; font-weight: bold;");
    } else {
        m_jitterLabel->setStyleSheet("font-size: 20px; color: #67C23A; font-weight: bold;");
    }

    if (packetLoss > 0.5) {
        m_packetLossLabel->setStyleSheet("font-size: 20px; color: #F56C6C; font-weight: bold;");
    } else if (packetLoss > 0.1) {
        m_packetLossLabel->setStyleSheet("font-size: 20px; color: #E6A23C; font-weight: bold;");
    } else {
        m_packetLossLabel->setStyleSheet("font-size: 20px; color: #67C23A; font-weight: bold;");
    }

    // ── QoS Policy table (simulated) ──
    m_policies.clear();
    m_policyTable->setRowCount(0);

    QList<QoSPolicy> simPolicies = {
        {"VOICE-POLICY",    m_interfaceCombo->currentText(), "DSCP EF (46)",    "Priority LLQ 10%",    "出"},
        {"VIDEO-POLICY",    m_interfaceCombo->currentText(), "DSCP AF41 (34)",  "Bandwidth 25%",       "出"},
        {"SIGNALING-POLICY",m_interfaceCombo->currentText(), "DSCP AF31 (26)",  "Bandwidth 15%",       "出"},
        {"DATA-POLICY",     m_interfaceCombo->currentText(), "DSCP AF21 (18)",  "Bandwidth 30%",       "出"},
        {"DEFAULT-POLICY",  m_interfaceCombo->currentText(), "DSCP BE (0)",     "Fair-Queue",          "出"},
        {"INPUT-MARKING",   m_interfaceCombo->currentText(), "ACL-VOICE",       "Set DSCP EF",         "入"},
        {"INPUT-CLASSIFY",  m_interfaceCombo->currentText(), "NBAR-HTTP",       "Set DSCP AF21",       "入"},
    };

    for (const auto& p : simPolicies) {
        m_policies.append(p);
        int row = m_policyTable->rowCount();
        m_policyTable->insertRow(row);
        m_policyTable->setItem(row, 0, new QTableWidgetItem(p.name));
        m_policyTable->setItem(row, 1, new QTableWidgetItem(p.interface));
        m_policyTable->setItem(row, 2, new QTableWidgetItem(p.classifier));
        m_policyTable->setItem(row, 3, new QTableWidgetItem(p.action));
        m_policyTable->setItem(row, 4, new QTableWidgetItem(p.direction));
    }

    // ── Congestion detection ──
    m_congestionText->clear();
    QStringList congestionLines;
    congestionLines << QString("[%1] QoS 拥塞检测 — 设备: %2 接口: %3")
                           .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                m_deviceCombo->currentText(),
                                m_interfaceCombo->currentText());

    bool hasCongestion = false;
    for (const auto& q : m_queues) {
        if (q.currentUtilization >= 80) {
            congestionLines << QString("  ⚠ 警告: 队列 [%1] 利用率 %2% 超过阈值 80%")
                                   .arg(q.name).arg(q.currentUtilization);
            hasCongestion = true;
        }
        if (q.droppedPackets > 50) {
            congestionLines << QString("  ⚠ 警告: 队列 [%1] 丢弃 %2 个包")
                                   .arg(q.name).arg(q.droppedPackets);
            hasCongestion = true;
        }
        if (q.queueTime > 10.0) {
            congestionLines << QString("  ⚠ 警告: 队列 [%1] 排队时长 %2 ms 过高")
                                   .arg(q.name).arg(q.queueTime, 0, 'f', 1);
            hasCongestion = true;
        }
    }

    if (packetLoss > 0.5) {
        congestionLines << QString("  ⚠ 警告: 丢包率 %1% 超过阈值 0.5%").arg(packetLoss, 0, 'f', 2);
        hasCongestion = true;
    }
    if (latency > 20) {
        congestionLines << QString("  ⚠ 警告: 时延 %1 ms 超过阈值 20 ms").arg(latency);
        hasCongestion = true;
    }

    if (!hasCongestion) {
        congestionLines << "  ✅ 未检测到拥塞，所有队列运行正常。";
    }

    for (const auto& line : congestionLines) {
        m_congestionText->appendPlainText(line);
    }

    // ── Health score ──
    int score = calculateHealthScore();
    m_healthScoreLabel->setText(QString::number(score));

    if (score >= 80) {
        m_healthScoreLabel->setStyleSheet("font-size: 28px; color: #67C23A; font-weight: bold;");
    } else if (score >= 60) {
        m_healthScoreLabel->setStyleSheet("font-size: 28px; color: #E6A23C; font-weight: bold;");
    } else {
        m_healthScoreLabel->setStyleSheet("font-size: 28px; color: #F56C6C; font-weight: bold;");
    }
}

// ─── Calculate Health Score ──────────────────────────────────────────────────

int QoSWidget::calculateHealthScore()
{
    int score = 0;

    // ── DSCP 分类 (30分) ──
    // 理想情况: EF 10-20%, AF 占主要比例, BE 占比不高
    int ef = 0, af41 = 0, af31 = 0, af21 = 0, be = 0;
    int ifaceIdx = m_interfaceCombo->currentIndex();
    QList<QList<int>> dscpData = {
        {15, 20, 25, 15, 25},
        {10, 15, 20, 25, 30},
        {25, 30, 20, 15, 10},
        {12, 18, 22, 20, 28},
    };
    if (ifaceIdx >= 0 && ifaceIdx < dscpData.size()) {
        ef = dscpData[ifaceIdx][0];
        af41 = dscpData[ifaceIdx][1];
        af31 = dscpData[ifaceIdx][2];
        af21 = dscpData[ifaceIdx][3];
        be = dscpData[ifaceIdx][4];
    }

    int total = ef + af41 + af31 + af21 + be;
    if (total == 0) total = 1;

    // EF should not exceed 30%
    if (ef * 100 / total <= 30) score += 10;
    else if (ef * 100 / total <= 40) score += 5;

    // AF classes should be well-distributed
    int afTotal = af41 + af31 + af21;
    if (afTotal * 100 / total >= 40) score += 10;
    else if (afTotal * 100 / total >= 25) score += 5;

    // BE should not dominate
    if (be * 100 / total <= 30) score += 10;
    else if (be * 100 / total <= 50) score += 5;

    // ── 队列利用率 (30分) ──
    int queueScore = 30;
    for (const auto& q : m_queues) {
        if (q.currentUtilization >= 90) queueScore -= 10;
        else if (q.currentUtilization >= 80) queueScore -= 6;
        else if (q.currentUtilization >= 60) queueScore -= 2;
    }
    if (queueScore < 0) queueScore = 0;
    score += queueScore;

    // ── 时延 (20分) ──
    QString latencyStr = m_latencyLabel->text();
    latencyStr.remove(" ms");
    int latency = latencyStr.toInt();
    if (latency <= 10) score += 20;
    else if (latency <= 20) score += 15;
    else if (latency <= 30) score += 8;
    else if (latency <= 50) score += 3;

    // ── 丢包率 (20分) ──
    QString lossStr = m_packetLossLabel->text();
    lossStr.remove("%");
    double loss = lossStr.toDouble();
    if (loss <= 0.05) score += 20;
    else if (loss <= 0.1) score += 15;
    else if (loss <= 0.5) score += 8;
    else if (loss <= 1.0) score += 3;

    return qBound(0, score, 100);
}

// ─── Chart Painting (delegated to child widgets) ─────────────────────────────

void QoSWidget::paintDSCPChart()
{
    // Chart painting is handled by DscpChartWidget::paintEvent
}

void QoSWidget::paintQueueChart()
{
    // Chart painting is handled by QueueChartWidget::paintEvent
}