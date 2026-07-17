#include "ha/HAWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTimer>
#include <QTabWidget>
#include <QFrame>
#include <QDateTime>
#include <QApplication>
#include <QRandomGenerator>
#include <QScrollArea>

// ═══════════════════════════════════════════════════════════════════════════════
// HAWidget — 高可用性管理 (第59章)
// ═══════════════════════════════════════════════════════════════════════════════

HAWidget::HAWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();
    generateMockHAData();

    // 默认选中第一个设备
    if (!m_haConfigs.isEmpty()) {
        m_deviceCombo->setCurrentIndex(0);
        updateHAStatus(0);
    }

    Logger::instance().info(QStringLiteral("HA Management"),
        QStringLiteral("HA 高可用性管理模块已初始化"));
}

HAWidget::~HAWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── Mock Data ───────────────────────────────────────────────────────────────

void HAWidget::generateMockHAData()
{
    // ── HA 组 1: ASA 防火墙 HA ──
    HAConfig asaHa;
    asaHa.groupName = QStringLiteral("ASA-FW-HA");
    asaHa.role = QStringLiteral("Active");
    asaHa.priority = 200;
    asaHa.status = QStringLiteral("Active");
    asaHa.peer = QStringLiteral("ASA-FW-02");
    asaHa.heartbeatInterval = 200;
    asaHa.haType = QStringLiteral("ASA-HA");
    m_haConfigs.append(asaHa);

    QList<HAInterface> asaIfs;
    asaIfs.append({QStringLiteral("GigabitEthernet0/0"), QStringLiteral("192.168.1.1/24"), QStringLiteral("00:50:56:AA:01:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    asaIfs.append({QStringLiteral("GigabitEthernet0/1"), QStringLiteral("10.0.0.1/24"), QStringLiteral("00:50:56:AA:01:02"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    asaIfs.append({QStringLiteral("GigabitEthernet0/2"), QStringLiteral("172.16.0.1/24"), QStringLiteral("00:50:56:AA:01:03"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    asaIfs.append({QStringLiteral("Management0/0"), QStringLiteral("192.168.100.1/24"), QStringLiteral("00:50:56:AA:01:04"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    asaIfs.append({QStringLiteral("HA-Link"), QStringLiteral("169.254.1.1/30"), QStringLiteral("00:50:56:AA:01:99"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    m_haInterfaces.append(asaIfs);

    QList<SwitchLog> asaLogs;
    asaLogs.append({QStringLiteral("2026-07-15 14:32:18"), QStringLiteral("主防火墙故障切换"), QStringLiteral("ASA-FW-01"), QStringLiteral("ASA-FW-02"), 350});
    asaLogs.append({QStringLiteral("2026-07-10 09:15:42"), QStringLiteral("计划内维护切换"), QStringLiteral("ASA-FW-02"), QStringLiteral("ASA-FW-01"), 280});
    asaLogs.append({QStringLiteral("2026-06-28 22:08:33"), QStringLiteral("HA链路中断触发切换"), QStringLiteral("ASA-FW-01"), QStringLiteral("ASA-FW-02"), 420});
    asaLogs.append({QStringLiteral("2026-06-15 03:11:05"), QStringLiteral("电源故障自动切换"), QStringLiteral("ASA-FW-02"), QStringLiteral("ASA-FW-01"), 390});
    m_haSwitchLogs.append(asaLogs);

    // ── HA 组 2: 核心交换机 HSRP ──
    HAConfig hsrp;
    hsrp.groupName = QStringLiteral("Core-HSRP-Group10");
    hsrp.role = QStringLiteral("Master");
    hsrp.priority = 120;
    hsrp.status = QStringLiteral("Active");
    hsrp.peer = QStringLiteral("Core-SW-02");
    hsrp.heartbeatInterval = 3000;
    hsrp.haType = QStringLiteral("HSRP");
    m_haConfigs.append(hsrp);

    QList<HAInterface> hsrpIfs;
    hsrpIfs.append({QStringLiteral("Vlan10"), QStringLiteral("10.10.10.254/24"), QStringLiteral("00:1A:2B:CC:10:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    hsrpIfs.append({QStringLiteral("Vlan20"), QStringLiteral("10.10.20.254/24"), QStringLiteral("00:1A:2B:CC:20:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    hsrpIfs.append({QStringLiteral("Vlan30"), QStringLiteral("10.10.30.254/24"), QStringLiteral("00:1A:2B:CC:30:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    hsrpIfs.append({QStringLiteral("Vlan40"), QStringLiteral("10.10.40.254/24"), QStringLiteral("00:1A:2B:CC:40:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    hsrpIfs.append({QStringLiteral("TenGigabitEthernet1/1"), QStringLiteral("10.0.0.10/30"), QStringLiteral("00:1A:2B:CC:00:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    m_haInterfaces.append(hsrpIfs);

    QList<SwitchLog> hsrpLogs;
    hsrpLogs.append({QStringLiteral("2026-07-14 08:00:10"), QStringLiteral("Core-SW-01 上行链路故障"), QStringLiteral("Core-SW-01"), QStringLiteral("Core-SW-02"), 180});
    hsrpLogs.append({QStringLiteral("2026-07-05 16:45:00"), QStringLiteral("计划内升级切换"), QStringLiteral("Core-SW-02"), QStringLiteral("Core-SW-01"), 250});
    hsrpLogs.append({QStringLiteral("2026-06-20 11:30:22"), QStringLiteral("CPU 过载触发切换"), QStringLiteral("Core-SW-01"), QStringLiteral("Core-SW-02"), 310});
    m_haSwitchLogs.append(hsrpLogs);

    // ── HA 组 3: VRRP 路由器组 ──
    HAConfig vrrp;
    vrrp.groupName = QStringLiteral("VRRP-Group1");
    vrrp.role = QStringLiteral("Master");
    vrrp.priority = 110;
    vrrp.status = QStringLiteral("Active");
    vrrp.peer = QStringLiteral("Router-02");
    vrrp.heartbeatInterval = 1000;
    vrrp.haType = QStringLiteral("VRRP");
    m_haConfigs.append(vrrp);

    QList<HAInterface> vrrpIfs;
    vrrpIfs.append({QStringLiteral("GigabitEthernet0/0/0"), QStringLiteral("192.168.0.1/24"), QStringLiteral("00:25:9E:DD:01:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    vrrpIfs.append({QStringLiteral("GigabitEthernet0/0/1"), QStringLiteral("192.168.1.1/24"), QStringLiteral("00:25:9E:DD:01:02"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    vrrpIfs.append({QStringLiteral("GigabitEthernet0/0/2"), QStringLiteral("172.20.0.1/24"), QStringLiteral("00:25:9E:DD:01:03"), QStringLiteral("Down"), QStringLiteral("Link Down")});
    vrrpIfs.append({QStringLiteral("Loopback0"), QStringLiteral("10.255.255.1/32"), QStringLiteral("N/A"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    m_haInterfaces.append(vrrpIfs);

    QList<SwitchLog> vrrpLogs;
    vrrpLogs.append({QStringLiteral("2026-07-16 06:22:15"), QStringLiteral("主路由器接口Down"), QStringLiteral("Router-01"), QStringLiteral("Router-02"), 220});
    vrrpLogs.append({QStringLiteral("2026-07-08 12:10:33"), QStringLiteral("手动切换测试"), QStringLiteral("Router-02"), QStringLiteral("Router-01"), 150});
    m_haSwitchLogs.append(vrrpLogs);

    // ── HA 组 4: 堆叠交换机 ──
    HAConfig stack;
    stack.groupName = QStringLiteral("Stack-Group1");
    stack.role = QStringLiteral("Master");
    stack.priority = 200;
    stack.status = QStringLiteral("Active");
    stack.peer = QStringLiteral("Stack-Member-02");
    stack.heartbeatInterval = 200;
    stack.haType = QStringLiteral("Stack");
    m_haConfigs.append(stack);

    QList<HAInterface> stackIfs;
    stackIfs.append({QStringLiteral("Stack-Port1/1"), QStringLiteral("N/A"), QStringLiteral("N/A"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    stackIfs.append({QStringLiteral("Stack-Port1/2"), QStringLiteral("N/A"), QStringLiteral("N/A"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    stackIfs.append({QStringLiteral("TenGigabitEthernet1/0/1"), QStringLiteral("10.0.0.1/30"), QStringLiteral("00:1E:F6:EE:01:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    stackIfs.append({QStringLiteral("TenGigabitEthernet1/0/2"), QStringLiteral("10.0.0.5/30"), QStringLiteral("00:1E:F6:EE:01:02"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    stackIfs.append({QStringLiteral("TenGigabitEthernet2/0/1"), QStringLiteral("10.0.0.9/30"), QStringLiteral("00:1E:F6:EE:02:01"), QStringLiteral("Up"), QStringLiteral("Link Up")});
    m_haInterfaces.append(stackIfs);

    QList<SwitchLog> stackLogs;
    stackLogs.append({QStringLiteral("2026-07-12 23:59:01"), QStringLiteral("堆叠成员2重启"), QStringLiteral("Stack-Master"), QStringLiteral("Stack-Member-02"), 5200});
    stackLogs.append({QStringLiteral("2026-06-30 04:15:00"), QStringLiteral("堆叠分裂恢复"), QStringLiteral("Stack-Member-02"), QStringLiteral("Stack-Master"), 4800});
    stackLogs.append({QStringLiteral("2026-06-10 18:30:45"), QStringLiteral("堆叠线缆故障"), QStringLiteral("Stack-Master"), QStringLiteral("Stack-Master"), 0});
    m_haSwitchLogs.append(stackLogs);
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void HAWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──
    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
            QStringLiteral(
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
            )
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
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

    // ── Top: Device selector + HA Overview ──
    auto* topArea = new QWidget();
    auto* topLayout = new QVBoxLayout(topArea);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    // Device selector row
    auto* selectorRow = new QHBoxLayout();
    auto* selectorLabel = new QLabel(QStringLiteral("选择设备:"));
    selectorLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #8C8C8C; font-weight: bold;"));
    selectorLabel->setFixedWidth(70);

    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(320);
    m_deviceCombo->setStyleSheet(
        QStringLiteral(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 6px 12px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; width: 24px; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        )
    );

    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    styleButton(refreshBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));

    selectorRow->addWidget(selectorLabel);
    selectorRow->addWidget(m_deviceCombo);
    selectorRow->addStretch();
    selectorRow->addWidget(refreshBtn);
    topLayout->addLayout(selectorRow);

    // HA Overview cards (5 status cards)
    auto* cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(10);

    auto createCard = [](const QString& title, QLabel*& valueLabel, const QString& valueColor) -> QFrame* {
        auto* card = new QFrame();
        card->setStyleSheet(
            QStringLiteral(
                "QFrame {"
                "  background-color: #25262B;"
                "  border: 1px solid #3C3F41;"
                "  border-radius: 6px;"
                "}"
            )
        );
        card->setFixedHeight(80);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 8, 12, 8);
        cardLayout->setSpacing(4);

        auto* titleLbl = new QLabel(title);
        titleLbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #8C8C8C;"));
        titleLbl->setAlignment(Qt::AlignCenter);

        valueLabel = new QLabel(QStringLiteral("--"));
        valueLabel->setStyleSheet(
            QString(QStringLiteral("font-size: 18px; color: %1; font-weight: bold;")).arg(valueColor)
        );
        valueLabel->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(titleLbl);
        cardLayout->addWidget(valueLabel);
        return card;
    };

    cardsRow->addWidget(createCard(QStringLiteral("HA 状态"), m_haStatusLabel, QStringLiteral("#67C23A")));
    cardsRow->addWidget(createCard(QStringLiteral("主设备"), m_primaryDeviceLabel, QStringLiteral("#409EFF")));
    cardsRow->addWidget(createCard(QStringLiteral("备设备"), m_standbyDeviceLabel, QStringLiteral("#E6A23C")));
    cardsRow->addWidget(createCard(QStringLiteral("切换时间"), m_switchTimeLabel, QStringLiteral("#8C8C8C")));
    cardsRow->addWidget(createCard(QStringLiteral("上次切换"), m_lastSwitchLabel, QStringLiteral("#8C8C8C")));

    topLayout->addLayout(cardsRow);
    mainLayout->addWidget(topArea);

    // ── Middle: Tab widget ──
    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        QStringLiteral(
            "QTabWidget::pane {"
            "  background-color: #1E1F22;"
            "  border: 1px solid #3C3F41;"
            "}"
            "QTabBar::tab {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  padding: 8px 20px; border: 1px solid #3C3F41;"
            "  border-bottom: none; font-size: 13px;"
            "  border-top-left-radius: 4px; border-top-right-radius: 4px;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: #1E1F22; color: #409EFF;"
            "  font-weight: bold;"
            "}"
            "QTabBar::tab:hover {"
            "  color: #DCDCDC;"
            "}"
        )
    );

    // ── Tab 1: HA 信息 ──
    auto* haInfoTab = new QWidget();
    auto* haInfoLayout = new QVBoxLayout(haInfoTab);
    haInfoLayout->setContentsMargins(4, 8, 4, 4);

    m_haInfoTable = new QTableWidget(0, 6);
    m_haInfoTable->setHorizontalHeaderLabels({
        QStringLiteral("HA 组名"),
        QStringLiteral("角色"),
        QStringLiteral("优先级"),
        QStringLiteral("状态"),
        QStringLiteral("对端设备"),
        QStringLiteral("心跳间隔")
    });
    m_haInfoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_haInfoTable->setColumnWidth(1, 80);
    m_haInfoTable->setColumnWidth(2, 70);
    m_haInfoTable->setColumnWidth(3, 80);
    m_haInfoTable->setColumnWidth(4, 140);
    m_haInfoTable->setColumnWidth(5, 100);
    styleTable(m_haInfoTable);

    haInfoLayout->addWidget(m_haInfoTable);
    tabWidget->addTab(haInfoTab, QStringLiteral("HA 信息"));

    // ── Tab 2: 设备状态对比 ──
    auto* compareTab = new QWidget();
    auto* compareLayout = new QVBoxLayout(compareTab);
    compareLayout->setContentsMargins(4, 8, 4, 4);

    m_deviceCompareTable = new QTableWidget(0, 4);
    m_deviceCompareTable->setHorizontalHeaderLabels({
        QStringLiteral("指标"),
        QStringLiteral("主设备值"),
        QStringLiteral("备设备值"),
        QStringLiteral("状态")
    });
    m_deviceCompareTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_deviceCompareTable->setColumnWidth(1, 180);
    m_deviceCompareTable->setColumnWidth(2, 180);
    m_deviceCompareTable->setColumnWidth(3, 80);
    styleTable(m_deviceCompareTable);

    compareLayout->addWidget(m_deviceCompareTable);
    tabWidget->addTab(compareTab, QStringLiteral("设备对比"));

    // ── Tab 3: 接口状态 ──
    auto* ifTab = new QWidget();
    auto* ifLayout = new QVBoxLayout(ifTab);
    ifLayout->setContentsMargins(4, 8, 4, 4);

    m_ifStatusTable = new QTableWidget(0, 5);
    m_ifStatusTable->setHorizontalHeaderLabels({
        QStringLiteral("接口"),
        QStringLiteral("HA 地址"),
        QStringLiteral("物理地址"),
        QStringLiteral("状态"),
        QStringLiteral("链路状态")
    });
    m_ifStatusTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ifStatusTable->setColumnWidth(1, 150);
    m_ifStatusTable->setColumnWidth(2, 160);
    m_ifStatusTable->setColumnWidth(3, 70);
    m_ifStatusTable->setColumnWidth(4, 90);
    styleTable(m_ifStatusTable);

    ifLayout->addWidget(m_ifStatusTable);
    tabWidget->addTab(ifTab, QStringLiteral("接口状态"));

    // ── Tab 4: 切换日志 ──
    auto* logTab = new QWidget();
    auto* logLayout = new QVBoxLayout(logTab);
    logLayout->setContentsMargins(4, 8, 4, 4);

    m_switchLogTable = new QTableWidget(0, 5);
    m_switchLogTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("切换原因"),
        QStringLiteral("切换前主"),
        QStringLiteral("切换后主"),
        QStringLiteral("耗时(ms)")
    });
    m_switchLogTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_switchLogTable->setColumnWidth(1, 220);
    m_switchLogTable->setColumnWidth(2, 140);
    m_switchLogTable->setColumnWidth(3, 140);
    m_switchLogTable->setColumnWidth(4, 80);
    styleTable(m_switchLogTable);

    logLayout->addWidget(m_switchLogTable);
    tabWidget->addTab(logTab, QStringLiteral("切换日志"));

    mainLayout->addWidget(tabWidget, 1);

    // ── Bottom: Action buttons ──
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    m_switchTestBtn = new QPushButton(QStringLiteral("手动触发 HA 切换"));
    styleButton(m_switchTestBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));

    m_configSyncBtn = new QPushButton(QStringLiteral("配置文件同步"));
    styleButton(m_configSyncBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));

    m_exportBtn = new QPushButton(QStringLiteral("导出报告"));
    styleButton(m_exportBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));

    bottomLayout->addWidget(m_switchTestBtn);
    bottomLayout->addWidget(m_configSyncBtn);
    bottomLayout->addWidget(m_exportBtn);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void HAWidget::setupConnections()
{
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HAWidget::onDeviceChanged);
    connect(m_switchTestBtn, &QPushButton::clicked, this, &HAWidget::onSwitchTest);
    connect(m_configSyncBtn, &QPushButton::clicked, this, &HAWidget::onConfigSync);
    connect(m_exportBtn, &QPushButton::clicked, this, &HAWidget::onExport);

    // 定时刷新 (30秒)
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000);
    connect(m_refreshTimer, &QTimer::timeout, this, &HAWidget::onRefresh);
    m_refreshTimer->start();
}

// ─── Device Changed ──────────────────────────────────────────────────────────

void HAWidget::onDeviceChanged(int index)
{
    if (index < 0 || index >= m_haConfigs.size()) return;
    updateHAStatus(index);
    Logger::instance().debug(QStringLiteral("HA Management"),
        QString(QStringLiteral("切换到设备: %1")).arg(m_haConfigs[index].groupName));
}

// ─── Update HA Status ────────────────────────────────────────────────────────

void HAWidget::updateHAStatus(int haIndex)
{
    if (haIndex < 0 || haIndex >= m_haConfigs.size()) return;

    const HAConfig& cfg = m_haConfigs[haIndex];

    // 更新概览卡片
    QString statusColor = (cfg.status == QStringLiteral("Active")) ? QStringLiteral("#67C23A") :
                          (cfg.status == QStringLiteral("Standby")) ? QStringLiteral("#E6A23C") : QStringLiteral("#F56C6C");
    m_haStatusLabel->setStyleSheet(
        QString(QStringLiteral("font-size: 18px; color: %1; font-weight: bold;")).arg(statusColor)
    );
    m_haStatusLabel->setText(cfg.status);

    QString primaryDevice = (cfg.role == QStringLiteral("Active") || cfg.role == QStringLiteral("Master"))
        ? cfg.groupName : cfg.peer;
    m_primaryDeviceLabel->setText(primaryDevice);

    QString standbyDevice = (cfg.role == QStringLiteral("Active") || cfg.role == QStringLiteral("Master"))
        ? cfg.peer : cfg.groupName;
    m_standbyDeviceLabel->setText(standbyDevice);

    m_switchTimeLabel->setText(
        QString(QStringLiteral("%1 ms")).arg(cfg.heartbeatInterval)
    );

    // 上次切换时间
    if (haIndex < m_haSwitchLogs.size() && !m_haSwitchLogs[haIndex].isEmpty()) {
        const SwitchLog& lastLog = m_haSwitchLogs[haIndex].first();
        m_lastSwitchLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #8C8C8C; font-weight: bold;"));
        m_lastSwitchLabel->setText(lastLog.time.mid(5)); // 只显示月-日 时:分:秒
    } else {
        m_lastSwitchLabel->setText(QStringLiteral("无记录"));
    }

    // ── 填充 HA 信息表格 ──
    m_haInfoTable->setRowCount(0);
    int row = m_haInfoTable->rowCount();
    m_haInfoTable->insertRow(row);
    m_haInfoTable->setItem(row, 0, new QTableWidgetItem(cfg.groupName));
    m_haInfoTable->setItem(row, 1, new QTableWidgetItem(cfg.role));

    auto* priorityItem = new QTableWidgetItem(QString::number(cfg.priority));
    priorityItem->setTextAlignment(Qt::AlignCenter);
    m_haInfoTable->setItem(row, 2, priorityItem);

    auto* statusItem = new QTableWidgetItem(cfg.status);
    statusItem->setForeground(QColor(statusColor));
    m_haInfoTable->setItem(row, 3, statusItem);

    m_haInfoTable->setItem(row, 4, new QTableWidgetItem(cfg.peer));
    m_haInfoTable->setItem(row, 5, new QTableWidgetItem(
        QString(QStringLiteral("%1 ms")).arg(cfg.heartbeatInterval)));

    // ── 填充设备对比表格 ──
    m_deviceCompareTable->setRowCount(0);
    auto addCompareRow = [this](const QString& metric, const QString& primaryVal,
                                 const QString& standbyVal, const QString& status) {
        int r = m_deviceCompareTable->rowCount();
        m_deviceCompareTable->insertRow(r);
        m_deviceCompareTable->setItem(r, 0, new QTableWidgetItem(metric));
        m_deviceCompareTable->setItem(r, 1, new QTableWidgetItem(primaryVal));
        m_deviceCompareTable->setItem(r, 2, new QTableWidgetItem(standbyVal));

        auto* statusCell = new QTableWidgetItem(status);
        if (status == QStringLiteral("正常")) {
            statusCell->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (status == QStringLiteral("警告")) {
            statusCell->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else {
            statusCell->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_deviceCompareTable->setItem(r, 3, statusCell);
    };

    // 模拟主备设备对比数据
    auto* rng = QRandomGenerator::global();
    int cpuPrimary = rng->bounded(15, 45);
    int cpuStandby = rng->bounded(5, 25);
    int memPrimary = rng->bounded(40, 70);
    int memStandby = rng->bounded(30, 55);
    int tempPrimary = rng->bounded(35, 55);
    int tempStandby = rng->bounded(30, 48);

    addCompareRow(QStringLiteral("CPU 使用率"),
        QString(QStringLiteral("%1%")).arg(cpuPrimary),
        QString(QStringLiteral("%1%")).arg(cpuStandby),
        cpuPrimary < 80 ? QStringLiteral("正常") : QStringLiteral("警告"));

    addCompareRow(QStringLiteral("内存使用率"),
        QString(QStringLiteral("%1%")).arg(memPrimary),
        QString(QStringLiteral("%1%")).arg(memStandby),
        memPrimary < 80 ? QStringLiteral("正常") : QStringLiteral("警告"));

    addCompareRow(QStringLiteral("温度"),
        QString(QStringLiteral("%1°C")).arg(tempPrimary),
        QString(QStringLiteral("%1°C")).arg(tempStandby),
        tempPrimary < 65 ? QStringLiteral("正常") : QStringLiteral("警告"));

    addCompareRow(QStringLiteral("运行时间"),
        QStringLiteral("126天 8小时"),
        QStringLiteral("126天 8小时"),
        QStringLiteral("正常"));

    addCompareRow(QStringLiteral("配置版本"),
        QStringLiteral("v2.4.1"),
        QStringLiteral("v2.4.1"),
        QStringLiteral("正常"));

    addCompareRow(QStringLiteral("HA 协议版本"),
        QStringLiteral("1.2.0"),
        QStringLiteral("1.2.0"),
        QStringLiteral("正常"));

    addCompareRow(QStringLiteral("会话数"),
        QStringLiteral("12,450"),
        QStringLiteral("8,320"),
        QStringLiteral("正常"));

    addCompareRow(QStringLiteral("吞吐量"),
        QStringLiteral("850 Mbps"),
        QStringLiteral("420 Mbps"),
        QStringLiteral("正常"));

    // ── 填充接口状态表格 ──
    m_ifStatusTable->setRowCount(0);
    if (haIndex < m_haInterfaces.size()) {
        for (const auto& iface : m_haInterfaces[haIndex]) {
            int r = m_ifStatusTable->rowCount();
            m_ifStatusTable->insertRow(r);
            m_ifStatusTable->setItem(r, 0, new QTableWidgetItem(iface.ifName));
            m_ifStatusTable->setItem(r, 1, new QTableWidgetItem(iface.haAddr));
            m_ifStatusTable->setItem(r, 2, new QTableWidgetItem(iface.physAddr));

            auto* statusCell = new QTableWidgetItem(iface.status);
            statusCell->setForeground(iface.status == QStringLiteral("Up")
                ? QColor(QStringLiteral("#67C23A")) : QColor(QStringLiteral("#F56C6C")));
            m_ifStatusTable->setItem(r, 3, statusCell);

            auto* linkCell = new QTableWidgetItem(iface.linkState);
            linkCell->setForeground(iface.linkState == QStringLiteral("Link Up")
                ? QColor(QStringLiteral("#67C23A")) : QColor(QStringLiteral("#F56C6C")));
            m_ifStatusTable->setItem(r, 4, linkCell);
        }
    }

    // ── 填充切换日志表格 ──
    m_switchLogTable->setRowCount(0);
    if (haIndex < m_haSwitchLogs.size()) {
        for (const auto& log : m_haSwitchLogs[haIndex]) {
            int r = m_switchLogTable->rowCount();
            m_switchLogTable->insertRow(r);
            m_switchLogTable->setItem(r, 0, new QTableWidgetItem(log.time));
            m_switchLogTable->setItem(r, 1, new QTableWidgetItem(log.reason));
            m_switchLogTable->setItem(r, 2, new QTableWidgetItem(log.beforeMaster));
            m_switchLogTable->setItem(r, 3, new QTableWidgetItem(log.afterMaster));

            auto* durationItem = new QTableWidgetItem(QString::number(log.durationMs));
            if (log.durationMs > 1000) {
                durationItem->setForeground(QColor(QStringLiteral("#E6A23C")));
            } else if (log.durationMs > 500) {
                durationItem->setForeground(QColor(QStringLiteral("#E6A23C")));
            } else {
                durationItem->setForeground(QColor(QStringLiteral("#67C23A")));
            }
            m_switchLogTable->setItem(r, 4, durationItem);
        }
    }
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void HAWidget::onRefresh()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_haConfigs.size()) return;

    // 模拟状态刷新
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 5) {
        // 5% 概率模拟状态变化
        if (m_haConfigs[idx].status == QStringLiteral("Active")) {
            m_haConfigs[idx].status = QStringLiteral("Warning");
        }
    }

    updateHAStatus(idx);
    Logger::instance().debug(QStringLiteral("HA Management"),
        QString(QStringLiteral("刷新 HA 状态: %1")).arg(m_haConfigs[idx].groupName));
}

// ─── Switch Test ─────────────────────────────────────────────────────────────

void HAWidget::onSwitchTest()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_haConfigs.size()) return;

    const HAConfig& cfg = m_haConfigs[idx];

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        QStringLiteral("HA 切换测试"),
        QString(QStringLiteral("确认要对 HA 组 \"%1\" 触发主备切换测试吗？\n\n"
                                "这将导致主设备切换到备设备，可能影响业务流量。\n"
                                "请确保已做好准备工作。"))
            .arg(cfg.groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    // 模拟切换过程
    Logger::instance().info(QStringLiteral("HA Management"),
        QString(QStringLiteral("HA 切换测试开始: %1 → %2")).arg(cfg.groupName, cfg.peer));

    // 添加切换日志
    SwitchLog newLog;
    newLog.time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    newLog.reason = QStringLiteral("手动触发切换测试");
    newLog.beforeMaster = cfg.groupName;
    newLog.afterMaster = cfg.peer;
    newLog.durationMs = QRandomGenerator::global()->bounded(150, 450);
    m_haSwitchLogs[idx].prepend(newLog);

    // 更新状态
    if (m_haConfigs[idx].status == QStringLiteral("Active")) {
        m_haConfigs[idx].status = QStringLiteral("Standby");
    } else {
        m_haConfigs[idx].status = QStringLiteral("Active");
    }

    updateHAStatus(idx);

    QMessageBox::information(this,
        QStringLiteral("切换完成"),
        QString(QStringLiteral("HA 切换测试完成！\n"
                               "切换前主设备: %1\n"
                               "切换后主设备: %2\n"
                               "切换耗时: %3 ms"))
            .arg(newLog.beforeMaster, newLog.afterMaster)
            .arg(newLog.durationMs));

    Logger::instance().info(QStringLiteral("HA Management"),
        QString(QStringLiteral("HA 切换测试完成，耗时 %1 ms")).arg(newLog.durationMs));
}

// ─── Config Sync ─────────────────────────────────────────────────────────────

void HAWidget::onConfigSync()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_haConfigs.size()) return;

    const HAConfig& cfg = m_haConfigs[idx];

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("配置文件同步"),
        QString(QStringLiteral("确认要将主设备 \"%1\" 的配置同步到备设备 \"%2\" 吗？\n\n"
                                "同步内容:\n"
                                "  • HA 配置参数\n"
                                "  • 访问控制列表 (ACL)\n"
                                "  • 路由策略\n"
                                "  • 对象组定义\n"
                                "  • NAT 规则\n\n"
                                "同步后将自动验证配置一致性。"))
            .arg(cfg.groupName, cfg.peer),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    Logger::instance().info(QStringLiteral("HA Management"),
        QString(QStringLiteral("开始配置文件同步: %1 → %2")).arg(cfg.groupName, cfg.peer));

    // 模拟同步过程
    QApplication::processEvents();

    QMessageBox::information(this,
        QStringLiteral("同步完成"),
        QString(QStringLiteral("配置文件同步成功！\n\n"
                               "源设备: %1\n"
                               "目标设备: %2\n"
                               "同步项: 5 项全部成功\n"
                               "配置一致性: 100%%\n\n"
                               "已自动创建配置备份。"))
            .arg(cfg.groupName, cfg.peer));

    Logger::instance().info(QStringLiteral("HA Management"),
        QString(QStringLiteral("配置同步完成: %1 → %2")).arg(cfg.groupName, cfg.peer));
}

// ─── Export ───────────────────────────────────────────────────────────────────

void HAWidget::onExport()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_haConfigs.size()) return;

    const HAConfig& cfg = m_haConfigs[idx];

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出 HA 状态报告"),
        QString(QStringLiteral("ha_report_%1_%2.html"))
            .arg(cfg.groupName)
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("HTML 文件 (*.html)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("无法写入文件: ") + filePath);
        Logger::instance().error(QStringLiteral("HA Management"),
            QStringLiteral("Failed to open report file: ") + filePath);
        return;
    }

    QTextStream stream(&file);

    stream << QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    stream << QStringLiteral("<meta charset=\"UTF-8\">\n");
    stream << QStringLiteral("<title>HA 高可用性状态报告</title>\n");
    stream << QStringLiteral("<style>\n");
    stream << QStringLiteral("body { font-family: 'Microsoft YaHei', sans-serif; "
                              "background: #1E1F22; color: #DCDCDC; padding: 20px; }\n");
    stream << QStringLiteral("h1 { color: #409EFF; text-align: center; }\n");
    stream << QStringLiteral("h2 { color: #409EFF; margin-top: 30px; }\n");
    stream << QStringLiteral(".summary { display: flex; justify-content: space-around; "
                              "margin: 20px 0; }\n");
    stream << QStringLiteral(".card { background: #25262B; border: 1px solid #3C3F41; "
                              "border-radius: 8px; padding: 16px 24px; text-align: center; }\n");
    stream << QStringLiteral(".card .label { color: #8C8C8C; font-size: 12px; }\n");
    stream << QStringLiteral(".card .value { font-size: 20px; font-weight: bold; "
                              "margin-top: 4px; }\n");
    stream << QStringLiteral(".card .green { color: #67C23A; }\n");
    stream << QStringLiteral(".card .blue { color: #409EFF; }\n");
    stream << QStringLiteral(".card .orange { color: #E6A23C; }\n");
    stream << QStringLiteral("table { width: 100%%; border-collapse: collapse; "
                              "margin-top: 10px; }\n");
    stream << QStringLiteral("th { background: #25262B; color: #8C8C8C; padding: 10px; "
                              "border-bottom: 2px solid #3C3F41; text-align: left; }\n");
    stream << QStringLiteral("td { padding: 8px; border-bottom: 1px solid #2C2D30; }\n");
    stream << QStringLiteral(".up { color: #67C23A; font-weight: bold; }\n");
    stream << QStringLiteral(".down { color: #F56C6C; font-weight: bold; }\n");
    stream << QStringLiteral(".footer { text-align: center; color: #8C8C8C; "
                              "margin-top: 30px; font-size: 12px; }\n");
    stream << QStringLiteral("</style>\n</head>\n<body>\n");

    stream << QString(QStringLiteral("<h1>HA 高可用性状态报告</h1>\n"));
    stream << QString(QStringLiteral("<p style=\"text-align:center;color:#8C8C8C;\">"
                                      "HA 组: %1 | 生成时间: %2</p>\n"))
                .arg(cfg.groupName,
                     QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // Summary cards
    stream << QStringLiteral("<div class=\"summary\">\n");
    stream << QString(QStringLiteral(
        "<div class=\"card\"><div class=\"label\">HA 状态</div>"
        "<div class=\"value green\">%1</div></div>\n")).arg(cfg.status);
    stream << QString(QStringLiteral(
        "<div class=\"card\"><div class=\"label\">主设备</div>"
        "<div class=\"value blue\">%1</div></div>\n")).arg(cfg.groupName);
    stream << QString(QStringLiteral(
        "<div class=\"card\"><div class=\"label\">备设备</div>"
        "<div class=\"value orange\">%1</div></div>\n")).arg(cfg.peer);
    stream << QString(QStringLiteral(
        "<div class=\"card\"><div class=\"label\">心跳间隔</div>"
        "<div class=\"value\">%1 ms</div></div>\n")).arg(cfg.heartbeatInterval);
    stream << QStringLiteral("</div>\n");

    // HA Info table
    stream << QStringLiteral("<h2>HA 配置信息</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>HA 组名</th><th>角色</th>"
                              "<th>优先级</th><th>状态</th><th>对端</th><th>心跳间隔</th></tr>\n");
    stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td>"
                                      "<td class=\"up\">%4</td><td>%5</td><td>%6 ms</td></tr>\n"))
                .arg(cfg.groupName, cfg.role)
                .arg(cfg.priority)
                .arg(cfg.status, cfg.peer)
                .arg(cfg.heartbeatInterval);
    stream << QStringLiteral("</table>\n");

    // Interface status table
    stream << QStringLiteral("<h2>接口状态</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>接口</th><th>HA 地址</th>"
                              "<th>物理地址</th><th>状态</th><th>链路状态</th></tr>\n");
    if (idx < m_haInterfaces.size()) {
        for (const auto& iface : m_haInterfaces[idx]) {
            QString ifStatusClass = (iface.status == QStringLiteral("Up")) ? QStringLiteral("up") : QStringLiteral("down");
            QString linkClass = (iface.linkState == QStringLiteral("Link Up")) ? QStringLiteral("up") : QStringLiteral("down");
            stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td>"
                                              "<td class=\"%4\">%5</td><td class=\"%6\">%7</td></tr>\n"))
                        .arg(iface.ifName, iface.haAddr, iface.physAddr,
                             ifStatusClass, iface.status, linkClass, iface.linkState);
        }
    }
    stream << QStringLiteral("</table>\n");

    // Switch log table
    stream << QStringLiteral("<h2>切换日志</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>时间</th><th>切换原因</th>"
                              "<th>切换前主</th><th>切换后主</th><th>耗时(ms)</th></tr>\n");
    if (idx < m_haSwitchLogs.size()) {
        for (const auto& log : m_haSwitchLogs[idx]) {
            stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td>"
                                              "<td>%4</td><td>%5</td></tr>\n"))
                        .arg(log.time, log.reason, log.beforeMaster, log.afterMaster)
                        .arg(log.durationMs);
        }
    }
    stream << QStringLiteral("</table>\n");

    stream << QString(QStringLiteral("<div class=\"footer\">WukongToolkit — 网络工程师工具箱 | HA 高可用性管理</div>\n"));
    stream << QStringLiteral("</body>\n</html>\n");

    file.close();

    Logger::instance().info(QStringLiteral("HA Management"),
        QString(QStringLiteral("HA 报告已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("HA 状态报告已保存到:\n%1")).arg(filePath));
}