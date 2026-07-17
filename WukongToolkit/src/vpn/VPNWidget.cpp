#include "vpn/VPNWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QTabWidget>
#include <QFrame>
#include <QDateTime>
#include <QRandomGenerator>
#include <QColor>
#include <QApplication>
#include <QSplitter>

// ═══════════════════════════════════════════════════════════════════════════════
// VPNWidget — VPN 管理中心 (第56章)
// ═══════════════════════════════════════════════════════════════════════════════

VPNWidget::VPNWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
    generateMockTunnels();

    // 预置设备
    m_deviceCombo->addItem(QStringLiteral("HQ-FW-01  (10.0.0.1)"));
    m_deviceCombo->addItem(QStringLiteral("Branch-FW-01  (172.16.0.1)"));
    m_deviceCombo->addItem(QStringLiteral("Branch-FW-02  (172.16.1.1)"));
    m_deviceCombo->addItem(QStringLiteral("DC-FW-01  (192.168.0.1)"));
    m_deviceCombo->addItem(QStringLiteral("Cloud-VPN-GW  (100.64.0.1)"));

    // 刷新一次初始状态
    updateTunnelStatus();
    calculateHealthScore();

    // 启动定时刷新 (30秒)
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000);
    m_refreshTimer->start();

    Logger::instance().info(QStringLiteral("VPN Center"),
        QStringLiteral("VPN 管理中心初始化完成"));
}

VPNWidget::~VPNWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void VPNWidget::setupUI()
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

    auto styleCombo = [](QComboBox* combo) {
        combo->setStyleSheet(
            QStringLiteral(
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
            )
        );
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

    // ── Top: Device + VPN Type selection ──
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(12);

    auto* deviceGroup = new QGroupBox(QStringLiteral("设备选择"));
    styleGroupBox(deviceGroup);
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    m_deviceCombo = new QComboBox();
    styleCombo(m_deviceCombo);
    deviceLayout->addWidget(m_deviceCombo);
    topBar->addWidget(deviceGroup, 1);

    auto* typeGroup = new QGroupBox(QStringLiteral("VPN 类型"));
    styleGroupBox(typeGroup);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    m_vpnTypeCombo = new QComboBox();
    m_vpnTypeCombo->addItem(QStringLiteral("全部"));
    m_vpnTypeCombo->addItem(QStringLiteral("IPSec VPN"));
    m_vpnTypeCombo->addItem(QStringLiteral("SSL VPN"));
    m_vpnTypeCombo->addItem(QStringLiteral("L2TP"));
    m_vpnTypeCombo->addItem(QStringLiteral("GRE"));
    m_vpnTypeCombo->addItem(QStringLiteral("DMVPN"));
    styleCombo(m_vpnTypeCombo);
    typeLayout->addWidget(m_vpnTypeCombo);
    topBar->addWidget(typeGroup, 1);

    mainLayout->addLayout(topBar);

    // ── VPN Overview Cards ──
    auto* overviewFrame = new QFrame();
    overviewFrame->setStyleSheet(
        QStringLiteral(
            "QFrame#overviewFrame {"
            "  background-color: #25262B;"
            "  border: 1px solid #3C3F41;"
            "  border-radius: 6px;"
            "}"
        )
    );
    overviewFrame->setObjectName(QStringLiteral("overviewFrame"));
    auto* overviewLayout = new QHBoxLayout(overviewFrame);
    overviewLayout->setContentsMargins(12, 10, 12, 10);
    overviewLayout->setSpacing(16);

    auto createCard = [](const QString& title, const QString& value, const QString& color) -> QWidget* {
        auto* card = new QFrame();
        card->setStyleSheet(
            QString(QStringLiteral(
                "QFrame {"
                "  background-color: #1E1F22;"
                "  border: 1px solid %1;"
                "  border-radius: 6px;"
                "}"
            )).arg(color)
        );
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 8, 12, 8);
        cardLayout->setSpacing(4);

        auto* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(QStringLiteral("color: #8C8C8C; font-size: 12px;"));
        titleLabel->setAlignment(Qt::AlignCenter);

        auto* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(
            QString(QStringLiteral("color: %1; font-size: 28px; font-weight: bold;")).arg(color)
        );
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setObjectName(QStringLiteral("cardValue"));

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);
        return card;
    };

    auto* totalCard = createCard(QStringLiteral("总隧道数"), QStringLiteral("0"), QStringLiteral("#409EFF"));
    auto* onlineCard = createCard(QStringLiteral("在线"), QStringLiteral("0"), QStringLiteral("#67C23A"));
    auto* offlineCard = createCard(QStringLiteral("离线"), QStringLiteral("0"), QStringLiteral("#E6A23C"));
    auto* faultCard = createCard(QStringLiteral("故障"), QStringLiteral("0"), QStringLiteral("#F56C6C"));

    // 从卡片中获取 QLabel 引用
    m_totalTunnelsLabel = totalCard->findChild<QLabel*>(QStringLiteral("cardValue"));
    m_onlineTunnelsLabel = onlineCard->findChild<QLabel*>(QStringLiteral("cardValue"));
    m_offlineTunnelsLabel = offlineCard->findChild<QLabel*>(QStringLiteral("cardValue"));
    m_faultTunnelsLabel = faultCard->findChild<QLabel*>(QStringLiteral("cardValue"));

    overviewLayout->addWidget(totalCard);
    overviewLayout->addWidget(onlineCard);
    overviewLayout->addWidget(offlineCard);
    overviewLayout->addWidget(faultCard);

    mainLayout->addWidget(overviewFrame);

    // ── Middle: Tab Widget (隧道列表 / IKE SA / IPSec SA / 在线用户) ──
    auto* tabGroup = new QGroupBox(QStringLiteral("VPN 详情"));
    styleGroupBox(tabGroup);
    auto* tabLayout = new QVBoxLayout(tabGroup);
    tabLayout->setContentsMargins(4, 8, 4, 4);

    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        QStringLiteral(
            "QTabWidget::pane {"
            "  border: 1px solid #3C3F41;"
            "  background-color: #1E1F22;"
            "}"
            "QTabBar::tab {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  padding: 6px 16px; border: 1px solid #3C3F41;"
            "  border-bottom: none;"
            "  font-size: 12px; font-weight: bold;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: #1E1F22; color: #409EFF;"
            "  border-bottom: 2px solid #409EFF;"
            "}"
            "QTabBar::tab:hover {"
            "  color: #DCDCDC;"
            "}"
        )
    );

    // ─── 隧道列表 ───
    m_tunnelTable = new QTableWidget(0, 9);
    m_tunnelTable->setHorizontalHeaderLabels({
        QStringLiteral("隧道名称"),
        QStringLiteral("本地地址"),
        QStringLiteral("远程地址"),
        QStringLiteral("加密算法"),
        QStringLiteral("认证算法"),
        QStringLiteral("密钥时长"),
        QStringLiteral("状态"),
        QStringLiteral("在线时长"),
        QStringLiteral("类型")
    });
    m_tunnelTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tunnelTable->setColumnWidth(1, 130);
    m_tunnelTable->setColumnWidth(2, 130);
    m_tunnelTable->setColumnWidth(3, 100);
    m_tunnelTable->setColumnWidth(4, 100);
    m_tunnelTable->setColumnWidth(5, 80);
    m_tunnelTable->setColumnWidth(6, 70);
    m_tunnelTable->setColumnWidth(7, 100);
    m_tunnelTable->setColumnWidth(8, 90);
    styleTable(m_tunnelTable);
    tabWidget->addTab(m_tunnelTable, QStringLiteral("隧道列表"));

    // ─── IKE SA ───
    m_ikeTable = new QTableWidget(0, 7);
    m_ikeTable->setHorizontalHeaderLabels({
        QStringLiteral("对端地址"),
        QStringLiteral("加密算法"),
        QStringLiteral("哈希算法"),
        QStringLiteral("DH 组"),
        QStringLiteral("生命期"),
        QStringLiteral("状态"),
        QStringLiteral("关联隧道")
    });
    m_ikeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ikeTable->setColumnWidth(1, 100);
    m_ikeTable->setColumnWidth(2, 100);
    m_ikeTable->setColumnWidth(3, 80);
    m_ikeTable->setColumnWidth(4, 100);
    m_ikeTable->setColumnWidth(5, 80);
    m_ikeTable->setColumnWidth(6, 140);
    styleTable(m_ikeTable);
    tabWidget->addTab(m_ikeTable, QStringLiteral("IKE SA"));

    // ─── IPSec SA ───
    m_ipsecTable = new QTableWidget(0, 8);
    m_ipsecTable->setHorizontalHeaderLabels({
        QStringLiteral("本地子网"),
        QStringLiteral("远程子网"),
        QStringLiteral("协议"),
        QStringLiteral("加密"),
        QStringLiteral("认证"),
        QStringLiteral("SPI"),
        QStringLiteral("状态"),
        QStringLiteral("关联隧道")
    });
    m_ipsecTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ipsecTable->setColumnWidth(1, 130);
    m_ipsecTable->setColumnWidth(2, 60);
    m_ipsecTable->setColumnWidth(3, 100);
    m_ipsecTable->setColumnWidth(4, 100);
    m_ipsecTable->setColumnWidth(5, 90);
    m_ipsecTable->setColumnWidth(6, 70);
    m_ipsecTable->setColumnWidth(7, 140);
    styleTable(m_ipsecTable);
    tabWidget->addTab(m_ipsecTable, QStringLiteral("IPSec SA"));

    // ─── 在线用户 ───
    m_userTable = new QTableWidget(0, 7);
    m_userTable->setHorizontalHeaderLabels({
        QStringLiteral("用户名"),
        QStringLiteral("来源 IP"),
        QStringLiteral("关联隧道"),
        QStringLiteral("登录时间"),
        QStringLiteral("时长"),
        QStringLiteral("流量"),
        QStringLiteral("状态")
    });
    m_userTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_userTable->setColumnWidth(1, 120);
    m_userTable->setColumnWidth(2, 140);
    m_userTable->setColumnWidth(3, 140);
    m_userTable->setColumnWidth(4, 80);
    m_userTable->setColumnWidth(5, 100);
    m_userTable->setColumnWidth(6, 70);
    styleTable(m_userTable);
    tabWidget->addTab(m_userTable, QStringLiteral("在线用户"));

    tabLayout->addWidget(tabWidget);
    mainLayout->addWidget(tabGroup, 1);

    // ── Bottom: Connection Test Area ──
    auto* bottomBar = new QHBoxLayout();
    bottomBar->setSpacing(12);

    m_testBtn = new QPushButton(QStringLiteral("连接测试 (Ping)"));
    styleButton(m_testBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));
    bottomBar->addWidget(m_testBtn);

    m_exportBtn = new QPushButton(QStringLiteral("导出报告"));
    styleButton(m_exportBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));
    bottomBar->addWidget(m_exportBtn);

    bottomBar->addStretch();

    mainLayout->addLayout(bottomBar);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void VPNWidget::setupConnections()
{
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VPNWidget::onDeviceChanged);
    connect(m_vpnTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateTunnelStatus(); });
    connect(m_testBtn, &QPushButton::clicked, this, &VPNWidget::onTestConnection);
    connect(m_exportBtn, &QPushButton::clicked, this, &VPNWidget::onExport);
    connect(m_refreshTimer, &QTimer::timeout, this, &VPNWidget::onRefresh);
}

// ─── Generate Mock Data ──────────────────────────────────────────────────────

void VPNWidget::generateMockTunnels()
{
    auto* rng = QRandomGenerator::global();

    // 8 条隧道定义
    struct TunnelDef {
        QString name;
        QString localAddr;
        QString remoteAddr;
        QString type;
        bool    siteToSite;
    };

    QList<TunnelDef> tunnelDefs = {
        { QStringLiteral("HQ-to-BJ-IDC"),    QStringLiteral("10.0.0.1"),    QStringLiteral("10.1.0.1"),    QStringLiteral("IPSec VPN"), true  },
        { QStringLiteral("HQ-to-SH-IDC"),    QStringLiteral("10.0.0.1"),    QStringLiteral("10.2.0.1"),    QStringLiteral("IPSec VPN"), true  },
        { QStringLiteral("HQ-to-GZ-IDC"),    QStringLiteral("10.0.0.1"),    QStringLiteral("10.3.0.1"),    QStringLiteral("IPSec VPN"), true  },
        { QStringLiteral("Branch-A-to-HQ"),  QStringLiteral("172.16.0.1"),  QStringLiteral("10.0.0.1"),    QStringLiteral("DMVPN"),     true  },
        { QStringLiteral("Branch-B-to-HQ"),  QStringLiteral("172.16.1.1"),  QStringLiteral("10.0.0.1"),    QStringLiteral("DMVPN"),     true  },
        { QStringLiteral("SSL-VPN-Portal"),  QStringLiteral("100.64.0.1"),  QStringLiteral("0.0.0.0"),     QStringLiteral("SSL VPN"),   false },
        { QStringLiteral("Partner-L2TP"),    QStringLiteral("192.168.0.1"), QStringLiteral("203.0.113.1"),  QStringLiteral("L2TP"),      false },
        { QStringLiteral("DC-GRE-Tunnel"),   QStringLiteral("192.168.0.1"), QStringLiteral("192.168.1.1"),  QStringLiteral("GRE"),       true  },
    };

    QStringList encAlgos = { QStringLiteral("AES-256-GCM"), QStringLiteral("AES-256-CBC"),
                             QStringLiteral("AES-128-GCM"), QStringLiteral("3DES") };
    QStringList authAlgos = { QStringLiteral("SHA256"), QStringLiteral("SHA384"),
                              QStringLiteral("SHA512"), QStringLiteral("MD5") };
    QStringList statuses = { QStringLiteral("在线"), QStringLiteral("在线"), QStringLiteral("在线"),
                             QStringLiteral("在线"), QStringLiteral("在线"), QStringLiteral("离线"),
                             QStringLiteral("在线"), QStringLiteral("故障") };

    for (int i = 0; i < tunnelDefs.size(); ++i) {
        const auto& def = tunnelDefs[i];

        VPNTunnel tunnel;
        tunnel.tunnelName   = def.name;
        tunnel.localAddr    = def.localAddr;
        tunnel.remoteAddr   = def.remoteAddr;
        tunnel.encAlgo      = encAlgos[i % encAlgos.size()];
        tunnel.authAlgo     = authAlgos[i % authAlgos.size()];
        tunnel.keyLifetime  = QStringLiteral("%1h").arg(8 + i * 2);
        tunnel.status       = statuses[i];
        tunnel.type         = def.type;
        tunnel.isSiteToSite = def.siteToSite;

        if (tunnel.status == QStringLiteral("在线")) {
            tunnel.onlineDuration = QStringLiteral("%1d %2h %3m")
                .arg(rng->bounded(1, 90))
                .arg(rng->bounded(0, 24))
                .arg(rng->bounded(0, 60));
        } else {
            tunnel.onlineDuration = QStringLiteral("--");
        }

        m_tunnels.append(tunnel);
    }

    // 生成 IKE SA (每条隧道 ~10 条)
    QStringList dhGroups = { QStringLiteral("DH14"), QStringLiteral("DH15"), QStringLiteral("DH16"),
                             QStringLiteral("DH19"), QStringLiteral("DH20"), QStringLiteral("DH21") };
    QStringList ikeStatuses = { QStringLiteral("已建立"), QStringLiteral("已建立"), QStringLiteral("已建立"),
                                QStringLiteral("已建立"), QStringLiteral("已建立"), QStringLiteral("已建立"),
                                QStringLiteral("已建立"), QStringLiteral("已建立"), QStringLiteral("协商中"),
                                QStringLiteral("已过期") };

    for (const auto& tunnel : m_tunnels) {
        int count = 8 + rng->bounded(0, 4); // 8-11 条
        for (int j = 0; j < count; ++j) {
            IKESA ike;
            ike.tunnelName = tunnel.tunnelName;
            ike.peerAddr   = tunnel.remoteAddr;
            ike.encAlgo    = encAlgos[rng->bounded(encAlgos.size())];
            ike.hashAlgo   = authAlgos[rng->bounded(authAlgos.size())];
            ike.dhGroup    = dhGroups[rng->bounded(dhGroups.size())];
            ike.lifetime   = QStringLiteral("%1s").arg(86400 + rng->bounded(0, 86400));
            ike.status     = (tunnel.status == QStringLiteral("在线"))
                ? ikeStatuses[rng->bounded(8)]  // 偏向"已建立"
                : ikeStatuses[8 + rng->bounded(2)]; // 偏向非正常
            m_ikeSAs.append(ike);
        }
    }

    // 生成 IPSec SA (每条隧道 ~10 条)
    QStringList subnetsLocal = { QStringLiteral("10.0.0.0/8"), QStringLiteral("172.16.0.0/16"),
                                 QStringLiteral("192.168.0.0/24"), QStringLiteral("10.10.0.0/16"),
                                 QStringLiteral("10.20.0.0/16") };
    QStringList subnetsRemote = { QStringLiteral("10.1.0.0/16"), QStringLiteral("10.2.0.0/16"),
                                  QStringLiteral("10.3.0.0/16"), QStringLiteral("172.17.0.0/16"),
                                  QStringLiteral("192.168.100.0/24") };
    QStringList protocols = { QStringLiteral("ESP"), QStringLiteral("ESP"), QStringLiteral("ESP"),
                              QStringLiteral("AH") };
    QStringList ipsecStatuses = { QStringLiteral("活跃"), QStringLiteral("活跃"), QStringLiteral("活跃"),
                                  QStringLiteral("活跃"), QStringLiteral("活跃"), QStringLiteral("活跃"),
                                  QStringLiteral("活跃"), QStringLiteral("活跃"), QStringLiteral("活跃"),
                                  QStringLiteral("已过期") };

    for (const auto& tunnel : m_tunnels) {
        int count = 8 + rng->bounded(0, 4); // 8-11 条
        for (int j = 0; j < count; ++j) {
            IPSecSA sa;
            sa.tunnelName   = tunnel.tunnelName;
            sa.localSubnet  = subnetsLocal[rng->bounded(subnetsLocal.size())];
            sa.remoteSubnet = subnetsRemote[rng->bounded(subnetsRemote.size())];
            sa.protocol     = protocols[rng->bounded(protocols.size())];
            sa.encAlgo      = encAlgos[rng->bounded(encAlgos.size())];
            sa.authAlgo     = authAlgos[rng->bounded(authAlgos.size())];
            sa.spi          = QStringLiteral("0x%1").arg(
                rng->bounded(static_cast<quint32>(0x10000000), static_cast<quint32>(0xFFFFFFFF)), 8, 16, QChar('0'));
            sa.status       = (tunnel.status == QStringLiteral("在线"))
                ? ipsecStatuses[rng->bounded(10)]
                : ipsecStatuses[9 + rng->bounded(1)];
            m_ipsecSAs.append(sa);
        }
    }

    // 生成 15 个在线用户
    QStringList usernames = {
        QStringLiteral("zhangsan"), QStringLiteral("lisi"), QStringLiteral("wangwu"),
        QStringLiteral("zhaoliu"), QStringLiteral("sunqi"), QStringLiteral("zhouba"),
        QStringLiteral("wujiu"), QStringLiteral("zhengshi"), QStringLiteral("wangfang"),
        QStringLiteral("liuyang"), QStringLiteral("chenwei"), QStringLiteral("huangli"),
        QStringLiteral("xuming"), QStringLiteral("linjie"), QStringLiteral("heyun")
    };
    QStringList sourceIPs = {
        QStringLiteral("203.0.113.10"), QStringLiteral("203.0.113.20"), QStringLiteral("198.51.100.5"),
        QStringLiteral("198.51.100.15"), QStringLiteral("192.0.2.50"), QStringLiteral("192.0.2.100"),
        QStringLiteral("203.0.113.55"), QStringLiteral("198.51.100.88"), QStringLiteral("192.0.2.200"),
        QStringLiteral("203.0.113.77"), QStringLiteral("198.51.100.120"), QStringLiteral("192.0.2.30"),
        QStringLiteral("203.0.113.90"), QStringLiteral("198.51.100.66"), QStringLiteral("192.0.2.150")
    };
    QStringList associatedTunnels = {
        QStringLiteral("SSL-VPN-Portal"), QStringLiteral("SSL-VPN-Portal"), QStringLiteral("SSL-VPN-Portal"),
        QStringLiteral("SSL-VPN-Portal"), QStringLiteral("SSL-VPN-Portal"), QStringLiteral("Partner-L2TP"),
        QStringLiteral("Partner-L2TP"), QStringLiteral("Partner-L2TP"), QStringLiteral("SSL-VPN-Portal"),
        QStringLiteral("SSL-VPN-Portal"), QStringLiteral("SSL-VPN-Portal"), QStringLiteral("Partner-L2TP"),
        QStringLiteral("Partner-L2TP"), QStringLiteral("SSL-VPN-Portal"), QStringLiteral("SSL-VPN-Portal")
    };

    QDateTime now = QDateTime::currentDateTime();

    for (int i = 0; i < 15; ++i) {
        VPNUser user;
        user.username          = usernames[i];
        user.sourceIP          = sourceIPs[i];
        user.associatedTunnel  = associatedTunnels[i];
        int minutesAgo = rng->bounded(1, 480);
        user.loginTime         = now.addSecs(-minutesAgo * 60).toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        user.duration          = QStringLiteral("%1h %2m").arg(minutesAgo / 60).arg(minutesAgo % 60);
        double trafficMB = rng->generateDouble() * 5000.0 + 1.0;
        if (trafficMB >= 1000.0) {
            user.traffic = QStringLiteral("%1 GB").arg(QString::number(trafficMB / 1024.0, 'f', 2));
        } else {
            user.traffic = QStringLiteral("%1 MB").arg(QString::number(trafficMB, 'f', 1));
        }
        m_users.append(user);
    }
}

// ─── Update Tunnel Status ────────────────────────────────────────────────────

void VPNWidget::updateTunnelStatus()
{
    QString vpnTypeFilter = m_vpnTypeCombo->currentText();

    // 更新隧道列表
    m_tunnelTable->setRowCount(0);
    int onlineCount = 0, offlineCount = 0, faultCount = 0;

    for (const auto& tunnel : m_tunnels) {
        // 按 VPN 类型过滤
        if (vpnTypeFilter != QStringLiteral("全部") && tunnel.type != vpnTypeFilter) {
            continue;
        }

        int row = m_tunnelTable->rowCount();
        m_tunnelTable->insertRow(row);

        m_tunnelTable->setItem(row, 0, new QTableWidgetItem(tunnel.tunnelName));
        m_tunnelTable->setItem(row, 1, new QTableWidgetItem(tunnel.localAddr));
        m_tunnelTable->setItem(row, 2, new QTableWidgetItem(tunnel.remoteAddr));
        m_tunnelTable->setItem(row, 3, new QTableWidgetItem(tunnel.encAlgo));
        m_tunnelTable->setItem(row, 4, new QTableWidgetItem(tunnel.authAlgo));
        m_tunnelTable->setItem(row, 5, new QTableWidgetItem(tunnel.keyLifetime));

        auto* statusItem = new QTableWidgetItem(tunnel.status);
        if (tunnel.status == QStringLiteral("在线")) {
            statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
            onlineCount++;
        } else if (tunnel.status == QStringLiteral("离线")) {
            statusItem->setForeground(QColor(QStringLiteral("#E6A23C")));
            offlineCount++;
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
            faultCount++;
        }
        m_tunnelTable->setItem(row, 6, statusItem);

        auto* durItem = new QTableWidgetItem(tunnel.onlineDuration);
        durItem->setForeground(QColor(QStringLiteral("#8C8C8C")));
        m_tunnelTable->setItem(row, 7, durItem);

        auto* typeItem = new QTableWidgetItem(tunnel.type);
        if (tunnel.type == QStringLiteral("IPSec VPN")) {
            typeItem->setForeground(QColor(QStringLiteral("#409EFF")));
        } else if (tunnel.type == QStringLiteral("SSL VPN")) {
            typeItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (tunnel.type == QStringLiteral("DMVPN")) {
            typeItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else if (tunnel.type == QStringLiteral("GRE")) {
            typeItem->setForeground(QColor(QStringLiteral("#909399")));
        } else {
            typeItem->setForeground(QColor(QStringLiteral("#DCDCDC")));
        }
        m_tunnelTable->setItem(row, 8, typeItem);
    }

    // 更新概览卡片
    int totalCount = onlineCount + offlineCount + faultCount;
    m_totalTunnelsLabel->setText(QString::number(totalCount));
    m_onlineTunnelsLabel->setText(QString::number(onlineCount));
    m_offlineTunnelsLabel->setText(QString::number(offlineCount));
    m_faultTunnelsLabel->setText(QString::number(faultCount));

    // 更新 IKE SA 表格
    m_ikeTable->setRowCount(0);
    for (const auto& ike : m_ikeSAs) {
        if (vpnTypeFilter != QStringLiteral("全部")) {
            // 按隧道类型过滤 IKE SA
            bool match = false;
            for (const auto& t : m_tunnels) {
                if (t.tunnelName == ike.tunnelName && t.type == vpnTypeFilter) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;
        }

        int row = m_ikeTable->rowCount();
        m_ikeTable->insertRow(row);

        m_ikeTable->setItem(row, 0, new QTableWidgetItem(ike.peerAddr));
        m_ikeTable->setItem(row, 1, new QTableWidgetItem(ike.encAlgo));
        m_ikeTable->setItem(row, 2, new QTableWidgetItem(ike.hashAlgo));
        m_ikeTable->setItem(row, 3, new QTableWidgetItem(ike.dhGroup));
        m_ikeTable->setItem(row, 4, new QTableWidgetItem(ike.lifetime));

        auto* statusItem = new QTableWidgetItem(ike.status);
        if (ike.status == QStringLiteral("已建立")) {
            statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else if (ike.status == QStringLiteral("协商中")) {
            statusItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_ikeTable->setItem(row, 5, statusItem);

        m_ikeTable->setItem(row, 6, new QTableWidgetItem(ike.tunnelName));
    }

    // 更新 IPSec SA 表格
    m_ipsecTable->setRowCount(0);
    for (const auto& sa : m_ipsecSAs) {
        if (vpnTypeFilter != QStringLiteral("全部")) {
            bool match = false;
            for (const auto& t : m_tunnels) {
                if (t.tunnelName == sa.tunnelName && t.type == vpnTypeFilter) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;
        }

        int row = m_ipsecTable->rowCount();
        m_ipsecTable->insertRow(row);

        m_ipsecTable->setItem(row, 0, new QTableWidgetItem(sa.localSubnet));
        m_ipsecTable->setItem(row, 1, new QTableWidgetItem(sa.remoteSubnet));
        m_ipsecTable->setItem(row, 2, new QTableWidgetItem(sa.protocol));
        m_ipsecTable->setItem(row, 3, new QTableWidgetItem(sa.encAlgo));
        m_ipsecTable->setItem(row, 4, new QTableWidgetItem(sa.authAlgo));
        m_ipsecTable->setItem(row, 5, new QTableWidgetItem(sa.spi));

        auto* statusItem = new QTableWidgetItem(sa.status);
        if (sa.status == QStringLiteral("活跃")) {
            statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        }
        m_ipsecTable->setItem(row, 6, statusItem);

        m_ipsecTable->setItem(row, 7, new QTableWidgetItem(sa.tunnelName));
    }

    // 更新在线用户表格
    m_userTable->setRowCount(0);
    for (const auto& user : m_users) {
        if (vpnTypeFilter != QStringLiteral("全部")) {
            bool match = false;
            for (const auto& t : m_tunnels) {
                if (t.tunnelName == user.associatedTunnel && t.type == vpnTypeFilter) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;
        }

        int row = m_userTable->rowCount();
        m_userTable->insertRow(row);

        m_userTable->setItem(row, 0, new QTableWidgetItem(user.username));
        m_userTable->setItem(row, 1, new QTableWidgetItem(user.sourceIP));
        m_userTable->setItem(row, 2, new QTableWidgetItem(user.associatedTunnel));
        m_userTable->setItem(row, 3, new QTableWidgetItem(user.loginTime));
        m_userTable->setItem(row, 4, new QTableWidgetItem(user.duration));
        m_userTable->setItem(row, 5, new QTableWidgetItem(user.traffic));

        auto* statusItem = new QTableWidgetItem(QStringLiteral("在线"));
        statusItem->setForeground(QColor(QStringLiteral("#67C23A")));
        m_userTable->setItem(row, 6, statusItem);
    }
}

// ─── Calculate Health Score ──────────────────────────────────────────────────

void VPNWidget::calculateHealthScore()
{
    int total = m_tunnels.size();
    int online = 0;
    for (const auto& t : m_tunnels) {
        if (t.status == QStringLiteral("在线")) online++;
    }

    double healthRate = (total > 0) ? (double)online / total * 100.0 : 0.0;
    Logger::instance().info(QStringLiteral("VPN Center"),
        QString(QStringLiteral("VPN 健康度: %1% (%2/%3 在线)"))
            .arg(QString::number(healthRate, 'f', 1)).arg(online).arg(total));
}

// ─── Device Changed ──────────────────────────────────────────────────────────

void VPNWidget::onDeviceChanged(int index)
{
    Q_UNUSED(index)
    Logger::instance().debug(QStringLiteral("VPN Center"),
        QString(QStringLiteral("VPN 设备切换: %1")).arg(m_deviceCombo->currentText()));
    updateTunnelStatus();
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void VPNWidget::onRefresh()
{
    auto* rng = QRandomGenerator::global();

    // 随机刷新一些隧道状态模拟变化
    if (!m_tunnels.isEmpty()) {
        int idx = rng->bounded(m_tunnels.size());
        // 不改变故障状态
        if (m_tunnels[idx].status != QStringLiteral("故障")) {
            int r = rng->bounded(100);
            if (r < 85) {
                m_tunnels[idx].status = QStringLiteral("在线");
                m_tunnels[idx].onlineDuration = QStringLiteral("%1d %2h %3m")
                    .arg(rng->bounded(1, 90))
                    .arg(rng->bounded(0, 24))
                    .arg(rng->bounded(0, 60));
            } else if (r < 95) {
                m_tunnels[idx].status = QStringLiteral("离线");
                m_tunnels[idx].onlineDuration = QStringLiteral("--");
            } else {
                m_tunnels[idx].status = QStringLiteral("故障");
                m_tunnels[idx].onlineDuration = QStringLiteral("--");
            }
        }
    }

    updateTunnelStatus();
    calculateHealthScore();

    Logger::instance().debug(QStringLiteral("VPN Center"),
        QStringLiteral("VPN 状态定时刷新完成"));
}

// ─── Test Connection ─────────────────────────────────────────────────────────

void VPNWidget::onTestConnection()
{
    int currentRow = m_tunnelTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先在隧道列表中选中一条隧道进行连接测试。"));
        return;
    }

    QString tunnelName = m_tunnelTable->item(currentRow, 0)->text();
    QString remoteAddr = m_tunnelTable->item(currentRow, 2)->text();

    if (remoteAddr == QStringLiteral("0.0.0.0")) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("SSL VPN 隧道无固定对端地址，无法使用 Ping 测试。"));
        return;
    }

    m_testBtn->setEnabled(false);
    m_testBtn->setText(QStringLiteral("测试中..."));

    Logger::instance().info(QStringLiteral("VPN Center"),
        QString(QStringLiteral("开始连接测试: 隧道=%1, 目标=%2")).arg(tunnelName, remoteAddr));

    auto* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, tunnelName, remoteAddr](int exitCode, QProcess::ExitStatus) {
        QString output = QString::fromUtf8(process->readAll());
        m_testBtn->setEnabled(true);
        m_testBtn->setText(QStringLiteral("连接测试 (Ping)"));

        if (exitCode == 0) {
            Logger::instance().info(QStringLiteral("VPN Center"),
                QString(QStringLiteral("连接测试成功: %1 -> %2")).arg(tunnelName, remoteAddr));
            QMessageBox::information(this, QStringLiteral("连接测试结果"),
                QString(QStringLiteral("隧道 [%1] 连通性测试成功!\n"
                                       "目标地址: %2\n\n%3"))
                    .arg(tunnelName, remoteAddr, output));
        } else {
            Logger::instance().error(QStringLiteral("VPN Center"),
                QString(QStringLiteral("连接测试失败: %1 -> %2")).arg(tunnelName, remoteAddr));
            QMessageBox::warning(this, QStringLiteral("连接测试结果"),
                QString(QStringLiteral("隧道 [%1] 连通性测试失败!\n"
                                       "目标地址: %2\n\n%3"))
                    .arg(tunnelName, remoteAddr, output));
        }

        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, tunnelName](QProcess::ProcessError error) {
        Q_UNUSED(error)
        m_testBtn->setEnabled(true);
        m_testBtn->setText(QStringLiteral("连接测试 (Ping)"));

        Logger::instance().error(QStringLiteral("VPN Center"),
            QString(QStringLiteral("Ping 进程启动失败: %1")).arg(tunnelName));
        QMessageBox::warning(this, QStringLiteral("连接测试失败"),
            QString(QStringLiteral("无法启动 Ping 进程，请检查系统网络配置。")));

        process->deleteLater();
    });

#ifdef Q_OS_WIN
    process->start(QStringLiteral("ping"), { QStringLiteral("-n"), QStringLiteral("4"), remoteAddr });
#else
    process->start(QStringLiteral("ping"), { QStringLiteral("-c"), QStringLiteral("4"), remoteAddr });
#endif
}

// ─── Export Report ───────────────────────────────────────────────────────────

void VPNWidget::onExport()
{
    if (m_tunnels.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("当前没有 VPN 数据可导出。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 VPN 管理报告"),
        QString(QStringLiteral("vpn_report_%1.html"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("HTML 文件 (*.html)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法写入文件: ") + filePath);
        Logger::instance().error(QStringLiteral("VPN Center"),
            QStringLiteral("Failed to open report file: ") + filePath);
        return;
    }

    QTextStream stream(&file);

    int online = 0, offline = 0, fault = 0;
    for (const auto& t : m_tunnels) {
        if (t.status == QStringLiteral("在线")) online++;
        else if (t.status == QStringLiteral("离线")) offline++;
        else fault++;
    }

    stream << QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    stream << QStringLiteral("<meta charset=\"UTF-8\">\n");
    stream << QStringLiteral("<title>VPN 管理中心报告</title>\n");
    stream << QStringLiteral("<style>\n");
    stream << QStringLiteral("body { font-family: 'Microsoft YaHei', sans-serif; "
                              "background: #1E1F22; color: #DCDCDC; padding: 20px; }\n");
    stream << QStringLiteral("h1 { color: #409EFF; text-align: center; }\n");
    stream << QStringLiteral(".meta { text-align: center; color: #8C8C8C; margin-bottom: 10px; font-size: 13px; }\n");
    stream << QStringLiteral(".cards { display: flex; justify-content: center; gap: 20px; margin: 20px 0; }\n");
    stream << QStringLiteral(".card { background: #25262B; border-radius: 8px; padding: 16px 24px; "
                              "text-align: center; min-width: 120px; }\n");
    stream << QStringLiteral(".card .value { font-size: 28px; font-weight: bold; }\n");
    stream << QStringLiteral(".card .label { color: #8C8C8C; font-size: 12px; margin-top: 4px; }\n");
    stream << QStringLiteral("table { width: 100%%; border-collapse: collapse; margin-top: 20px; }\n");
    stream << QStringLiteral("th { background: #25262B; color: #8C8C8C; padding: 10px; "
                              "border-bottom: 2px solid #3C3F41; }\n");
    stream << QStringLiteral("td { padding: 8px; border-bottom: 1px solid #2C2D30; text-align: center; }\n");
    stream << QStringLiteral(".online { color: #67C23A; font-weight: bold; }\n");
    stream << QStringLiteral(".offline { color: #E6A23C; font-weight: bold; }\n");
    stream << QStringLiteral(".fault { color: #F56C6C; font-weight: bold; }\n");
    stream << QStringLiteral("h2 { color: #409EFF; margin-top: 30px; }\n");
    stream << QStringLiteral(".footer { text-align: center; color: #8C8C8C; "
                              "margin-top: 30px; font-size: 12px; }\n");
    stream << QStringLiteral("</style>\n</head>\n<body>\n");

    stream << QStringLiteral("<h1>VPN 管理中心报告</h1>\n");
    stream << QString(QStringLiteral("<p class=\"meta\">设备: %1 | 生成时间: %2</p>\n"))
                .arg(m_deviceCombo->currentText(),
                     QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // 概览卡片
    stream << QStringLiteral("<div class=\"cards\">\n");
    stream << QString(QStringLiteral("<div class=\"card\"><div class=\"value\" style=\"color:#409EFF;\">%1</div>"
                                      "<div class=\"label\">总隧道数</div></div>\n"))
                .arg(m_tunnels.size());
    stream << QString(QStringLiteral("<div class=\"card\"><div class=\"value\" style=\"color:#67C23A;\">%1</div>"
                                      "<div class=\"label\">在线</div></div>\n"))
                .arg(online);
    stream << QString(QStringLiteral("<div class=\"card\"><div class=\"value\" style=\"color:#E6A23C;\">%1</div>"
                                      "<div class=\"label\">离线</div></div>\n"))
                .arg(offline);
    stream << QString(QStringLiteral("<div class=\"card\"><div class=\"value\" style=\"color:#F56C6C;\">%1</div>"
                                      "<div class=\"label\">故障</div></div>\n"))
                .arg(fault);
    stream << QStringLiteral("</div>\n");

    // 隧道列表
    stream << QStringLiteral("<h2>隧道列表</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>隧道名称</th><th>本地地址</th><th>远程地址</th>"
                              "<th>加密算法</th><th>认证算法</th><th>密钥时长</th><th>状态</th>"
                              "<th>在线时长</th><th>类型</th></tr>\n");
    for (const auto& tunnel : m_tunnels) {
        QString statusClass;
        if (tunnel.status == QStringLiteral("在线")) {
            statusClass = QStringLiteral("online");
        } else if (tunnel.status == QStringLiteral("离线")) {
            statusClass = QStringLiteral("offline");
        } else {
            statusClass = QStringLiteral("fault");
        }
        stream << QString(QStringLiteral("<tr>"
                                          "<td>%1</td><td>%2</td><td>%3</td>"
                                          "<td>%4</td><td>%5</td><td>%6</td>"
                                          "<td class=\"%7\">%8</td><td>%9</td><td>%10</td>"
                                          "</tr>\n"))
                    .arg(tunnel.tunnelName, tunnel.localAddr, tunnel.remoteAddr,
                         tunnel.encAlgo, tunnel.authAlgo, tunnel.keyLifetime,
                         statusClass, tunnel.status, tunnel.onlineDuration, tunnel.type);
    }
    stream << QStringLiteral("</table>\n");

    // 在线用户
    stream << QStringLiteral("<h2>在线用户 (Top 15)</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>用户名</th><th>来源 IP</th><th>关联隧道</th>"
                              "<th>登录时间</th><th>时长</th><th>流量</th><th>状态</th></tr>\n");
    int maxUsers = qMin(15, m_users.size());
    for (int i = 0; i < maxUsers; ++i) {
        const auto& user = m_users[i];
        stream << QString(QStringLiteral("<tr>"
                                          "<td>%1</td><td>%2</td><td>%3</td>"
                                          "<td>%4</td><td>%5</td><td>%6</td>"
                                          "<td class=\"online\">在线</td>"
                                          "</tr>\n"))
                    .arg(user.username, user.sourceIP, user.associatedTunnel,
                         user.loginTime, user.duration, user.traffic);
    }
    stream << QStringLiteral("</table>\n");

    stream << QString(QStringLiteral("<div class=\"footer\">WukongToolkit — 网络工程师工具箱 | VPN 管理中心</div>\n"));
    stream << QStringLiteral("</body>\n</html>\n");

    file.close();

    Logger::instance().info(QStringLiteral("VPN Center"),
        QString(QStringLiteral("VPN 管理报告已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("VPN 管理报告已保存到:\n%1")).arg(filePath));
}