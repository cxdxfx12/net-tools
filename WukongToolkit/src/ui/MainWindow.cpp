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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("悟空网络工程师工具箱");
    resize(1280, 800);
    setMinimumSize(1024, 600);

    setupMenuBar();
    setupToolBar();
    setupMenuTree();
    setupWorkArea();
    setupLogDock();
    setupStatusBar();
    setupConnections();

    // Connect to logger
    connect(&Logger::instance(), &Logger::logMessage,
            this, &MainWindow::appendLog);

    Logger::instance().info("APP", "MainWindow initialized");
}

MainWindow::~MainWindow()
{
    saveLayout();
}

// ─── Menu Bar ────────────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    QMenuBar* mb = menuBar();

    // File menu
    QMenu* fileMenu = mb->addMenu("文件(&F)");
    fileMenu->addAction("新建会话(&N)", QKeySequence::New, this, &MainWindow::openSSHDialog);
    fileMenu->addAction("打开配置(&O)", QKeySequence::Open, this, []() {
        Logger::instance().info("APP", "Open config requested");
    });
    fileMenu->addSeparator();
    fileMenu->addAction("保存布局(&S)", QKeySequence::Save, this, &MainWindow::saveLayout);
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&X)", QKeySequence::Quit, this, &QMainWindow::close);

    // View menu
    QMenu* viewMenu = mb->addMenu("视图(&V)");
    viewMenu->addAction("显示菜单树", this, [this]() {
        m_menuDock->setVisible(!m_menuDock->isVisible());
    });
    viewMenu->addAction("显示日志", this, [this]() {
        m_logDock->setVisible(!m_logDock->isVisible());
    });
    viewMenu->addSeparator();
    viewMenu->addAction("切换主题 (&T)", this, []() {
        ThemeManager::instance().toggleTheme();
    });
    viewMenu->addSeparator();
    viewMenu->addAction("恢复默认布局", this, [this]() {
        // Reset to default layout
        m_menuDock->setVisible(true);
        m_logDock->setVisible(true);
        Logger::instance().info("APP", "Layout reset to default");
    });

    // Tools menu
    QMenu* toolsMenu = mb->addMenu("工具(&T)");
    toolsMenu->addAction("Ping", this, []() {
        Logger::instance().info("APP", "Ping tool requested");
    });
    toolsMenu->addAction("Traceroute", this, []() {
        Logger::instance().info("APP", "Traceroute tool requested");
    });
    toolsMenu->addAction("SSH 终端", this, &MainWindow::openSSHDialog);
    toolsMenu->addSeparator();
    toolsMenu->addAction("设置(&S)", this, []() {
        Logger::instance().info("APP", "Settings requested");
    });

    // Help menu
    QMenu* helpMenu = mb->addMenu("帮助(&H)");
    helpMenu->addAction("关于(&A)", this, [this]() {
        QMessageBox::about(this, "关于悟空工具箱",
                           "悟空网络工程师工具箱 v1.0.0\n\n"
                           "一个软件解决所有网络工程问题。\n"
                           "网络工程师的 IDE。");
    });
}

// ─── ToolBar ─────────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar("主工具栏");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    QAction* sshAction = toolbar->addAction("SSH");
    sshAction->setToolTip("新建 SSH 连接");
    connect(sshAction, &QAction::triggered, this, &MainWindow::openSSHDialog);

    QAction* pingAction = toolbar->addAction("Ping");
    pingAction->setToolTip("Ping 工具");
    connect(pingAction, &QAction::triggered, this, []() {
        Logger::instance().info("APP", "Ping toolbar clicked");
    });

    toolbar->addSeparator();

    QAction* themeAction = toolbar->addAction("主题");
    themeAction->setToolTip("切换亮色/暗色主题");
    connect(themeAction, &QAction::triggered, this, []() {
        ThemeManager::instance().toggleTheme();
    });

    QAction* settingsAction = toolbar->addAction("设置");
    settingsAction->setToolTip("打开设置");
    connect(settingsAction, &QAction::triggered, this, []() {
        Logger::instance().info("APP", "Settings toolbar clicked");
    });
}

// ─── Menu Tree (Left Dock) ───────────────────────────────────────────

void MainWindow::setupMenuTree()
{
    m_menuDock = new QDockWidget("工具导航", this);
    m_menuDock->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetClosable);
    m_menuDock->setMinimumWidth(200);

    m_menuTree = new QTreeWidget();
    m_menuTree->setHeaderHidden(true);
    m_menuTree->setIndentation(16);
    m_menuTree->setIconSize(QSize(18, 18));
    m_menuTree->setAnimated(true);

    // Build menu tree
    auto addCategory = [this](const QString& name) -> QTreeWidgetItem* {
        auto* item = new QTreeWidgetItem(m_menuTree);
        item->setText(0, name);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
        item->setExpanded(true);
        return item;
    };

    auto addTool = [](QTreeWidgetItem* parent, const QString& name) -> QTreeWidgetItem* {
        auto* item = new QTreeWidgetItem(parent);
        item->setText(0, name);
        item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return item;
    };

    // 网络工具
    auto* netTools = addCategory("网络工具");
    addTool(netTools, "Ping");
    addTool(netTools, "Traceroute");
    addTool(netTools, "DNS");
    addTool(netTools, "DNS 诊断");
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
    auto* terminal = addCategory("终端");
    addTool(terminal, "SSH");
    addTool(terminal, "Telnet");
    addTool(terminal, "Serial");

    // 文件传输
    auto* fileTransfer = addCategory("文件传输");
    addTool(fileTransfer, "FTP");
    addTool(fileTransfer, "SFTP");
    addTool(fileTransfer, "TFTP");
    addTool(fileTransfer, "SCP");

    // 网络协议
    auto* protocols = addCategory("网络协议");
    addTool(protocols, "SNMP");
    addTool(protocols, "SNMP 监控");
    addTool(protocols, "MIB 浏览器");
    addTool(protocols, "DHCP");
    addTool(protocols, "HTTP");
    addTool(protocols, "Protocol Analyzer");

    // 监控
    auto* monitor = addCategory("监控");
    addTool(monitor, "设备监控");
    addTool(monitor, "告警中心");
    addTool(monitor, "日志中心");
    addTool(monitor, "运维总览");
    addTool(monitor, "性能分析");
    addTool(monitor, "流量分析");
    addTool(monitor, "QoS 分析");

    // 运维
    auto* ops = addCategory("运维");
    addTool(ops, "巡检中心");
    addTool(ops, "安全审计中心");
    addTool(ops, "报表中心");
    addTool(ops, "设备发现");
    addTool(ops, "网络拓扑");
    addTool(ops, "HA 高可用性管理");
    addTool(ops, "VPN 管理中心");
    addTool(ops, "自动化运维");
    addTool(ops, "资产管理");
    addTool(ops, "AAA 认证管理");
    addTool(ops, "流采集器");

    // 无线
    auto* wireless = addCategory("无线");
    addTool(wireless, "无线管理");

    // IPv6
    auto* ipv6 = addCategory("IPv6");
    addTool(ipv6, "IPv6 管理");

    // 规划
    auto* planning = addCategory("网络规划");
    addTool(planning, "IP 计算器");
    addTool(planning, "子网规划");
    addTool(planning, "MAC 地址工具");

    // 工业协议
    auto* industrial = addCategory("工业协议");
    addTool(industrial, "MQTT");
    addTool(industrial, "Modbus");

    // 工具
    auto* tools = addCategory("工具");
    addTool(tools, "配置中心");
    addTool(tools, "配置Diff");
    addTool(tools, "配置生成");
    addTool(tools, "MAC查询");

    // 系统
    auto* sysCategory = addCategory("系统");
    addTool(sysCategory, "系统设置");
    addTool(sysCategory, "日志");
    addTool(sysCategory, "插件管理");
    addTool(sysCategory, "Device Center");

    m_menuDock->setWidget(m_menuTree);
    addDockWidget(Qt::LeftDockWidgetArea, m_menuDock);
}

// ─── Work Area (Center) ──────────────────────────────────────────────

void MainWindow::setupWorkArea()
{
    m_workArea = new QTabWidget();
    m_workArea->setTabsClosable(true);
    m_workArea->setMovable(true);
    m_workArea->setDocumentMode(true);

    // Welcome tab
    auto* welcomeWidget = new QWidget();
    auto* welcomeLayout = new QVBoxLayout(welcomeWidget);
    welcomeLayout->setAlignment(Qt::AlignCenter);

    auto* welcomeLabel = new QLabel("悟空网络工程师工具箱");
    welcomeLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #409EFF;");
    welcomeLabel->setAlignment(Qt::AlignCenter);

    auto* welcomeSub = new QLabel("从左侧菜单中选择工具开始使用");
    welcomeSub->setStyleSheet("font-size: 14px; color: #8C8C8C;");
    welcomeSub->setAlignment(Qt::AlignCenter);

    auto* versionLabel = new QLabel("v1.0.0 MVP");
    versionLabel->setStyleSheet("font-size: 12px; color: #5C5C5C;");
    versionLabel->setAlignment(Qt::AlignCenter);

    welcomeLayout->addWidget(welcomeLabel);
    welcomeLayout->addWidget(welcomeSub);
    welcomeLayout->addSpacing(20);
    welcomeLayout->addWidget(versionLabel);

    m_workArea->addTab(welcomeWidget, "欢迎");

    // Close tab handler
    connect(m_workArea, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (index <= 0) return; // Don't close welcome tab

        QWidget* w = m_workArea->widget(index);
        if (m_sshTabs.contains(w)) {
            // SSH tabs are handled by their own close handler
            return;
        }
        m_workArea->removeTab(index);
        w->deleteLater();
    });

    setCentralWidget(m_workArea);
}

// ─── Log Dock (Bottom) ───────────────────────────────────────────────

void MainWindow::setupLogDock()
{
    m_logDock = new QDockWidget("日志输出", this);
    m_logDock->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetClosable);

    m_logView = new QPlainTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1E1F22;"
        "  color: #DCDCDC;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 12px;"
        "  border: none;"
        "}"
    );

    m_logDock->setWidget(m_logView);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
}

// ─── StatusBar ───────────────────────────────────────────────────────

void MainWindow::setupStatusBar()
{
    m_statusBarManager = new StatusBarManager(statusBar(), this);
    m_statusBarManager->setVersion("v1.0.0");
    m_statusBarManager->startMonitoring();
}

// ─── Connections ─────────────────────────────────────────────────────

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
    if (!item || item->childCount() > 0) return; // Skip category items

    QString toolName = item->text(0);
    Logger::instance().info("APP", "Menu clicked: " + toolName);
}

void MainWindow::onMenuTreeItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    if (!item || item->childCount() > 0) return; // Skip category items

    QString toolName = item->text(0);
    Logger::instance().info("APP", "Tool launched: " + toolName);

    if (toolName == "SSH") {
        openSSHDialog();
        return;
    }

    if (toolName == "Ping") {
        auto* widget = new PingWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Traceroute") {
        auto* widget = new TracerouteWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "DNS") {
        auto* widget = new DNSWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "IP Scanner") {
        auto* widget = new IPScannerWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Port Scanner") {
        auto* widget = new PortScannerWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "SNMP") {
        auto* widget = new SNMPWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "SFTP") {
        auto* widget = new SFTPWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "TFTP") {
        auto* widget = new TFTPWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Telnet") {
        auto* widget = new TelnetWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Serial") {
        auto* widget = new SerialWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Syslog") {
        auto* widget = new SyslogWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Flow Analyzer") {
        auto* widget = new FlowWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Packet Capture") {
        auto* widget = new PacketCaptureWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Protocol Analyzer") {
        auto* widget = new ProtocolAnalyzerWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "ARP") {
        auto* widget = new ARPWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "DHCP") {
        auto* widget = new DHCPWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "HTTP") {
        auto* widget = new HttpWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "WOL") {
        auto* widget = new WOLWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "Device Center") {
        auto* widget = new DeviceCenter();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "MAC 地址工具") {
        auto* widget = new MacToolkitWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "IP 计算器") {
        auto* widget = new IpCalculatorWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "子网规划") {
        auto* widget = new PlannerWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "DNS 诊断") {
        auto* widget = new DnsToolkitWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "SNMP 监控") {
        auto* widget = new SNMPMonitorWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "MIB 浏览器") {
        auto* widget = new MibWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "设备监控") {
        auto* widget = new MonitorCenterWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "告警中心") {
        auto* widget = new AlarmCenterWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "日志中心") {
        auto* widget = new SyslogCenterWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "配置中心") {
        auto* widget = new ConfigCenterWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "巡检中心") {
        auto* widget = new InspectionWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "安全审计中心") {
        auto* widget = new SecurityWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "报表中心") {
        auto* widget = new ReportWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "设备发现") {
        auto* widget = new DiscoveryWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "网络拓扑") {
        auto* widget = new TopologyWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "HA 高可用性管理") {
        auto* widget = new HAWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "VPN 管理中心") {
        auto* widget = new VPNWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "插件管理") {
        auto* widget = new PluginWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "运维总览") {
        auto* widget = new DashboardWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "性能分析") {
        auto* widget = new PerformanceWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "流量分析") {
        auto* widget = new TrafficWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "QoS 分析") {
        auto* widget = new QoSWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "自动化运维") {
        auto* widget = new AutomationWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "资产管理") {
        auto* widget = new AssetWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "AAA 认证管理") {
        auto* widget = new AAAWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "流采集器") {
        auto* widget = new FlowCollectorWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "无线管理") {
        auto* widget = new WirelessWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "IPv6 管理") {
        auto* widget = new IPv6Widget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }

    if (toolName == "系统设置") {
        auto* widget = new SettingsWidget();
        m_workArea->addTab(widget, toolName);
        m_workArea->setCurrentWidget(widget);
        return;
    }
}

// ─── Log Display ─────────────────────────────────────────────────────

void MainWindow::appendLog(const QString& time, const QString& level,
                           const QString& module, const QString& message)
{
    QString color;
    if (level == "INFO") {
        color = "#67C23A";
    } else if (level == "WARNING") {
        color = "#E6A23C";
    } else if (level == "ERROR") {
        color = "#F56C6C";
    } else {
        color = "#8C8C8C";
    }

    QString html = QString(
        "<span style='color:#8C8C8C;'>%1</span> "
        "<span style='color:%2;'>[%3]</span> "
        "<span style='color:#409EFF;'>[%4]</span> "
        "<span style='color:#DCDCDC;'>%5</span>"
    ).arg(time, color, level, module, message.toHtmlEscaped());

    m_logView->appendHtml(html);
}

// ─── SSH Integration ─────────────────────────────────────────────────

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
    // Create terminal widget
    auto* terminal = new TerminalWidget();
    auto* history = new CommandHistory(terminal);

    // Create container with header info
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar
    auto* header = new QWidget();
    header->setStyleSheet("background-color: #25262B; border-bottom: 1px solid #3C3F41;");
    header->setFixedHeight(32);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 0, 8, 0);

    auto* hostLabel = new QLabel(QString("%1:%2").arg(host).arg(port));
    hostLabel->setStyleSheet("color: #409EFF; font-weight: bold; font-size: 12px;");
    auto* protoLabel = new QLabel("SSH");
    protoLabel->setStyleSheet("color: #67C23A; font-size: 11px; padding: 0 6px;");
    auto* statusLabel = new QLabel("连接中...");
    statusLabel->setStyleSheet("color: #E6A23C; font-size: 11px;");

    headerLayout->addWidget(hostLabel);
    headerLayout->addWidget(protoLabel);
    headerLayout->addWidget(statusLabel);
    headerLayout->addStretch();

    auto* encodingLabel = new QLabel("UTF-8");
    encodingLabel->setStyleSheet("color: #8C8C8C; font-size: 11px;");
    headerLayout->addWidget(encodingLabel);

    layout->addWidget(header);
    layout->addWidget(terminal);

    QString tabTitle = QString("%1@%2").arg(user, host);
    int tabIndex = m_workArea->addTab(container, tabTitle);
    m_workArea->setCurrentIndex(tabIndex);

    // Create SSH session
    SSHConfig cfg;
    cfg.host = host;
    cfg.port = port;
    cfg.username = user;
    cfg.password = password;

    auto* session = SSHManager::instance().createSession(cfg);

    // Track tab info
    SSHTabInfo info;
    info.terminal = terminal;
    info.history = history;
    info.sessionId = session->id();
    m_sshTabs.insert(container, info);

    // Connect terminal output to session
    connect(terminal, &TerminalWidget::dataReady, session, &SSHSession::sendData);

    // Connect session data to terminal
    connect(session, &SSHSession::dataReceived, terminal, &TerminalWidget::appendOutput);

    // Connect state changes
    connect(session, &SSHSession::stateChanged, container, [statusLabel, terminal](SessionState state) {
        switch (state) {
        case SessionState::Connecting:
            statusLabel->setText("连接中...");
            statusLabel->setStyleSheet("color: #E6A23C; font-size: 11px;");
            break;
        case SessionState::Connected:
        case SessionState::Running:
            statusLabel->setText("已连接");
            statusLabel->setStyleSheet("color: #67C23A; font-size: 11px;");
            terminal->setConnected(true);
            break;
        case SessionState::Disconnecting:
            statusLabel->setText("断开中...");
            statusLabel->setStyleSheet("color: #E6A23C; font-size: 11px;");
            break;
        case SessionState::Closed:
            statusLabel->setText("已断开");
            statusLabel->setStyleSheet("color: #F56C6C; font-size: 11px;");
            terminal->setConnected(false);
            break;
        case SessionState::Error:
            statusLabel->setText("错误");
            statusLabel->setStyleSheet("color: #F56C6C; font-size: 11px;");
            terminal->setConnected(false);
            break;
        default:
            break;
        }
    });

    // Connect errors
    connect(session, &SSHSession::connectionError, container, [statusLabel](const QString& error) {
        Q_UNUSED(error)
        statusLabel->setText("连接失败");
        statusLabel->setStyleSheet("color: #F56C6C; font-size: 11px;");
    });

    // Connect reconnecting
    connect(session, &SSHSession::reconnecting, container, [statusLabel](int attempt) {
        statusLabel->setText(QString("重连中 (%1/5)").arg(attempt));
        statusLabel->setStyleSheet("color: #E6A23C; font-size: 11px;");
    });

    // Handle tab close
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

    // Start connection
    session->connectToHost();

    Logger::instance().info("SSH", QString("SSH tab opened: %1@%2:%3").arg(user, host).arg(port));
}

// ─── Layout Persistence ──────────────────────────────────────────────

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