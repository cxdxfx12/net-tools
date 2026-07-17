#include "ui/MainWindow.h"
#include "ui/ThemeManager.h"
#include "ui/StatusBarManager.h"
#include "log/Logger.h"
#include "config/ConfigManager.h"
#include "terminal/SSHConnectDialog.h"
#include "terminal/SSHManager.h"
#include "terminal/SSHSession.h"
#include "terminal/TerminalWidget.h"
#include "terminal/CommandHistory.h"
#include "ping/PingWidget.h"
#include "traceroute/TracerouteWidget.h"
#include "network/DNSWidget.h"
#include "network/IPScannerWidget.h"
#include "network/PortScannerWidget.h"
#include "snmp/SNMPWidget.h"
#include "ftp/SFTPWidget.h"
#include "ftp/TFTPWidget.h"
#include "terminal/TelnetWidget.h"
#include "serial/SerialWidget.h"
#include "network/SyslogWidget.h"
#include "network/FlowWidget.h"
#include "network/PacketCaptureWidget.h"
#include "network/ProtocolAnalyzerWidget.h"
#include "network/ARPWidget.h"
#include "network/DHCPWidget.h"
#include "http/HttpWidget.h"
#include "network/WOLWidget.h"
#include "network/DeviceCenter.h"
#include "mac/MacToolkitWidget.h"
#include "ipcalc/IpCalculatorWidget.h"
#include "planner/PlannerWidget.h"
#include "dns/DnsToolkitWidget.h"
#include "snmp/SNMPMonitorWidget.h"
#include "mib/MibWidget.h"
#include "monitor/MonitorCenterWidget.h"
#include "alarm/AlarmCenterWidget.h"
#include "syslog/SyslogCenterWidget.h"
#include "configcenter/ConfigCenterWidget.h"
#include "inspection/InspectionWidget.h"
#include "report/ReportWidget.h"
#include "discovery/DiscoveryWidget.h"
#include "security/SecurityWidget.h"
#include "topology/TopologyWidget.h"
#include "plugins/PluginWidget.h"
#include "ha/HAWidget.h"
#include "vpn/VPNWidget.h"
#include "core/AppCore.h"
#include "traffic/TrafficWidget.h"
#include "wireless/WirelessWidget.h"
#include "ipv6/IPv6Widget.h"
#include "aaa/AAAWidget.h"
#include "performance/PerformanceWidget.h"
#include "flow/FlowCollectorWidget.h"
#include "qos/QoSWidget.h"
#include "asset/AssetWidget.h"
#include "dashboard/DashboardWidget.h"
#include "automation/AutomationWidget.h"
#include "settings/SettingsWidget.h"
#include "network/WhoisWidget.h"
#include "ftp/FTPWidget.h"
#include "ftp/SCPWidget.h"
#include "mqtt/MQTTWidget.h"
#include "modbus/ModbusWidget.h"
#include "configcenter/ConfigDiffWidget.h"
#include "configcenter/ConfigGenWidget.h"
#include "mac/MacLookupWidget.h"
#include "log/LogViewerWidget.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QFrame>
#include <QPainter>
#include <QLinearGradient>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8("悟空网络工程师工具箱"));
    resize(1400, 900);
    setMinimumSize(1100, 680);

    setupMenuBar();
    setupToolBar();
    setupMenuTree();
    setupWorkArea();
    setupLogDock();
    setupStatusBar();
    setupConnections();

    connect(&Logger::instance(), &Logger::logMessage,
            this, &MainWindow::appendLog);

    Logger::instance().info("APP", "MainWindow initialized");
}

MainWindow::~MainWindow()
{
    saveLayout();
}

// ═══════════════════════════════════════════════════════════════════════════
// MENU BAR
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupMenuBar()
{
    QMenuBar* mb = menuBar();

    QMenu* fileMenu = mb->addMenu(QString::fromUtf8("文件(&F)"));
    fileMenu->addAction(QString::fromUtf8("新建会话(&N)"), QKeySequence::New, this, &MainWindow::openSSHDialog);
    fileMenu->addAction(QString::fromUtf8("打开配置(&O)"), QKeySequence::Open, this, []() {
        Logger::instance().info("APP", "Open config requested");
    });
    fileMenu->addSeparator();
    fileMenu->addAction(QString::fromUtf8("保存布局(&S)"), QKeySequence::Save, this, &MainWindow::saveLayout);
    fileMenu->addSeparator();
    fileMenu->addAction(QString::fromUtf8("退出(&X)"), QKeySequence::Quit, this, &QMainWindow::close);

    QMenu* viewMenu = mb->addMenu(QString::fromUtf8("视图(&V)"));
    viewMenu->addAction(QString::fromUtf8("显示导航面板"), this, [this]() {
        m_menuDock->setVisible(!m_menuDock->isVisible());
    });
    viewMenu->addAction(QString::fromUtf8("显示日志面板"), this, [this]() {
        m_logDock->setVisible(!m_logDock->isVisible());
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QString::fromUtf8("切换主题(&T)"), this, []() {
        ThemeManager::instance().cycleTheme();
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QString::fromUtf8("恢复默认布局"), this, [this]() {
        m_menuDock->setVisible(true);
        m_logDock->setVisible(true);
        Logger::instance().info("APP", "Layout reset to default");
    });

    QMenu* toolsMenu = mb->addMenu(QString::fromUtf8("工具(&T)"));
    toolsMenu->addAction("Ping", this, []() {
        Logger::instance().info("APP", "Ping tool requested");
    });
    toolsMenu->addAction("Traceroute", this, []() {
        Logger::instance().info("APP", "Traceroute tool requested");
    });
    toolsMenu->addAction(QString::fromUtf8("SSH 终端"), this, &MainWindow::openSSHDialog);
    toolsMenu->addSeparator();
    toolsMenu->addAction(QString::fromUtf8("设置(&S)"), this, []() {
        Logger::instance().info("APP", "Settings requested");
    });

    QMenu* helpMenu = mb->addMenu(QString::fromUtf8("帮助(&H)"));
    helpMenu->addAction(QString::fromUtf8("关于(&A)"), this, [this]() {
        QMessageBox::about(this, QString::fromUtf8("关于悟空工具箱"),
            QString::fromUtf8("悟空网络工程师工具箱 v1.0.0\n\n"
                              "一站式网络工程解决方案\n"
                              "集成终端 · 监控 · 诊断 · 自动化"));
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// TOOLBAR
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar(QString::fromUtf8("主工具栏"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setObjectName("mainToolBar");

    QAction* sshAction = toolbar->addAction(QString::fromUtf8("SSH 连接"));
    sshAction->setToolTip(QString::fromUtf8("新建 SSH 会话"));
    connect(sshAction, &QAction::triggered, this, &MainWindow::openSSHDialog);

    QAction* pingAction = toolbar->addAction(QString::fromUtf8("Ping"));
    pingAction->setToolTip(QString::fromUtf8("网络连通性测试"));
    connect(pingAction, &QAction::triggered, this, []() {
        Logger::instance().info("APP", "Ping toolbar clicked");
    });

    toolbar->addSeparator();

    QAction* themeAction = toolbar->addAction(QString::fromUtf8("主题"));
    themeAction->setToolTip(QString::fromUtf8("循环切换主题：暗夜蓝 → 森林绿 → 日落橙"));
    connect(themeAction, &QAction::triggered, this, []() {
        ThemeManager::instance().cycleTheme();
    });

    QAction* settingsAction = toolbar->addAction(QString::fromUtf8("设置"));
    settingsAction->setToolTip(QString::fromUtf8("系统设置"));
    connect(settingsAction, &QAction::triggered, this, []() {
        Logger::instance().info("APP", "Settings toolbar clicked");
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// MENU TREE (Left Navigation Sidebar)
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupMenuTree()
{
    m_menuDock = new QDockWidget(QString::fromUtf8("导航"), this);
    m_menuDock->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetClosable);
    m_menuDock->setMinimumWidth(210);
    m_menuDock->setMaximumWidth(320);

    m_menuTree = new QTreeWidget();
    m_menuTree->setHeaderHidden(true);
    m_menuTree->setIndentation(12);
    m_menuTree->setIconSize(QSize(16, 16));
    m_menuTree->setAnimated(true);
    m_menuTree->setRootIsDecorated(true);
    m_menuTree->setFrameShape(QFrame::NoFrame);

    auto addCategory = [this](const QString& name) -> QTreeWidgetItem* {
        auto* item = new QTreeWidgetItem(m_menuTree);
        item->setText(0, name);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        QFont font = item->font(0);
        font.setBold(true);
        font.setPointSize(11);
        item->setFont(0, font);
        item->setForeground(0, ThemeManager::textSecondary());
        item->setExpanded(true);
        item->setSizeHint(0, QSize(0, 28));
        return item;
    };

    auto addTool = [](QTreeWidgetItem* parent, const QString& name) -> QTreeWidgetItem* {
        auto* item = new QTreeWidgetItem(parent);
        item->setText(0, name);
        item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setSizeHint(0, QSize(0, 32));
        return item;
    };

    // 网络工具
    auto* netTools = addCategory(QString::fromUtf8("网络工具"));
    addTool(netTools, "Ping");
    addTool(netTools, "Traceroute");
    addTool(netTools, "DNS");
    addTool(netTools, QString::fromUtf8("DNS 诊断"));
    addTool(netTools, "Whois");
    addTool(netTools, "IP Scanner");
    addTool(netTools, "Port Scanner");
    addTool(netTools, "Syslog");
    addTool(netTools, "Flow Analyzer");
    addTool(netTools, "Packet Capture");
    addTool(netTools, "ARP");
    addTool(netTools, "DHCP");
    addTool(netTools, "WOL");

    // 终端
    auto* terminal = addCategory(QString::fromUtf8("终端"));
    addTool(terminal, "SSH");
    addTool(terminal, "Telnet");
    addTool(terminal, "Serial");

    // 文件传输
    auto* fileTransfer = addCategory(QString::fromUtf8("文件传输"));
    addTool(fileTransfer, "FTP");
    addTool(fileTransfer, "SFTP");
    addTool(fileTransfer, "TFTP");
    addTool(fileTransfer, "SCP");

    // 网络协议
    auto* protocols = addCategory(QString::fromUtf8("网络协议"));
    addTool(protocols, "SNMP");
    addTool(protocols, QString::fromUtf8("SNMP 监控"));
    addTool(protocols, QString::fromUtf8("MIB 浏览器"));
    addTool(protocols, "DHCP");
    addTool(protocols, "HTTP");
    addTool(protocols, "Protocol Analyzer");

    // 监控
    auto* monitor = addCategory(QString::fromUtf8("监控"));
    addTool(monitor, QString::fromUtf8("设备监控"));
    addTool(monitor, QString::fromUtf8("告警中心"));
    addTool(monitor, QString::fromUtf8("日志中心"));
    addTool(monitor, QString::fromUtf8("运维总览"));
    addTool(monitor, QString::fromUtf8("性能分析"));
    addTool(monitor, QString::fromUtf8("流量分析"));
    addTool(monitor, QString::fromUtf8("QoS 分析"));

    // 运维
    auto* ops = addCategory(QString::fromUtf8("运维"));
    addTool(ops, QString::fromUtf8("巡检中心"));
    addTool(ops, QString::fromUtf8("安全审计中心"));
    addTool(ops, QString::fromUtf8("报表中心"));
    addTool(ops, QString::fromUtf8("设备发现"));
    addTool(ops, QString::fromUtf8("网络拓扑"));
    addTool(ops, QString::fromUtf8("HA 高可用性管理"));
    addTool(ops, QString::fromUtf8("VPN 管理中心"));
    addTool(ops, QString::fromUtf8("自动化运维"));
    addTool(ops, QString::fromUtf8("资产管理"));
    addTool(ops, QString::fromUtf8("AAA 认证管理"));
    addTool(ops, QString::fromUtf8("流采集器"));

    // 无线
    auto* wireless = addCategory(QString::fromUtf8("无线"));
    addTool(wireless, QString::fromUtf8("无线管理"));

    // IPv6
    auto* ipv6 = addCategory("IPv6");
    addTool(ipv6, QString::fromUtf8("IPv6 管理"));

    // 网络规划
    auto* planning = addCategory(QString::fromUtf8("网络规划"));
    addTool(planning, QString::fromUtf8("IP 计算器"));
    addTool(planning, QString::fromUtf8("子网规划"));
    addTool(planning, QString::fromUtf8("MAC 地址工具"));

    // 工业协议
    auto* industrial = addCategory(QString::fromUtf8("工业协议"));
    addTool(industrial, "MQTT");
    addTool(industrial, "Modbus");

    // 工具
    auto* tools = addCategory(QString::fromUtf8("工具"));
    addTool(tools, QString::fromUtf8("配置中心"));
    addTool(tools, QString::fromUtf8("配置Diff"));
    addTool(tools, QString::fromUtf8("配置生成"));
    addTool(tools, QString::fromUtf8("MAC查询"));

    // 系统
    auto* sysCategory = addCategory(QString::fromUtf8("系统"));
    addTool(sysCategory, QString::fromUtf8("系统设置"));
    addTool(sysCategory, QString::fromUtf8("日志"));
    addTool(sysCategory, QString::fromUtf8("插件管理"));
    addTool(sysCategory, "Device Center");

    m_menuDock->setWidget(m_menuTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_menuDock);
}

// ═══════════════════════════════════════════════════════════════════════════
// WORK AREA (Center) — Modern Welcome Page
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupWorkArea()
{
    m_workArea = new QTabWidget();
    m_workArea->setTabsClosable(true);
    m_workArea->setMovable(true);
    m_workArea->setDocumentMode(true);
    m_workArea->setObjectName("workArea");

    // ── Modern welcome page ──
    auto* welcomeWidget = new QWidget();
    welcomeWidget->setObjectName("welcomePage");
    auto* outerLayout = new QVBoxLayout(welcomeWidget);
    outerLayout->setAlignment(Qt::AlignCenter);
    outerLayout->setContentsMargins(60, 40, 60, 40);

    // Logo area
    auto* logoContainer = new QWidget();
    logoContainer->setFixedSize(80, 80);
    logoContainer->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #58A6FF, stop:1 #3FB950);"
        "border-radius: 20px;"
    );
    auto* logoLayout = new QVBoxLayout(logoContainer);
    logoLayout->setAlignment(Qt::AlignCenter);
    auto* logoText = new QLabel("W");
    logoText->setStyleSheet("color: white; font-size: 36px; font-weight: 900; background: transparent;");
    logoText->setAlignment(Qt::AlignCenter);
    logoLayout->addWidget(logoText);

    auto* logoWrapper = new QHBoxLayout();
    logoWrapper->setAlignment(Qt::AlignCenter);
    logoWrapper->addWidget(logoContainer);
    outerLayout->addLayout(logoWrapper);
    outerLayout->addSpacing(24);

    // Title
    auto* welcomeLabel = new QLabel(QString::fromUtf8("悟空网络工程师工具箱"));
    welcomeLabel->setStyleSheet("font-size: 32px; font-weight: 800; color: #E6EDF3; letter-spacing: -0.5px;");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    outerLayout->addWidget(welcomeLabel);

    outerLayout->addSpacing(8);

    // Subtitle
    auto* welcomeSub = new QLabel(QString::fromUtf8("一站式网络工程解决方案"));
    welcomeSub->setStyleSheet("font-size: 15px; color: #8B949E; font-weight: 400;");
    welcomeSub->setAlignment(Qt::AlignCenter);
    outerLayout->addWidget(welcomeSub);

    outerLayout->addSpacing(32);

    // Quick actions grid
    auto* quickGrid = new QHBoxLayout();
    quickGrid->setSpacing(12);
    quickGrid->setAlignment(Qt::AlignCenter);

    auto makeQuickCard = [](const QString& icon, const QString& title, const QString& desc,
                             const QString& accent) -> QFrame* {
        auto* card = new QFrame();
        card->setFixedSize(180, 130);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(
            QString("QFrame {"
                    "  background-color: #161B22;"
                    "  border: 1px solid #30363D;"
                    "  border-radius: 10px;"
                    "  padding: 16px;"
                    "}"
                    "QFrame:hover {"
                    "  border-color: %1;"
                    "  background-color: #1C2128;"
                    "}").arg(accent)
        );
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(8);

        auto* iconLbl = new QLabel(icon);
        iconLbl->setStyleSheet(QString("font-size: 28px; background: transparent; border: none;"));
        cl->addWidget(iconLbl);

        auto* titleLbl = new QLabel(title);
        titleLbl->setStyleSheet(QString("font-size: 13px; font-weight: 600; color: #E6EDF3; background: transparent; border: none;"));
        cl->addWidget(titleLbl);

        auto* descLbl = new QLabel(desc);
        descLbl->setWordWrap(true);
        descLbl->setStyleSheet("font-size: 11px; color: #8B949E; background: transparent; border: none; line-height: 1.4;");
        cl->addWidget(descLbl);

        cl->addStretch();
        return card;
    };

    auto* sshCard = makeQuickCard(QString::fromUtf8("\xF0\x9F\x96\xA5"), "SSH",
                                  QString::fromUtf8("远程终端连接\n管理网络设备"), "#58A6FF");
    auto* pingCard = makeQuickCard(QString::fromUtf8("\xF0\x9F\x93\xA1"), "Ping",
                                   QString::fromUtf8("网络连通性测试\n延迟与丢包分析"), "#3FB950");
    auto* monitorCard = makeQuickCard(QString::fromUtf8("\xF0\x9F\x93\x8A"), QString::fromUtf8("运维总览"),
                                      QString::fromUtf8("全局设备状态\n性能与告警面板"), "#D29922");
    auto* configCard = makeQuickCard(QString::fromUtf8("\xE2\x9A\x99"), QString::fromUtf8("配置管理"),
                                     QString::fromUtf8("设备配置生成\n差异对比与备份"), "#F85149");

    quickGrid->addWidget(sshCard);
    quickGrid->addWidget(pingCard);
    quickGrid->addWidget(monitorCard);
    quickGrid->addWidget(configCard);

    outerLayout->addLayout(quickGrid);
    outerLayout->addSpacing(36);

    // Version
    auto* versionLabel = new QLabel("v1.0.0 · Build 2026");
    versionLabel->setStyleSheet("font-size: 11px; color: #484F58; letter-spacing: 1px;");
    versionLabel->setAlignment(Qt::AlignCenter);
    outerLayout->addWidget(versionLabel);

    m_workArea->addTab(welcomeWidget, QString::fromUtf8("欢迎"));

    connect(m_workArea, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index <= 0) return;

        QWidget* w = m_workArea->widget(index);
        if (m_sshTabs.contains(w)) {
            return;
        }
        m_workArea->removeTab(index);
        w->deleteLater();
    });

    setCentralWidget(m_workArea);
}

// ═══════════════════════════════════════════════════════════════════════════
// LOG DOCK (Bottom Panel)
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupLogDock()
{
    m_logDock = new QDockWidget(QString::fromUtf8("日志输出"), this);
    m_logDock->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetClosable);

    m_logView = new QPlainTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setFrameShape(QFrame::NoFrame);
    m_logView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_logView->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #0D1117;"
        "  color: #E6EDF3;"
        "  font-family: \"JetBrains Mono\", \"SF Mono\", \"Cascadia Code\", \"Consolas\", monospace;"
        "  font-size: 11px;"
        "  border: none;"
        "  border-top: 1px solid #21262D;"
        "  padding: 8px 12px;"
        "  line-height: 1.5;"
        "}"
    );

    m_logDock->setWidget(m_logView);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
}

// ═══════════════════════════════════════════════════════════════════════════
// STATUSBAR
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupStatusBar()
{
    m_statusBarManager = new StatusBarManager(statusBar(), this);
    m_statusBarManager->setVersion("v1.0.0");
    m_statusBarManager->startMonitoring();
}

// ═══════════════════════════════════════════════════════════════════════════
// CONNECTIONS
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupConnections()
{
    connect(m_menuTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onMenuTreeItemClicked);
    connect(m_menuTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onMenuTreeItemDoubleClicked);
}

void MainWindow::onMenuTreeItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    if (!item || item->childCount() > 0) return;

    QString toolName = item->text(0);
    Logger::instance().info("APP", "Menu clicked: " + toolName);
}

void MainWindow::onMenuTreeItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    if (!item || item->childCount() > 0) return;

    QString toolName = item->text(0);
    Logger::instance().info("APP", "Tool launched: " + toolName);

    // ── Tool routing ──
    auto addWidget = [this](QWidget* w, const QString& title) {
        m_workArea->addTab(w, title);
        m_workArea->setCurrentWidget(w);
    };

    if (toolName == "SSH") { openSSHDialog(); return; }
    if (toolName == "Ping") { addWidget(new PingWidget(), toolName); return; }
    if (toolName == "Traceroute") { addWidget(new TracerouteWidget(), toolName); return; }
    if (toolName == "DNS") { addWidget(new DNSWidget(), toolName); return; }
    if (toolName == "IP Scanner") { addWidget(new IPScannerWidget(), toolName); return; }
    if (toolName == "Port Scanner") { addWidget(new PortScannerWidget(), toolName); return; }
    if (toolName == "SNMP") { addWidget(new SNMPWidget(), toolName); return; }
    if (toolName == "SFTP") { addWidget(new SFTPWidget(), toolName); return; }
    if (toolName == "TFTP") { addWidget(new TFTPWidget(), toolName); return; }
    if (toolName == "Telnet") { addWidget(new TelnetWidget(), toolName); return; }
    if (toolName == "Serial") { addWidget(new SerialWidget(), toolName); return; }
    if (toolName == "Syslog") { addWidget(new SyslogWidget(), toolName); return; }
    if (toolName == "Flow Analyzer") { addWidget(new FlowWidget(), toolName); return; }
    if (toolName == "Packet Capture") { addWidget(new PacketCaptureWidget(), toolName); return; }
    if (toolName == "Protocol Analyzer") { addWidget(new ProtocolAnalyzerWidget(), toolName); return; }
    if (toolName == "ARP") { addWidget(new ARPWidget(), toolName); return; }
    if (toolName == "DHCP") { addWidget(new DHCPWidget(), toolName); return; }
    if (toolName == "HTTP") { addWidget(new HttpWidget(), toolName); return; }
    if (toolName == "WOL") { addWidget(new WOLWidget(), toolName); return; }
    if (toolName == "Device Center") { addWidget(new DeviceCenter(), toolName); return; }
    if (toolName == QString::fromUtf8("MAC 地址工具")) { addWidget(new MacToolkitWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("IP 计算器")) { addWidget(new IpCalculatorWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("子网规划")) { addWidget(new PlannerWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("DNS 诊断")) { addWidget(new DnsToolkitWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("SNMP 监控")) { addWidget(new SNMPMonitorWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("MIB 浏览器")) { addWidget(new MibWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("设备监控")) { addWidget(new MonitorCenterWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("告警中心")) { addWidget(new AlarmCenterWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("日志中心")) { addWidget(new SyslogCenterWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("配置中心")) { addWidget(new ConfigCenterWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("巡检中心")) { addWidget(new InspectionWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("安全审计中心")) { addWidget(new SecurityWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("报表中心")) { addWidget(new ReportWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("设备发现")) { addWidget(new DiscoveryWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("网络拓扑")) { addWidget(new TopologyWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("HA 高可用性管理")) { addWidget(new HAWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("VPN 管理中心")) { addWidget(new VPNWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("插件管理")) { addWidget(new PluginWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("运维总览")) { addWidget(new DashboardWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("性能分析")) { addWidget(new PerformanceWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("流量分析")) { addWidget(new TrafficWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("QoS 分析")) { addWidget(new QoSWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("自动化运维")) { addWidget(new AutomationWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("资产管理")) { addWidget(new AssetWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("AAA 认证管理")) { addWidget(new AAAWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("流采集器")) { addWidget(new FlowCollectorWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("无线管理")) { addWidget(new WirelessWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("IPv6 管理")) { addWidget(new IPv6Widget(), toolName); return; }
    if (toolName == QString::fromUtf8("系统设置")) { addWidget(new SettingsWidget(), toolName); return; }
    if (toolName == "Whois") { addWidget(new WhoisWidget(), toolName); return; }
    if (toolName == "FTP") { addWidget(new FTPWidget(), toolName); return; }
    if (toolName == "SCP") { addWidget(new SCPWidget(), toolName); return; }
    if (toolName == "MQTT") { addWidget(new MQTTWidget(), toolName); return; }
    if (toolName == "Modbus") { addWidget(new ModbusWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("配置Diff")) { addWidget(new ConfigDiffWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("配置生成")) { addWidget(new ConfigGenWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("MAC查询")) { addWidget(new MacLookupWidget(), toolName); return; }
    if (toolName == QString::fromUtf8("日志")) { addWidget(new LogViewerWidget(), toolName); return; }
}

// ═══════════════════════════════════════════════════════════════════════════
// LOG DISPLAY
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::appendLog(const QString& time, const QString& level,
                           const QString& module, const QString& message)
{
    QString color;
    QString badge;
    if (level == "INFO") {
        color = "#3FB950";
        badge = QString::fromUtf8("●");
    } else if (level == "WARNING") {
        color = "#D29922";
        badge = QString::fromUtf8("▲");
    } else if (level == "ERROR") {
        color = "#F85149";
        badge = QString::fromUtf8("■");
    } else {
        color = "#8B949E";
        badge = QString::fromUtf8("○");
    }

    QString html = QString(
        "<span style='color:#484F58;'>%1</span> "
        "<span style='color:%3;'>%4</span> "
        "<span style='color:#58A6FF; font-weight:600;'>[%2]</span> "
        "<span style='color:#E6EDF3;'>%5</span>"
    ).arg(time, module, color, badge, message.toHtmlEscaped());

    m_logView->appendHtml(html);
}

// ═══════════════════════════════════════════════════════════════════════════
// SSH INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::openSSHDialog()
{
    SSHConnectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        SSHConfig cfg = dialog.config();
        openSSHTab(cfg.host, cfg.port, cfg.username, cfg.password);
    }
}

void MainWindow::openSSHTab(const QString& host, int port,
                            const QString& user, const QString& password)
{
    auto* terminal = new TerminalWidget();
    auto* history = new CommandHistory(terminal);

    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar
    auto* header = new QWidget();
    header->setStyleSheet("background-color: #161B22; border-bottom: 1px solid #21262D;");
    header->setFixedHeight(36);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);
    headerLayout->setSpacing(10);

    auto* hostLabel = new QLabel(QString("%1:%2").arg(host).arg(port));
    hostLabel->setStyleSheet("color: #58A6FF; font-weight: 600; font-size: 12px;");
    auto* protoLabel = new QLabel("SSH");
    protoLabel->setStyleSheet("color: #3FB950; font-size: 10px; font-weight: 600; "
                               "background-color: rgba(63,185,80,0.15); padding: 2px 8px; border-radius: 10px;");
    auto* statusLabel = new QLabel(QString::fromUtf8("连接中..."));
    statusLabel->setStyleSheet("color: #D29922; font-size: 11px;");

    headerLayout->addWidget(hostLabel);
    headerLayout->addWidget(protoLabel);
    headerLayout->addWidget(statusLabel);
    headerLayout->addStretch();

    auto* encodingLabel = new QLabel("UTF-8");
    encodingLabel->setStyleSheet("color: #484F58; font-size: 10px;");
    headerLayout->addWidget(encodingLabel);

    layout->addWidget(header);
    layout->addWidget(terminal);

    QString tabTitle = QString("%1@%2").arg(user, host);
    int tabIndex = m_workArea->addTab(container, tabTitle);
    m_workArea->setCurrentIndex(tabIndex);

    SSHConfig cfg;
    cfg.host = host;
    cfg.port = port;
    cfg.username = user;
    cfg.password = password;

    auto* session = SSHManager::instance().createSession(cfg);

    SSHTabInfo info;
    info.terminal = terminal;
    info.history = history;
    info.sessionId = session->id();
    m_sshTabs.insert(container, info);

    connect(terminal, &TerminalWidget::dataReady, session, &SSHSession::sendData);
    connect(session, &SSHSession::dataReceived, terminal, &TerminalWidget::appendOutput);

    connect(session, &SSHSession::stateChanged, container, [statusLabel, terminal](SessionState state) {
        switch (state) {
        case SessionState::Connecting:
            statusLabel->setText(QString::fromUtf8("连接中..."));
            statusLabel->setStyleSheet("color: #D29922; font-size: 11px;");
            break;
        case SessionState::Connected:
        case SessionState::Running:
            statusLabel->setText(QString::fromUtf8("已连接"));
            statusLabel->setStyleSheet("color: #3FB950; font-size: 11px;");
            terminal->setConnected(true);
            break;
        case SessionState::Disconnecting:
            statusLabel->setText(QString::fromUtf8("断开中..."));
            statusLabel->setStyleSheet("color: #D29922; font-size: 11px;");
            break;
        case SessionState::Closed:
            statusLabel->setText(QString::fromUtf8("已断开"));
            statusLabel->setStyleSheet("color: #F85149; font-size: 11px;");
            terminal->setConnected(false);
            break;
        case SessionState::Error:
            statusLabel->setText(QString::fromUtf8("错误"));
            statusLabel->setStyleSheet("color: #F85149; font-size: 11px;");
            terminal->setConnected(false);
            break;
        default:
            break;
        }
    });

    connect(session, &SSHSession::connectionError, container, [statusLabel](const QString& error) {
        Q_UNUSED(error)
        statusLabel->setText(QString::fromUtf8("连接失败"));
        statusLabel->setStyleSheet("color: #F85149; font-size: 11px;");
    });

    connect(session, &SSHSession::reconnecting, container, [statusLabel](int attempt) {
        statusLabel->setText(QString::fromUtf8("重连中 (%1/5)").arg(attempt));
        statusLabel->setStyleSheet("color: #D29922; font-size: 11px;");
    });

    connect(m_workArea, &QTabWidget::tabCloseRequested, this, [this, container](int index) {
        if (m_workArea->widget(index) == container) {
            if (m_sshTabs.contains(container)) {
                QString sessionId = m_sshTabs[container].sessionId;
                SSHManager::instance().closeSession(sessionId);
                m_sshTabs.remove(container);
            }
            m_workArea->removeTab(index);
            container->deleteLater();
        }
    });

    session->connectToHost();
    Logger::instance().info("SSH", QString("SSH tab opened: %1@%2:%3").arg(user, host).arg(port));
}

// ═══════════════════════════════════════════════════════════════════════════
// LAYOUT PERSISTENCE
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::saveLayout()
{
    auto& config = ConfigManager::instance();
    config.setValue("window/geometry", QString::fromLatin1(saveGeometry().toBase64()));
    config.setValue("window/state", QString::fromLatin1(saveState().toBase64()));
    Logger::instance().debug("APP", "Layout saved");
}

void MainWindow::restoreLayout()
{
    auto& config = ConfigManager::instance();
    QString geo = config.value("window/geometry").toString();
    QString state = config.value("window/state").toString();

    if (!geo.isEmpty()) {
        restoreGeometry(QByteArray::fromBase64(geo.toLatin1()));
    }
    if (!state.isEmpty()) {
        restoreState(QByteArray::fromBase64(state.toLatin1()));
    }
    Logger::instance().debug("APP", "Layout restored");
}
