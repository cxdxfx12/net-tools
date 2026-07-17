#include "alarm/AlarmCenterWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QDateTime>
#include <QUuid>

// ============================================================================
// 构造 / 析构
// ============================================================================
AlarmCenterWidget::AlarmCenterWidget(QWidget* parent)
    : QWidget(parent)
    , m_totalLabel(nullptr)
    , m_unacknowledgedLabel(nullptr)
    , m_criticalLabel(nullptr)
    , m_emergencyLabel(nullptr)
    , m_levelFilter(nullptr)
    , m_sourceFilter(nullptr)
    , m_deviceFilter(nullptr)
    , m_statusFilter(nullptr)
    , m_searchEdit(nullptr)
    , m_applyFilterBtn(nullptr)
    , m_clearFilterBtn(nullptr)
    , m_alarmTable(nullptr)
    , m_ackBtn(nullptr)
    , m_recoverBtn(nullptr)
    , m_ackAllBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_detailView(nullptr)
{
    setupUI();
    setupConnections();

    // 预置示例告警数据
    addAlarm("Emergency", "SNMP",    "Core-Switch-01", "设备宕机: 主核心交换机无响应超过 5 分钟");
    addAlarm("Critical",  "Ping",    "Router-GW-01",   "连通性丢失: 网关路由器 ICMP 超时，丢包率 100%");
    addAlarm("Critical",  "Syslog",  "FW-Cluster-01",  "安全事件: 检测到异常登录尝试，来源 IP 10.0.0.99");
    addAlarm("Warning",   "SNMP",    "Agg-Switch-03",  "CPU 使用率过高: 当前 92%，阈值 85%");
    addAlarm("Warning",   "SSH",     "Core-Router-02", "配置变更: 接口 GigabitEthernet0/0/1 状态变更");
    addAlarm("Warning",   "HTTP",    "AP-Controller-01","AP 离线: AP-3F-12 信号丢失，最后在线 30 分钟前");
    addAlarm("Info",      "Ping",    "Branch-Router-05","延迟升高: 分支路由器延迟从 5ms 升至 45ms");
    addAlarm("Info",      "Syslog",   "Core-Switch-02", "端口状态: GigabitEthernet1/0/24 链路 Up");
    addAlarm("Emergency", "SNMP",    "DC-Power-01",    "电源故障: 数据中心电源模块 A 输出电压异常");
    addAlarm("Critical",  "SNMP",    "Storage-Array-01","存储告警: RAID 阵列降级，磁盘槽位 3 故障");

    Logger::instance().info("AlarmCenter", "告警中心初始化完成");
}

// ============================================================================
// UI 构建
// ============================================================================
void AlarmCenterWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // ─── 顶部统计栏 ────────────────────────────────────────────────
    auto* statsGroup = new QGroupBox("告警统计");
    auto* statsLayout = new QHBoxLayout(statsGroup);

    auto makeStatLabel = [](const QString& title, const QString& color) -> QLabel* {
        auto* label = new QLabel(QString("%1: 0").arg(title));
        label->setStyleSheet(
            QString("font-size: 16px; font-weight: bold; color: %1; "
                    "background-color: #25262B; border-radius: 8px; "
                    "padding: 10px 16px;").arg(color));
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumWidth(140);
        return label;
    };

    m_totalLabel          = makeStatLabel("总数", "#DCDCDC");
    m_unacknowledgedLabel = makeStatLabel("未确认", "#409EFF");
    m_criticalLabel       = makeStatLabel("严重", "#E6A23C");
    m_emergencyLabel      = makeStatLabel("紧急", "#F56C6C");

    statsLayout->addWidget(m_totalLabel);
    statsLayout->addWidget(m_unacknowledgedLabel);
    statsLayout->addWidget(m_criticalLabel);
    statsLayout->addWidget(m_emergencyLabel);
    statsLayout->addStretch();

    mainLayout->addWidget(statsGroup);

    // ─── 中部过滤区 ────────────────────────────────────────────────
    auto* filterGroup = new QGroupBox("过滤条件");
    auto* filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("级别:"));
    m_levelFilter = new QComboBox();
    m_levelFilter->addItems({"全部", "Info", "Warning", "Critical", "Emergency"});
    filterLayout->addWidget(m_levelFilter);

    filterLayout->addWidget(new QLabel("来源:"));
    m_sourceFilter = new QComboBox();
    m_sourceFilter->addItems({"全部", "Ping", "SNMP", "Syslog", "SSH", "HTTP"});
    filterLayout->addWidget(m_sourceFilter);

    filterLayout->addWidget(new QLabel("设备:"));
    m_deviceFilter = new QComboBox();
    m_deviceFilter->addItem("全部");
    m_deviceFilter->setMinimumWidth(120);
    filterLayout->addWidget(m_deviceFilter);

    filterLayout->addWidget(new QLabel("状态:"));
    m_statusFilter = new QComboBox();
    m_statusFilter->addItems({"全部", "Unacknowledged", "Acknowledged", "Recovered"});
    filterLayout->addWidget(m_statusFilter);

    filterLayout->addWidget(new QLabel("搜索:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索告警消息或设备...");
    m_searchEdit->setMinimumWidth(150);
    filterLayout->addWidget(m_searchEdit);

    m_applyFilterBtn = new QPushButton("应用过滤");
    filterLayout->addWidget(m_applyFilterBtn);

    m_clearFilterBtn = new QPushButton("清除");
    m_clearFilterBtn->setStyleSheet(
        "QPushButton { background-color: #4A4D52; color: #DCDCDC; }"
        "QPushButton:hover { background-color: #5A5D62; }");
    filterLayout->addWidget(m_clearFilterBtn);

    filterLayout->addStretch();
    mainLayout->addWidget(filterGroup);

    // ─── 操作按钮栏 ────────────────────────────────────────────────
    auto* actionLayout = new QHBoxLayout();

    m_ackBtn = new QPushButton("确认告警");
    actionLayout->addWidget(m_ackBtn);

    m_recoverBtn = new QPushButton("恢复告警");
    m_recoverBtn->setStyleSheet(
        "QPushButton { background-color: #67C23A; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #85CE61; }");
    actionLayout->addWidget(m_recoverBtn);

    m_ackAllBtn = new QPushButton("全部确认");
    m_ackAllBtn->setStyleSheet(
        "QPushButton { background-color: #409EFF; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #66B1FF; }");
    actionLayout->addWidget(m_ackAllBtn);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton { background-color: #4A4D52; color: #DCDCDC; }"
        "QPushButton:hover { background-color: #5A5D62; }");
    actionLayout->addWidget(m_exportBtn);

    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // ─── 底部：表格 + 详情面板 (Splitter) ───────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // 告警表格
    m_alarmTable = new QTableWidget();
    m_alarmTable->setColumnCount(8);
    QStringList headers = {"时间", "级别", "来源", "设备", "消息", "状态", "确认人", "ID"};
    m_alarmTable->setHorizontalHeaderLabels(headers);
    m_alarmTable->horizontalHeader()->setStretchLastSection(true);
    m_alarmTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_alarmTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alarmTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alarmTable->setAlternatingRowColors(true);
    m_alarmTable->verticalHeader()->setVisible(false);
    m_alarmTable->setSortingEnabled(false);

    m_alarmTable->setColumnWidth(0, 150); // 时间
    m_alarmTable->setColumnWidth(1, 80);  // 级别
    m_alarmTable->setColumnWidth(2, 70);  // 来源
    m_alarmTable->setColumnWidth(3, 130); // 设备
    m_alarmTable->setColumnWidth(4, 300); // 消息
    m_alarmTable->setColumnWidth(5, 100); // 状态
    m_alarmTable->setColumnWidth(6, 80);  // 确认人
    m_alarmTable->setColumnWidth(7, 0);   // ID (隐藏)

    splitter->addWidget(m_alarmTable);

    // 详情面板
    m_detailView = new QPlainTextEdit();
    m_detailView->setReadOnly(true);
    m_detailView->setPlaceholderText("选择一条告警查看详情...");
    m_detailView->setMaximumBlockCount(1000);

    splitter->addWidget(m_detailView);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

// ============================================================================
// 信号槽连接
// ============================================================================
void AlarmCenterWidget::setupConnections()
{
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onApplyFilter);
    connect(m_clearFilterBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onClearFilter);
    connect(m_ackBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onAcknowledge);
    connect(m_recoverBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onRecover);
    connect(m_ackAllBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onAcknowledgeAll);
    connect(m_exportBtn, &QPushButton::clicked, this, &AlarmCenterWidget::onExport);
    connect(m_alarmTable, &QTableWidget::itemSelectionChanged, this, &AlarmCenterWidget::onAlarmSelected);
}

// ============================================================================
// 级别 → 颜色映射
// ============================================================================
QString AlarmCenterWidget::levelToColor(const QString& level) const
{
    if (level == "Emergency") return "#F56C6C";
    if (level == "Critical")  return "#E6A23C";
    if (level == "Warning")   return "#E6DB5C";
    if (level == "Info")      return "#409EFF";
    return "#DCDCDC";
}

// ============================================================================
// 添加告警
// ============================================================================
void AlarmCenterWidget::addAlarm(const QString& level, const QString& source,
                                  const QString& device, const QString& message)
{
    AlarmEntry entry;
    entry.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.time      = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    entry.level     = level;
    entry.source    = source;
    entry.device    = device;
    entry.message   = message;
    entry.status    = "Unacknowledged";
    entry.acknowledgedBy.clear();

    m_alarms.append(entry);

    // 更新设备过滤下拉框
    bool deviceExists = false;
    for (int i = 0; i < m_deviceFilter->count(); ++i) {
        if (m_deviceFilter->itemText(i) == device) {
            deviceExists = true;
            break;
        }
    }
    if (!deviceExists) {
        m_deviceFilter->addItem(device);
    }

    updateStatistics();
    applyFilters();

    Logger::instance().info("AlarmCenter",
        QString("新增告警 [%1] %2: %3").arg(level, device, message));
}

// ============================================================================
// 应用过滤 (AND 逻辑)
// ============================================================================
void AlarmCenterWidget::applyFilters()
{
    QString levelFilter  = m_levelFilter->currentText();
    QString sourceFilter = m_sourceFilter->currentText();
    QString deviceFilter = m_deviceFilter->currentText();
    QString statusFilter = m_statusFilter->currentText();
    QString searchText   = m_searchEdit->text().trimmed();

    m_alarmTable->setRowCount(0);

    for (const AlarmEntry& alarm : m_alarms) {
        bool visible = true;

        if (levelFilter != "全部" && alarm.level != levelFilter)
            visible = false;
        if (sourceFilter != "全部" && alarm.source != sourceFilter)
            visible = false;
        if (deviceFilter != "全部" && alarm.device != deviceFilter)
            visible = false;
        if (statusFilter != "全部" && alarm.status != statusFilter)
            visible = false;
        if (!searchText.isEmpty()) {
            if (!alarm.message.contains(searchText, Qt::CaseInsensitive) &&
                !alarm.device.contains(searchText, Qt::CaseInsensitive) &&
                !alarm.source.contains(searchText, Qt::CaseInsensitive)) {
                visible = false;
            }
        }

        if (!visible) continue;

        int row = m_alarmTable->rowCount();
        m_alarmTable->insertRow(row);

        QColor levelColor(levelToColor(alarm.level));

        // 已确认的行用灰色
        bool isAcknowledged = (alarm.status == "Acknowledged" || alarm.status == "Recovered");
        QColor rowColor = isAcknowledged ? QColor("#8C8C8C") : levelColor;

        // 时间
        auto* timeItem = new QTableWidgetItem(alarm.time);
        timeItem->setForeground(rowColor);
        m_alarmTable->setItem(row, 0, timeItem);

        // 级别
        auto* levelItem = new QTableWidgetItem(alarm.level);
        levelItem->setForeground(levelColor);
        levelItem->setTextAlignment(Qt::AlignCenter);
        QFont boldFont = levelItem->font();
        boldFont.setBold(true);
        levelItem->setFont(boldFont);
        m_alarmTable->setItem(row, 1, levelItem);

        // 来源
        auto* sourceItem = new QTableWidgetItem(alarm.source);
        sourceItem->setForeground(rowColor);
        sourceItem->setTextAlignment(Qt::AlignCenter);
        m_alarmTable->setItem(row, 2, sourceItem);

        // 设备
        auto* deviceItem = new QTableWidgetItem(alarm.device);
        deviceItem->setForeground(rowColor);
        m_alarmTable->setItem(row, 3, deviceItem);

        // 消息
        auto* msgItem = new QTableWidgetItem(alarm.message);
        msgItem->setForeground(rowColor);
        m_alarmTable->setItem(row, 4, msgItem);

        // 状态
        auto* statusItem = new QTableWidgetItem(alarm.status);
        statusItem->setForeground(rowColor);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_alarmTable->setItem(row, 5, statusItem);

        // 确认人
        auto* ackItem = new QTableWidgetItem(alarm.acknowledgedBy);
        ackItem->setForeground(rowColor);
        ackItem->setTextAlignment(Qt::AlignCenter);
        m_alarmTable->setItem(row, 6, ackItem);

        // ID (隐藏)
        auto* idItem = new QTableWidgetItem(alarm.id);
        m_alarmTable->setItem(row, 7, idItem);
    }
}

// ============================================================================
// 过滤槽函数
// ============================================================================
void AlarmCenterWidget::onApplyFilter()
{
    applyFilters();
    Logger::instance().info("AlarmCenter", "过滤条件已应用");
}

void AlarmCenterWidget::onClearFilter()
{
    m_levelFilter->setCurrentIndex(0);
    m_sourceFilter->setCurrentIndex(0);
    m_deviceFilter->setCurrentIndex(0);
    m_statusFilter->setCurrentIndex(0);
    m_searchEdit->clear();
    applyFilters();
    Logger::instance().info("AlarmCenter", "过滤条件已清除");
}

// ============================================================================
// 确认告警
// ============================================================================
void AlarmCenterWidget::onAcknowledge()
{
    QList<QTableWidgetItem*> selected = m_alarmTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "确认告警", "请先选择一条告警。");
        return;
    }

    int row = selected.first()->row();
    QTableWidgetItem* idItem = m_alarmTable->item(row, 7);
    if (!idItem) return;

    QString alarmId = idItem->text();

    for (AlarmEntry& alarm : m_alarms) {
        if (alarm.id == alarmId && alarm.status == "Unacknowledged") {
            alarm.status = "Acknowledged";
            alarm.acknowledgedBy = "Admin";
            Logger::instance().info("AlarmCenter",
                QString("告警已确认: %1 [%2]").arg(alarm.device, alarm.message));
            break;
        }
    }

    updateStatistics();
    applyFilters();
}

// ============================================================================
// 恢复告警
// ============================================================================
void AlarmCenterWidget::onRecover()
{
    QList<QTableWidgetItem*> selected = m_alarmTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "恢复告警", "请先选择一条告警。");
        return;
    }

    int row = selected.first()->row();
    QTableWidgetItem* idItem = m_alarmTable->item(row, 7);
    if (!idItem) return;

    QString alarmId = idItem->text();

    for (AlarmEntry& alarm : m_alarms) {
        if (alarm.id == alarmId) {
            alarm.status = "Recovered";
            Logger::instance().info("AlarmCenter",
                QString("告警已恢复: %1 [%2]").arg(alarm.device, alarm.message));
            break;
        }
    }

    updateStatistics();
    applyFilters();
}

// ============================================================================
// 全部确认
// ============================================================================
void AlarmCenterWidget::onAcknowledgeAll()
{
    int count = 0;
    for (AlarmEntry& alarm : m_alarms) {
        if (alarm.status == "Unacknowledged") {
            alarm.status = "Acknowledged";
            alarm.acknowledgedBy = "Admin";
            ++count;
        }
    }

    Logger::instance().info("AlarmCenter",
        QString("已确认 %1 条告警").arg(count));

    updateStatistics();
    applyFilters();
}

// ============================================================================
// 导出 CSV
// ============================================================================
void AlarmCenterWidget::onExport()
{
    if (m_alarms.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出告警 CSV",
        "alarm_export.csv", "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    // BOM for Excel UTF-8
    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "时间,级别,来源,设备,消息,状态,确认人,ID\n";

    auto csvEscape = [](const QString& val) -> QString {
        QString v = val;
        if (v.contains(',') || v.contains('\n') || v.contains('"')) {
            v.replace("\"", "\"\"");
            v = "\"" + v + "\"";
        }
        return v;
    };

    for (const AlarmEntry& alarm : m_alarms) {
        out << csvEscape(alarm.time) << ","
            << csvEscape(alarm.level) << ","
            << csvEscape(alarm.source) << ","
            << csvEscape(alarm.device) << ","
            << csvEscape(alarm.message) << ","
            << csvEscape(alarm.status) << ","
            << csvEscape(alarm.acknowledgedBy) << ","
            << csvEscape(alarm.id) << "\n";
    }

    file.close();
    Logger::instance().info("AlarmCenter",
        QString("告警已导出到: %1 (%2 条记录)").arg(filePath).arg(m_alarms.size()));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条告警记录到:\n%2").arg(m_alarms.size()).arg(filePath));
}

// ============================================================================
// 选中告警 → 详情面板
// ============================================================================
void AlarmCenterWidget::onAlarmSelected()
{
    QList<QTableWidgetItem*> selected = m_alarmTable->selectedItems();
    if (selected.isEmpty()) {
        m_detailView->clear();
        return;
    }

    int row = selected.first()->row();
    QTableWidgetItem* idItem = m_alarmTable->item(row, 7);
    if (!idItem) return;

    QString alarmId = idItem->text();

    for (const AlarmEntry& alarm : m_alarms) {
        if (alarm.id == alarmId) {
            QString detail;
            detail += "══════════════════════════════════════════\n";
            detail += QString("  告警 ID:     %1\n").arg(alarm.id);
            detail += QString("  时间:        %1\n").arg(alarm.time);
            detail += QString("  级别:        %1\n").arg(alarm.level);
            detail += QString("  来源:        %1\n").arg(alarm.source);
            detail += QString("  设备:        %1\n").arg(alarm.device);
            detail += QString("  状态:        %1\n").arg(alarm.status);
            if (!alarm.acknowledgedBy.isEmpty()) {
                detail += QString("  确认人:      %1\n").arg(alarm.acknowledgedBy);
            }
            detail += "──────────────────────────────────────────\n";
            detail += QString("  消息:\n  %1\n").arg(alarm.message);
            detail += "══════════════════════════════════════════\n";

            m_detailView->setPlainText(detail);
            return;
        }
    }
}

// ============================================================================
// 更新统计
// ============================================================================
void AlarmCenterWidget::updateStatistics()
{
    int total = m_alarms.size();
    int unacknowledged = 0;
    int critical = 0;
    int emergency = 0;

    for (const AlarmEntry& alarm : m_alarms) {
        if (alarm.status == "Unacknowledged")
            ++unacknowledged;
        if (alarm.level == "Critical")
            ++critical;
        if (alarm.level == "Emergency")
            ++emergency;
    }

    m_totalLabel->setText(QString("总数: %1").arg(total));
    m_unacknowledgedLabel->setText(QString("未确认: %1").arg(unacknowledged));
    m_criticalLabel->setText(QString("严重: %1").arg(critical));
    m_emergencyLabel->setText(QString("紧急: %1").arg(emergency));
}