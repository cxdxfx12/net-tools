#include "aaa/AAAWidget.h"
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
#include <QRandomGenerator>

// ============================================================================
// 构造 / 析构
// ============================================================================
AAAWidget::AAAWidget(QWidget* parent)
    : QWidget(parent)
    , m_deviceCombo(nullptr)
    , m_aaaStatusLabel(nullptr)
    , m_sshStatusLabel(nullptr)
    , m_telnetStatusLabel(nullptr)
    , m_authMethodLabel(nullptr)
    , m_healthScoreLabel(nullptr)
    , m_radiusTable(nullptr)
    , m_tacacsTable(nullptr)
    , m_accountTable(nullptr)
    , m_loginStatsTable(nullptr)
    , m_riskTable(nullptr)
    , m_exportBtn(nullptr)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();

    // ── 预置模拟数据：4 个设备 ──────────────────────────────────────────

    // 设备1: Core-SW-01 (RADIUS + TACACS+)
    {
        AAAConfig cfg;
        cfg.deviceName   = QStringLiteral("Core-SW-01");
        cfg.aaaStatus    = QStringLiteral("已启用");
        cfg.sshStatus    = QStringLiteral("已启用");
        cfg.telnetStatus = QStringLiteral("未启用");
        cfg.authMethod   = QStringLiteral("混合");
        cfg.healthScore  = 0;

        RadiusServer r1;
        r1.ip = QStringLiteral("10.0.1.10"); r1.port = 1812; r1.timeout = 5;
        r1.retry = 3; r1.role = QStringLiteral("主"); r1.status = QStringLiteral("Active");
        r1.reachability = QStringLiteral("可达");
        RadiusServer r2;
        r2.ip = QStringLiteral("10.0.2.10"); r2.port = 1812; r2.timeout = 5;
        r2.retry = 3; r2.role = QStringLiteral("备"); r2.status = QStringLiteral("Active");
        r2.reachability = QStringLiteral("可达");
        cfg.radiusServers.append(r1);
        cfg.radiusServers.append(r2);

        TacacsServer t1;
        t1.ip = QStringLiteral("10.0.1.20"); t1.port = 49; t1.keyStatus = QStringLiteral("已加密");
        t1.role = QStringLiteral("主"); t1.timeout = 5; t1.authStatus = QStringLiteral("认证成功");
        cfg.tacacsServers.append(t1);

        LocalAccount a1, a2, a3, a4;
        a1.username = QStringLiteral("admin"); a1.privilegeLevel = 15;
        a1.lastLogin = QStringLiteral("2026-07-17 08:30:00"); a1.status = QStringLiteral("活跃");
        a1.expiryDate = QStringLiteral("2027-01-01");
        a2.username = QStringLiteral("netadmin"); a2.privilegeLevel = 15;
        a2.lastLogin = QStringLiteral("2026-07-16 18:00:00"); a2.status = QStringLiteral("活跃");
        a2.expiryDate = QStringLiteral("2027-01-01");
        a3.username = QStringLiteral("operator"); a3.privilegeLevel = 5;
        a3.lastLogin = QStringLiteral("2026-06-01 10:00:00"); a3.status = QStringLiteral("未使用");
        a3.expiryDate = QStringLiteral("2026-12-31");
        a4.username = QStringLiteral("monitor"); a4.privilegeLevel = 1;
        a4.lastLogin = QStringLiteral("2026-07-15 14:20:00"); a4.status = QStringLiteral("活跃");
        a4.expiryDate = QStringLiteral("2027-06-30");
        cfg.localAccounts.append(a1);
        cfg.localAccounts.append(a2);
        cfg.localAccounts.append(a3);
        cfg.localAccounts.append(a4);

        for (int i = 0; i < 10; ++i) {
            LoginRecord lr;
            lr.user = (i % 3 == 0) ? QStringLiteral("admin") :
                      (i % 3 == 1) ? QStringLiteral("netadmin") : QStringLiteral("monitor");
            lr.sourceIp = QStringLiteral("192.168.1.%1").arg(100 + i);
            lr.loginTime = QStringLiteral("2026-07-17 %1:%2:00")
                .arg(8 + i / 2, 2, 10, QLatin1Char('0'))
                .arg(i * 6 % 60, 2, 10, QLatin1Char('0'));
            lr.result = (i == 7) ? QStringLiteral("失败") : QStringLiteral("成功");
            lr.method = QStringLiteral("SSH");
            cfg.loginRecords.append(lr);
        }

        RiskItem ri1, ri2;
        ri1.riskName = QStringLiteral("未配置 SSH 密钥认证"); ri1.severity = QStringLiteral("中");
        ri1.device = QStringLiteral("Core-SW-01"); ri1.detail = QStringLiteral("SSH 仅支持密码认证，建议启用密钥认证");
        ri2.riskName = QStringLiteral("Telnet 协议未禁用"); ri2.severity = QStringLiteral("高");
        ri2.device = QStringLiteral("Core-SW-01"); ri2.detail = QStringLiteral("设备上 Telnet 服务端口仍处于监听状态");
        cfg.riskItems.append(ri1);
        cfg.riskItems.append(ri2);

        m_configs.append(cfg);
    }

    // 设备2: Router-GW-01 (仅 RADIUS)
    {
        AAAConfig cfg;
        cfg.deviceName   = QStringLiteral("Router-GW-01");
        cfg.aaaStatus    = QStringLiteral("已启用");
        cfg.sshStatus    = QStringLiteral("已启用");
        cfg.telnetStatus = QStringLiteral("未启用");
        cfg.authMethod   = QStringLiteral("RADIUS");
        cfg.healthScore  = 0;

        RadiusServer r1;
        r1.ip = QStringLiteral("10.0.1.10"); r1.port = 1812; r1.timeout = 10;
        r1.retry = 2; r1.role = QStringLiteral("主"); r1.status = QStringLiteral("Active");
        r1.reachability = QStringLiteral("可达");
        RadiusServer r2;
        r2.ip = QStringLiteral("10.0.2.10"); r2.port = 1812; r2.timeout = 10;
        r2.retry = 2; r2.role = QStringLiteral("备"); r2.status = QStringLiteral("Inactive");
        r2.reachability = QStringLiteral("不可达");
        cfg.radiusServers.append(r1);
        cfg.radiusServers.append(r2);

        LocalAccount a1, a2, a3;
        a1.username = QStringLiteral("root"); a1.privilegeLevel = 15;
        a1.lastLogin = QStringLiteral("2026-07-17 09:00:00"); a1.status = QStringLiteral("活跃");
        a1.expiryDate = QStringLiteral("永久");
        a2.username = QStringLiteral("engineer"); a2.privilegeLevel = 10;
        a2.lastLogin = QStringLiteral("2026-07-16 17:30:00"); a2.status = QStringLiteral("活跃");
        a2.expiryDate = QStringLiteral("2027-03-01");
        a3.username = QStringLiteral("guest"); a3.privilegeLevel = 0;
        a3.lastLogin = QStringLiteral("2025-12-01 08:00:00"); a3.status = QStringLiteral("锁定");
        a3.expiryDate = QStringLiteral("2026-01-01");
        cfg.localAccounts.append(a1);
        cfg.localAccounts.append(a2);
        cfg.localAccounts.append(a3);

        for (int i = 0; i < 10; ++i) {
            LoginRecord lr;
            lr.user = (i < 8) ? QStringLiteral("engineer") : QStringLiteral("guest");
            lr.sourceIp = QStringLiteral("10.10.10.%1").arg(50 + i);
            lr.loginTime = QStringLiteral("2026-07-17 %1:%2:00")
                .arg(7 + i / 2, 2, 10, QLatin1Char('0'))
                .arg(i * 5 % 60, 2, 10, QLatin1Char('0'));
            lr.result = (i < 8) ? QStringLiteral("成功") : QStringLiteral("失败");
            lr.method = (i % 2 == 0) ? QStringLiteral("SSH") : QStringLiteral("Console");
            cfg.loginRecords.append(lr);
        }

        RiskItem ri1;
        ri1.riskName = QStringLiteral("默认账户未禁用"); ri1.severity = QStringLiteral("高");
        ri1.device = QStringLiteral("Router-GW-01"); ri1.detail = QStringLiteral("存在 guest 默认账户且处于锁定状态");
        cfg.riskItems.append(ri1);

        m_configs.append(cfg);
    }

    // 设备3: FW-Cluster-01 (仅 TACACS+)
    {
        AAAConfig cfg;
        cfg.deviceName   = QStringLiteral("FW-Cluster-01");
        cfg.aaaStatus    = QStringLiteral("已启用");
        cfg.sshStatus    = QStringLiteral("已启用");
        cfg.telnetStatus = QStringLiteral("未启用");
        cfg.authMethod   = QStringLiteral("TACACS+");
        cfg.healthScore  = 0;

        TacacsServer t1;
        t1.ip = QStringLiteral("10.0.3.20"); t1.port = 49; t1.keyStatus = QStringLiteral("已加密");
        t1.role = QStringLiteral("主"); t1.timeout = 5; t1.authStatus = QStringLiteral("认证成功");
        cfg.tacacsServers.append(t1);

        LocalAccount a1, a2, a3, a4, a5;
        a1.username = QStringLiteral("fwadmin"); a1.privilegeLevel = 15;
        a1.lastLogin = QStringLiteral("2026-07-17 07:00:00"); a1.status = QStringLiteral("活跃");
        a1.expiryDate = QStringLiteral("2027-06-01");
        a2.username = QStringLiteral("auditor"); a2.privilegeLevel = 10;
        a2.lastLogin = QStringLiteral("2026-07-15 11:00:00"); a2.status = QStringLiteral("活跃");
        a2.expiryDate = QStringLiteral("2027-01-01");
        a3.username = QStringLiteral("backup"); a3.privilegeLevel = 15;
        a3.lastLogin = QStringLiteral("2026-05-01 09:00:00"); a3.status = QStringLiteral("未使用");
        a3.expiryDate = QStringLiteral("2027-01-01");
        a4.username = QStringLiteral("cisco"); a4.privilegeLevel = 15;
        a4.lastLogin = QStringLiteral("2025-01-01 00:00:00"); a4.status = QStringLiteral("锁定");
        a4.expiryDate = QStringLiteral("2025-12-31");
        a5.username = QStringLiteral("readonly"); a5.privilegeLevel = 1;
        a5.lastLogin = QStringLiteral("2026-07-17 06:30:00"); a5.status = QStringLiteral("活跃");
        a5.expiryDate = QStringLiteral("2027-12-31");
        cfg.localAccounts.append(a1);
        cfg.localAccounts.append(a2);
        cfg.localAccounts.append(a3);
        cfg.localAccounts.append(a4);
        cfg.localAccounts.append(a5);

        for (int i = 0; i < 10; ++i) {
            LoginRecord lr;
            lr.user = QStringLiteral("fwadmin");
            lr.sourceIp = QStringLiteral("172.16.0.%1").arg(10 + i);
            lr.loginTime = QStringLiteral("2026-07-16 %1:%2:00")
                .arg(20 + i / 2, 2, 10, QLatin1Char('0'))
                .arg(i * 7 % 60, 2, 10, QLatin1Char('0'));
            lr.result = (i == 9) ? QStringLiteral("失败") : QStringLiteral("成功");
            lr.method = QStringLiteral("SSH");
            cfg.loginRecords.append(lr);
        }

        RiskItem ri1, ri2, ri3;
        ri1.riskName = QStringLiteral("默认厂商账户 cisco"); ri1.severity = QStringLiteral("高");
        ri1.device = QStringLiteral("FW-Cluster-01"); ri1.detail = QStringLiteral("存在默认厂商账户 cisco，建议禁用或删除");
        ri2.riskName = QStringLiteral("长期未使用账户 backup"); ri2.severity = QStringLiteral("中");
        ri2.device = QStringLiteral("FW-Cluster-01"); ri2.detail = QStringLiteral("backup 账户已超过 60 天未登录");
        ri3.riskName = QStringLiteral("TACACS+ 单点故障"); ri3.severity = QStringLiteral("中");
        ri3.device = QStringLiteral("FW-Cluster-01"); ri3.detail = QStringLiteral("仅配置 1 台 TACACS+ 服务器，建议至少配置 2 台");
        cfg.riskItems.append(ri1);
        cfg.riskItems.append(ri2);
        cfg.riskItems.append(ri3);

        m_configs.append(cfg);
    }

    // 设备4: Agg-Switch-03 (AAA 未启用)
    {
        AAAConfig cfg;
        cfg.deviceName   = QStringLiteral("Agg-Switch-03");
        cfg.aaaStatus    = QStringLiteral("未启用");
        cfg.sshStatus    = QStringLiteral("已启用");
        cfg.telnetStatus = QStringLiteral("已启用");
        cfg.authMethod   = QStringLiteral("Local");
        cfg.healthScore  = 0;

        LocalAccount a1, a2, a3;
        a1.username = QStringLiteral("admin"); a1.privilegeLevel = 15;
        a1.lastLogin = QStringLiteral("2026-07-17 08:00:00"); a1.status = QStringLiteral("活跃");
        a1.expiryDate = QStringLiteral("永久");
        a2.username = QStringLiteral("user"); a2.privilegeLevel = 1;
        a2.lastLogin = QStringLiteral("2026-07-16 16:00:00"); a2.status = QStringLiteral("活跃");
        a2.expiryDate = QStringLiteral("永久");
        a3.username = QStringLiteral("test"); a3.privilegeLevel = 0;
        a3.lastLogin = QStringLiteral("2026-01-01 00:00:00"); a3.status = QStringLiteral("未使用");
        a3.expiryDate = QStringLiteral("2026-06-01");
        cfg.localAccounts.append(a1);
        cfg.localAccounts.append(a2);
        cfg.localAccounts.append(a3);

        for (int i = 0; i < 10; ++i) {
            LoginRecord lr;
            lr.user = QStringLiteral("admin");
            lr.sourceIp = QStringLiteral("192.168.100.%1").arg(1 + i);
            lr.loginTime = QStringLiteral("2026-07-17 %1:%2:00")
                .arg(6 + i, 2, 10, QLatin1Char('0'))
                .arg(i * 10 % 60, 2, 10, QLatin1Char('0'));
            lr.result = QStringLiteral("成功");
            lr.method = (i < 5) ? QStringLiteral("Telnet") : QStringLiteral("SSH");
            cfg.loginRecords.append(lr);
        }

        RiskItem ri1, ri2, ri3, ri4;
        ri1.riskName = QStringLiteral("AAA 未启用"); ri1.severity = QStringLiteral("高");
        ri1.device = QStringLiteral("Agg-Switch-03"); ri1.detail = QStringLiteral("设备未启用 AAA 认证，存在安全风险");
        ri2.riskName = QStringLiteral("Telnet 明文传输"); ri2.severity = QStringLiteral("高");
        ri2.device = QStringLiteral("Agg-Switch-03"); ri2.detail = QStringLiteral("Telnet 协议以明文传输登录凭据，建议禁用并仅使用 SSH");
        ri3.riskName = QStringLiteral("测试账户未清理"); ri3.severity = QStringLiteral("中");
        ri3.device = QStringLiteral("Agg-Switch-03"); ri3.detail = QStringLiteral("test 账户已过期但未删除，存在安全隐患");
        ri4.riskName = QStringLiteral("无 RADIUS/TACACS+ 配置"); ri4.severity = QStringLiteral("中");
        ri4.device = QStringLiteral("Agg-Switch-03"); ri4.detail = QStringLiteral("仅使用本地认证，建议配置 RADIUS 或 TACACS+ 集中认证");
        cfg.riskItems.append(ri1);
        cfg.riskItems.append(ri2);
        cfg.riskItems.append(ri3);
        cfg.riskItems.append(ri4);

        m_configs.append(cfg);
    }

    // 初始化设备下拉框
    for (const auto& cfg : m_configs) {
        m_deviceCombo->addItem(cfg.deviceName);
    }

    // 初始刷新
    onDeviceChanged(0);

    // 定时刷新 (30 秒)
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000);
    connect(m_refreshTimer, &QTimer::timeout, this, &AAAWidget::onRefresh);
    m_refreshTimer->start();

    Logger::instance().info(QStringLiteral("AAA Center"),
        QStringLiteral("AAA & 认证中心初始化完成，%1 个设备").arg(m_configs.size()));
}

// ============================================================================
// UI 构建
// ============================================================================
void AAAWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 通用样式 lambda ────────────────────────────────────────────────

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

    auto styleGroupBox = [](QGroupBox* group) {
        group->setStyleSheet(
            QStringLiteral(
                "QGroupBox {"
                "  color: #409EFF; font-size: 13px; font-weight: bold;"
                "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
                "  padding-top: 16px;"
                "}"
                "QGroupBox::title {"
                "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
                "}"
            )
        );
    };

    // ── 顶部: 设备选择 ─────────────────────────────────────────────────
    auto* topLayout = new QHBoxLayout();
    auto* deviceLabel = new QLabel(QStringLiteral("选择设备:"));
    deviceLabel->setStyleSheet(QStringLiteral("color: #DCDCDC; font-size: 13px; font-weight: bold;"));
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(200);
    m_deviceCombo->setStyleSheet(
        QStringLiteral(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 6px 12px;"
            "  border-radius: 3px; font-size: 13px; min-width: 180px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        )
    );
    topLayout->addWidget(deviceLabel);
    topLayout->addWidget(m_deviceCombo);

    m_exportBtn = new QPushButton(QStringLiteral("导出 CSV"));
    m_exportBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #4A4D52; color: #DCDCDC;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #5A5D62; }"
        )
    );
    topLayout->addStretch();
    topLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(topLayout);

    // ── 认证概览区: 5 个彩色状态卡片 ───────────────────────────────────
    auto* overviewGroup = new QGroupBox(QStringLiteral("认证概览"));
    styleGroupBox(overviewGroup);
    auto* overviewLayout = new QHBoxLayout(overviewGroup);
    overviewLayout->setSpacing(12);

    auto makeCard = [](const QString& title, const QString& color) -> QLabel* {
        auto* label = new QLabel(QStringLiteral("%1\n--").arg(title));
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumHeight(70);
        label->setStyleSheet(
            QString(QStringLiteral(
                "font-size: 14px; font-weight: bold; color: %1;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid %1;"
            )).arg(color)
        );
        return label;
    };

    m_aaaStatusLabel    = makeCard(QStringLiteral("AAA 状态"),    QStringLiteral("#409EFF"));
    m_sshStatusLabel    = makeCard(QStringLiteral("SSH 状态"),    QStringLiteral("#67C23A"));
    m_telnetStatusLabel = makeCard(QStringLiteral("Telnet 状态"), QStringLiteral("#E6A23C"));
    m_authMethodLabel   = makeCard(QStringLiteral("认证方式"),    QStringLiteral("#909399"));
    m_healthScoreLabel  = makeCard(QStringLiteral("健康评分"),    QStringLiteral("#F56C6C"));

    overviewLayout->addWidget(m_aaaStatusLabel);
    overviewLayout->addWidget(m_sshStatusLabel);
    overviewLayout->addWidget(m_telnetStatusLabel);
    overviewLayout->addWidget(m_authMethodLabel);
    overviewLayout->addWidget(m_healthScoreLabel);

    mainLayout->addWidget(overviewGroup);

    // ── 中部标签页: RADIUS / TACACS+ / 本地账户 / 登录统计 ───────────
    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        QStringLiteral(
            "QTabWidget::pane {"
            "  border: 1px solid #3C3F41; background-color: #1E1F22;"
            "  border-top: 2px solid #409EFF;"
            "}"
            "QTabBar::tab {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  padding: 8px 20px; font-size: 13px; font-weight: bold;"
            "  border: 1px solid #3C3F41; border-bottom: none;"
            "  border-top-left-radius: 4px; border-top-right-radius: 4px;"
            "  margin-right: 2px;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: #1E1F22; color: #409EFF;"
            "  border-bottom: 2px solid #409EFF;"
            "}"
            "QTabBar::tab:hover {"
            "  background-color: #3C3F41; color: #DCDCDC;"
            "}"
        )
    );

    // RADIUS 表格
    m_radiusTable = new QTableWidget();
    m_radiusTable->setColumnCount(7);
    m_radiusTable->setHorizontalHeaderLabels({
        QStringLiteral("服务器 IP"),
        QStringLiteral("端口"),
        QStringLiteral("超时(s)"),
        QStringLiteral("重试次数"),
        QStringLiteral("主备"),
        QStringLiteral("状态"),
        QStringLiteral("可达性")
    });
    m_radiusTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_radiusTable);
    tabWidget->addTab(m_radiusTable, QStringLiteral("RADIUS"));

    // TACACS+ 表格
    m_tacacsTable = new QTableWidget();
    m_tacacsTable->setColumnCount(6);
    m_tacacsTable->setHorizontalHeaderLabels({
        QStringLiteral("服务器 IP"),
        QStringLiteral("端口"),
        QStringLiteral("Key 状态"),
        QStringLiteral("主备"),
        QStringLiteral("超时(s)"),
        QStringLiteral("认证状态")
    });
    m_tacacsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_tacacsTable);
    tabWidget->addTab(m_tacacsTable, QStringLiteral("TACACS+"));

    // 本地账户表格
    m_accountTable = new QTableWidget();
    m_accountTable->setColumnCount(5);
    m_accountTable->setHorizontalHeaderLabels({
        QStringLiteral("用户名"),
        QStringLiteral("权限级别"),
        QStringLiteral("最后登录"),
        QStringLiteral("状态"),
        QStringLiteral("过期时间")
    });
    m_accountTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_accountTable);
    tabWidget->addTab(m_accountTable, QStringLiteral("本地账户"));

    // 登录统计表格
    m_loginStatsTable = new QTableWidget();
    m_loginStatsTable->setColumnCount(5);
    m_loginStatsTable->setHorizontalHeaderLabels({
        QStringLiteral("用户"),
        QStringLiteral("来源 IP"),
        QStringLiteral("登录时间"),
        QStringLiteral("结果"),
        QStringLiteral("方式")
    });
    m_loginStatsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_loginStatsTable);
    tabWidget->addTab(m_loginStatsTable, QStringLiteral("登录统计"));

    mainLayout->addWidget(tabWidget, 1);

    // ── 底部: 风险项 ───────────────────────────────────────────────────
    auto* riskGroup = new QGroupBox(QStringLiteral("安全风险项"));
    styleGroupBox(riskGroup);
    auto* riskLayout = new QVBoxLayout(riskGroup);
    riskLayout->setContentsMargins(4, 8, 4, 4);

    m_riskTable = new QTableWidget();
    m_riskTable->setColumnCount(4);
    m_riskTable->setHorizontalHeaderLabels({
        QStringLiteral("风险项"),
        QStringLiteral("严重级别"),
        QStringLiteral("设备"),
        QStringLiteral("详情")
    });
    m_riskTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    styleTable(m_riskTable);
    riskLayout->addWidget(m_riskTable);

    mainLayout->addWidget(riskGroup);
}

// ============================================================================
// 信号槽连接
// ============================================================================
void AAAWidget::setupConnections()
{
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AAAWidget::onDeviceChanged);
    connect(m_exportBtn, &QPushButton::clicked, this, &AAAWidget::onExport);
}

// ============================================================================
// 设备切换
// ============================================================================
void AAAWidget::onDeviceChanged(int index)
{
    if (index < 0 || index >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(index);

    // 更新概览卡片
    m_aaaStatusLabel->setText(QStringLiteral("AAA 状态\n%1").arg(cfg.aaaStatus));
    m_sshStatusLabel->setText(QStringLiteral("SSH 状态\n%1").arg(cfg.sshStatus));
    m_telnetStatusLabel->setText(QStringLiteral("Telnet 状态\n%1").arg(cfg.telnetStatus));
    m_authMethodLabel->setText(QStringLiteral("认证方式\n%1").arg(cfg.authMethod));

    // 刷新各项检查
    checkAAAStatus();
    checkRadius();
    checkTacacs();
    checkAccounts();
    checkLoginStats();
    calculateHealthScore();
}

// ============================================================================
// 刷新
// ============================================================================
void AAAWidget::onRefresh()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx >= 0 && idx < m_configs.size()) {
        onDeviceChanged(idx);
    }
    Logger::instance().debug(QStringLiteral("AAA Center"), QStringLiteral("定时刷新完成"));
}

// ============================================================================
// 检查 AAA 状态
// ============================================================================
void AAAWidget::checkAAAStatus()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);

    // AAA 状态卡片颜色
    if (cfg.aaaStatus == QStringLiteral("已启用")) {
        m_aaaStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #67C23A;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #67C23A;"
            )
        );
    } else {
        m_aaaStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #F56C6C;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #F56C6C;"
            )
        );
    }

    // SSH 状态
    if (cfg.sshStatus == QStringLiteral("已启用")) {
        m_sshStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #67C23A;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #67C23A;"
            )
        );
    } else {
        m_sshStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #F56C6C;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #F56C6C;"
            )
        );
    }

    // Telnet 状态
    if (cfg.telnetStatus == QStringLiteral("未启用")) {
        m_telnetStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #67C23A;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #67C23A;"
            )
        );
    } else {
        m_telnetStatusLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #E6A23C;"
                "background-color: #25262B; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #E6A23C;"
            )
        );
    }

    // 认证方式卡片
    QString methodColor = QStringLiteral("#909399");
    if (cfg.authMethod == QStringLiteral("混合")) {
        methodColor = QStringLiteral("#67C23A");
    } else if (cfg.authMethod == QStringLiteral("Local")) {
        methodColor = QStringLiteral("#E6A23C");
    } else {
        methodColor = QStringLiteral("#409EFF");
    }
    m_authMethodLabel->setStyleSheet(
        QString(QStringLiteral(
            "font-size: 14px; font-weight: bold; color: %1;"
            "background-color: #25262B; border-radius: 8px;"
            "padding: 10px 12px; border: 2px solid %1;"
        )).arg(methodColor)
    );

    Logger::instance().debug(QStringLiteral("AAA Center"),
        QString(QStringLiteral("AAA 状态检查完成: %1")).arg(cfg.deviceName));
}

// ============================================================================
// 检查 RADIUS
// ============================================================================
void AAAWidget::checkRadius()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);
    m_radiusTable->setRowCount(0);

    if (cfg.radiusServers.isEmpty()) {
        QTableWidgetItem* emptyItem = new QTableWidgetItem(QStringLiteral("未配置 RADIUS 服务器"));
        emptyItem->setForeground(QColor(QStringLiteral("#8C8C8C")));
        m_radiusTable->setRowCount(1);
        m_radiusTable->setItem(0, 0, emptyItem);
        m_radiusTable->setSpan(0, 0, 1, 7);
        return;
    }

    for (const RadiusServer& rs : cfg.radiusServers) {
        int row = m_radiusTable->rowCount();
        m_radiusTable->insertRow(row);

        m_radiusTable->setItem(row, 0, new QTableWidgetItem(rs.ip));

        auto* portItem = new QTableWidgetItem(QString::number(rs.port));
        portItem->setTextAlignment(Qt::AlignCenter);
        m_radiusTable->setItem(row, 1, portItem);

        auto* timeoutItem = new QTableWidgetItem(QString::number(rs.timeout));
        timeoutItem->setTextAlignment(Qt::AlignCenter);
        m_radiusTable->setItem(row, 2, timeoutItem);

        auto* retryItem = new QTableWidgetItem(QString::number(rs.retry));
        retryItem->setTextAlignment(Qt::AlignCenter);
        m_radiusTable->setItem(row, 3, retryItem);

        auto* roleItem = new QTableWidgetItem(rs.role);
        roleItem->setTextAlignment(Qt::AlignCenter);
        if (rs.role == QStringLiteral("主")) {
            roleItem->setForeground(QColor(QStringLiteral("#409EFF")));
        }
        m_radiusTable->setItem(row, 4, roleItem);

        auto* statusItem = new QTableWidgetItem(rs.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (rs.status == QStringLiteral("Active")) {
            statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_radiusTable->setItem(row, 5, statusItem);

        auto* reachItem = new QTableWidgetItem(rs.reachability);
        reachItem->setTextAlignment(Qt::AlignCenter);
        if (rs.reachability == QStringLiteral("可达")) {
            reachItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else {
            reachItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_radiusTable->setItem(row, 6, reachItem);
    }
}

// ============================================================================
// 检查 TACACS+
// ============================================================================
void AAAWidget::checkTacacs()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);
    m_tacacsTable->setRowCount(0);

    if (cfg.tacacsServers.isEmpty()) {
        QTableWidgetItem* emptyItem = new QTableWidgetItem(QStringLiteral("未配置 TACACS+ 服务器"));
        emptyItem->setForeground(QColor(QStringLiteral("#8C8C8C")));
        m_tacacsTable->setRowCount(1);
        m_tacacsTable->setItem(0, 0, emptyItem);
        m_tacacsTable->setSpan(0, 0, 1, 6);
        return;
    }

    for (const TacacsServer& ts : cfg.tacacsServers) {
        int row = m_tacacsTable->rowCount();
        m_tacacsTable->insertRow(row);

        m_tacacsTable->setItem(row, 0, new QTableWidgetItem(ts.ip));

        auto* portItem = new QTableWidgetItem(QString::number(ts.port));
        portItem->setTextAlignment(Qt::AlignCenter);
        m_tacacsTable->setItem(row, 1, portItem);

        auto* keyItem = new QTableWidgetItem(ts.keyStatus);
        keyItem->setTextAlignment(Qt::AlignCenter);
        if (ts.keyStatus == QStringLiteral("已加密")) {
            keyItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (ts.keyStatus == QStringLiteral("未配置")) {
            keyItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else {
            keyItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        }
        m_tacacsTable->setItem(row, 2, keyItem);

        auto* roleItem = new QTableWidgetItem(ts.role);
        roleItem->setTextAlignment(Qt::AlignCenter);
        if (ts.role == QStringLiteral("主")) {
            roleItem->setForeground(QColor(QStringLiteral("#409EFF")));
        }
        m_tacacsTable->setItem(row, 3, roleItem);

        auto* timeoutItem = new QTableWidgetItem(QString::number(ts.timeout));
        timeoutItem->setTextAlignment(Qt::AlignCenter);
        m_tacacsTable->setItem(row, 4, timeoutItem);

        auto* authItem = new QTableWidgetItem(ts.authStatus);
        authItem->setTextAlignment(Qt::AlignCenter);
        if (ts.authStatus == QStringLiteral("认证成功")) {
            authItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (ts.authStatus == QStringLiteral("认证失败")) {
            authItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else {
            authItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        }
        m_tacacsTable->setItem(row, 5, authItem);
    }
}

// ============================================================================
// 检查本地账户
// ============================================================================
void AAAWidget::checkAccounts()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);
    m_accountTable->setRowCount(0);

    for (const LocalAccount& acct : cfg.localAccounts) {
        int row = m_accountTable->rowCount();
        m_accountTable->insertRow(row);

        m_accountTable->setItem(row, 0, new QTableWidgetItem(acct.username));

        auto* privItem = new QTableWidgetItem(QString::number(acct.privilegeLevel));
        privItem->setTextAlignment(Qt::AlignCenter);
        if (acct.privilegeLevel == 15) {
            privItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else if (acct.privilegeLevel >= 5) {
            privItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        }
        m_accountTable->setItem(row, 1, privItem);

        m_accountTable->setItem(row, 2, new QTableWidgetItem(acct.lastLogin));

        auto* statusItem = new QTableWidgetItem(acct.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (acct.status == QStringLiteral("活跃")) {
            statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (acct.status == QStringLiteral("锁定")) {
            statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        }
        m_accountTable->setItem(row, 3, statusItem);

        auto* expiryItem = new QTableWidgetItem(acct.expiryDate);
        expiryItem->setTextAlignment(Qt::AlignCenter);
        if (acct.expiryDate != QStringLiteral("永久")) {
            QDate expiry = QDate::fromString(acct.expiryDate, QStringLiteral("yyyy-MM-dd"));
            if (expiry.isValid() && expiry < QDate::currentDate()) {
                expiryItem->setForeground(QColor(QStringLiteral("#F56C6C")));
            }
        }
        m_accountTable->setItem(row, 4, expiryItem);
    }
}

// ============================================================================
// 检查登录统计
// ============================================================================
void AAAWidget::checkLoginStats()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);
    m_loginStatsTable->setRowCount(0);

    for (const LoginRecord& lr : cfg.loginRecords) {
        int row = m_loginStatsTable->rowCount();
        m_loginStatsTable->insertRow(row);

        m_loginStatsTable->setItem(row, 0, new QTableWidgetItem(lr.user));
        m_loginStatsTable->setItem(row, 1, new QTableWidgetItem(lr.sourceIp));
        m_loginStatsTable->setItem(row, 2, new QTableWidgetItem(lr.loginTime));

        auto* resultItem = new QTableWidgetItem(lr.result);
        resultItem->setTextAlignment(Qt::AlignCenter);
        if (lr.result == QStringLiteral("成功")) {
            resultItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else {
            resultItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_loginStatsTable->setItem(row, 3, resultItem);

        auto* methodItem = new QTableWidgetItem(lr.method);
        methodItem->setTextAlignment(Qt::AlignCenter);
        if (lr.method == QStringLiteral("Telnet")) {
            methodItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else if (lr.method == QStringLiteral("SSH")) {
            methodItem->setForeground(QColor(QStringLiteral("#67C23A")));
        }
        m_loginStatsTable->setItem(row, 4, methodItem);
    }
}

// ============================================================================
// 计算健康评分
// ============================================================================
void AAAWidget::calculateHealthScore()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) return;

    const AAAConfig& cfg = m_configs.at(idx);
    int score = 0;

    // ── AAA 配置 (30 分) ──────────────────────────────────────────────
    if (cfg.aaaStatus == QStringLiteral("已启用")) {
        score += 15;
    }
    if (cfg.sshStatus == QStringLiteral("已启用")) {
        score += 10;
    }
    if (cfg.telnetStatus == QStringLiteral("未启用")) {
        score += 5;
    }

    // ── RADIUS (25 分) ────────────────────────────────────────────────
    if (!cfg.radiusServers.isEmpty()) {
        score += 10;
        int activeCount = 0;
        for (const auto& rs : cfg.radiusServers) {
            if (rs.status == QStringLiteral("Active") && rs.reachability == QStringLiteral("可达")) {
                activeCount++;
            }
        }
        if (activeCount >= 2) {
            score += 15;  // 主备都正常
        } else if (activeCount == 1) {
            score += 8;   // 只有一台正常
        } else {
            score += 0;   // 全部不可用
        }
    }

    // ── TACACS+ (25 分) ───────────────────────────────────────────────
    if (!cfg.tacacsServers.isEmpty()) {
        score += 10;
        int goodCount = 0;
        for (const auto& ts : cfg.tacacsServers) {
            if (ts.keyStatus == QStringLiteral("已加密") && ts.authStatus == QStringLiteral("认证成功")) {
                goodCount++;
            }
        }
        if (goodCount >= 2) {
            score += 15;
        } else if (goodCount == 1) {
            score += 8;
        } else {
            score += 0;
        }
    }

    // ── 账户 (20 分) ──────────────────────────────────────────────────
    int totalAccounts = cfg.localAccounts.size();
    if (totalAccounts > 0) {
        int goodAccounts = 0;
        for (const auto& acct : cfg.localAccounts) {
            if (acct.status == QStringLiteral("活跃") && acct.privilegeLevel <= 10) {
                goodAccounts++;
            }
            // 检测过期账户
            if (acct.expiryDate != QStringLiteral("永久")) {
                QDate expiry = QDate::fromString(acct.expiryDate, QStringLiteral("yyyy-MM-dd"));
                if (expiry.isValid() && expiry < QDate::currentDate()) {
                    score -= 2;
                }
            }
            // 检测锁定账户
            if (acct.status == QStringLiteral("锁定")) {
                score -= 2;
            }
        }
        score += qMin(goodAccounts * 4, 20);
    }

    // 风险项扣分
    int highRisks = 0;
    int mediumRisks = 0;
    for (const auto& risk : cfg.riskItems) {
        if (risk.severity == QStringLiteral("高")) {
            highRisks++;
        } else if (risk.severity == QStringLiteral("中")) {
            mediumRisks++;
        }
    }
    score -= highRisks * 5;
    score -= mediumRisks * 2;

    // 限制在 0-100
    score = qBound(0, score, 100);

    // 更新健康评分卡片
    QString scoreColor;
    if (score >= 80) {
        scoreColor = QStringLiteral("#67C23A");
    } else if (score >= 60) {
        scoreColor = QStringLiteral("#E6A23C");
    } else {
        scoreColor = QStringLiteral("#F56C6C");
    }

    m_healthScoreLabel->setText(QStringLiteral("健康评分\n%1 分").arg(score));
    m_healthScoreLabel->setStyleSheet(
        QString(QStringLiteral(
            "font-size: 14px; font-weight: bold; color: %1;"
            "background-color: #25262B; border-radius: 8px;"
            "padding: 10px 12px; border: 2px solid %1;"
        )).arg(scoreColor)
    );

    // 更新风险项表格
    m_riskTable->setRowCount(0);
    for (const RiskItem& risk : cfg.riskItems) {
        int row = m_riskTable->rowCount();
        m_riskTable->insertRow(row);

        m_riskTable->setItem(row, 0, new QTableWidgetItem(risk.riskName));

        auto* sevItem = new QTableWidgetItem(risk.severity);
        sevItem->setTextAlignment(Qt::AlignCenter);
        if (risk.severity == QStringLiteral("高")) {
            sevItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else if (risk.severity == QStringLiteral("中")) {
            sevItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else {
            sevItem->setForeground(QColor(QStringLiteral("#409EFF")));
        }
        m_riskTable->setItem(row, 1, sevItem);

        m_riskTable->setItem(row, 2, new QTableWidgetItem(risk.device));
        m_riskTable->setItem(row, 3, new QTableWidgetItem(risk.detail));
    }

    Logger::instance().debug(QStringLiteral("AAA Center"),
        QString(QStringLiteral("健康评分: %1 分 (%2)")).arg(score).arg(cfg.deviceName));
}

// ============================================================================
// 导出 CSV
// ============================================================================
void AAAWidget::onExport()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || idx >= m_configs.size()) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("没有可导出的数据。"));
        return;
    }

    const AAAConfig& cfg = m_configs.at(idx);

    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出 AAA 认证报告"),
        QString(QStringLiteral("aaa_report_%1.csv")).arg(cfg.deviceName),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法写入文件: ") + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF"); // BOM for Excel UTF-8
    QTextStream out(&file);

    auto csvEscape = [](const QString& val) -> QString {
        QString v = val;
        if (v.contains(',') || v.contains('\n') || v.contains('"')) {
            v.replace("\"", "\"\"");
            v = "\"" + v + "\"";
        }
        return v;
    };

    out << QStringLiteral("=== AAA 认证报告 — %1 ===\n").arg(cfg.deviceName);
    out << QStringLiteral("\n--- 认证概览 ---\n");
    out << QStringLiteral("AAA 状态,%1\n").arg(cfg.aaaStatus);
    out << QStringLiteral("SSH 状态,%1\n").arg(cfg.sshStatus);
    out << QStringLiteral("Telnet 状态,%1\n").arg(cfg.telnetStatus);
    out << QStringLiteral("认证方式,%1\n").arg(cfg.authMethod);
    out << QStringLiteral("健康评分,%1\n").arg(cfg.healthScore);

    out << QStringLiteral("\n--- RADIUS 服务器 ---\n");
    out << QStringLiteral("服务器 IP,端口,超时(s),重试次数,主备,状态,可达性\n");
    for (const auto& rs : cfg.radiusServers) {
        out << csvEscape(rs.ip) << ","
            << rs.port << ","
            << rs.timeout << ","
            << rs.retry << ","
            << csvEscape(rs.role) << ","
            << csvEscape(rs.status) << ","
            << csvEscape(rs.reachability) << "\n";
    }

    out << QStringLiteral("\n--- TACACS+ 服务器 ---\n");
    out << QStringLiteral("服务器 IP,端口,Key 状态,主备,超时(s),认证状态\n");
    for (const auto& ts : cfg.tacacsServers) {
        out << csvEscape(ts.ip) << ","
            << ts.port << ","
            << csvEscape(ts.keyStatus) << ","
            << csvEscape(ts.role) << ","
            << ts.timeout << ","
            << csvEscape(ts.authStatus) << "\n";
    }

    out << QStringLiteral("\n--- 本地账户 ---\n");
    out << QStringLiteral("用户名,权限级别,最后登录,状态,过期时间\n");
    for (const auto& acct : cfg.localAccounts) {
        out << csvEscape(acct.username) << ","
            << acct.privilegeLevel << ","
            << csvEscape(acct.lastLogin) << ","
            << csvEscape(acct.status) << ","
            << csvEscape(acct.expiryDate) << "\n";
    }

    out << QStringLiteral("\n--- 登录统计 ---\n");
    out << QStringLiteral("用户,来源 IP,登录时间,结果,方式\n");
    for (const auto& lr : cfg.loginRecords) {
        out << csvEscape(lr.user) << ","
            << csvEscape(lr.sourceIp) << ","
            << csvEscape(lr.loginTime) << ","
            << csvEscape(lr.result) << ","
            << csvEscape(lr.method) << "\n";
    }

    out << QStringLiteral("\n--- 安全风险项 ---\n");
    out << QStringLiteral("风险项,严重级别,设备,详情\n");
    for (const auto& risk : cfg.riskItems) {
        out << csvEscape(risk.riskName) << ","
            << csvEscape(risk.severity) << ","
            << csvEscape(risk.device) << ","
            << csvEscape(risk.detail) << "\n";
    }

    file.close();
    Logger::instance().info(QStringLiteral("AAA Center"),
        QString(QStringLiteral("AAA 报告已导出到: %1 (%2)").arg(filePath, cfg.deviceName)));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("AAA 认证报告已保存到:\n%1")).arg(filePath));
}