#include "network/DHCPWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QRegularExpression>
#include <QHostAddress>
#include <QScrollBar>
#include <QSplitter>

// ============================================================================
// DHCPWidget 实现
// ============================================================================

DHCPWidget::DHCPWidget(QWidget* parent)
    : QWidget(parent)
    , m_discoverProcess(nullptr)
    , m_serverSocket(nullptr)
    , m_serverRunning(false)
    , m_leaseProcess(nullptr)
{
    setupUI();
    setupConnections();
    populateInterfaces();
}

DHCPWidget::~DHCPWidget()
{
    // 停止 DHCP Server 监听
    if (m_serverRunning && m_serverSocket) {
        m_serverSocket->close();
    }

    // 终止 Discover 进程
    if (m_discoverProcess) {
        m_discoverProcess->kill();
        m_discoverProcess->waitForFinished(3000);
        delete m_discoverProcess;
        m_discoverProcess = nullptr;
    }

    // 终止 Lease 进程
    if (m_leaseProcess) {
        m_leaseProcess->kill();
        m_leaseProcess->waitForFinished(3000);
        delete m_leaseProcess;
        m_leaseProcess = nullptr;
    }
}

// ─── 填充网络接口列表 ─────────────────────────────────────────────────

void DHCPWidget::populateInterfaces()
{
    QStringList interfaces;
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface& iface : ifaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            // 优先使用有 IPv4 地址的接口
            bool hasIPv4 = false;
            const QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (const QNetworkAddressEntry& entry : entries) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    hasIPv4 = true;
                    break;
                }
            }
            if (hasIPv4) {
                interfaces.append(iface.name());
            }
        }
    }

    // 如果没有有 IPv4 的接口，添加所有非回环接口
    if (interfaces.isEmpty()) {
        for (const QNetworkInterface& iface : ifaces) {
            if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
                !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                interfaces.append(iface.name());
            }
        }
    }

    if (interfaces.isEmpty()) {
        interfaces.append("en0");  // macOS 默认
    }

    m_discoverInterfaceCombo->addItems(interfaces);
    m_leaseInterfaceCombo->addItems(interfaces);
}

// ============================================================================
// UI 构建
// ============================================================================

void DHCPWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── QTabWidget ──
    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "  background-color: #0D1117; border: 1px solid #30363D;"
        "}"
        "QTabBar::tab {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 8px 20px;"
        "  font-size: 13px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #0D1117; color: #58A6FF;"
        "  border-bottom: 2px solid #409EFF;"
        "}"
        "QTabBar::tab:hover {"
        "  color: #E6EDF3;"
        "}"
    );

    // ========================
    // Tab 1: DHCP Discover
    // ========================
    auto* discoverTab = new QWidget();
    auto* discoverLayout = new QVBoxLayout(discoverTab);
    discoverLayout->setContentsMargins(8, 8, 8, 8);
    discoverLayout->setSpacing(8);

    // 配置区
    auto* discoverConfigGroup = new QGroupBox("DHCP Discover 配置");
    auto* discoverConfigLayout = new QHBoxLayout(discoverConfigGroup);
    discoverConfigLayout->setSpacing(8);

    auto* ifaceLabel = new QLabel("网络接口:");
    
    m_discoverInterfaceCombo = new QComboBox();
    m_discoverInterfaceCombo->setMinimumWidth(180);
    m_discoverInterfaceCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    discoverConfigLayout->addWidget(ifaceLabel);
    discoverConfigLayout->addWidget(m_discoverInterfaceCombo);

    m_discoverBtn = new QPushButton("发送 Discover");
    m_discoverBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_discoverBtn->setFixedHeight(32);
    discoverConfigLayout->addWidget(m_discoverBtn);

    m_discoverStatusLabel = new QLabel("就绪");
    
    discoverConfigLayout->addWidget(m_discoverStatusLabel);

    m_discoverExportBtn = new QPushButton("导出 CSV");
    m_discoverExportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_discoverExportBtn->setEnabled(false);
    discoverConfigLayout->addWidget(m_discoverExportBtn);

    discoverConfigLayout->addStretch();
    discoverLayout->addWidget(discoverConfigGroup);

    // 结果表
    m_discoverTable = new QTableWidget(0, 6);
    m_discoverTable->setHorizontalHeaderLabels({"Server IP", "Offered IP", "Subnet Mask", "Gateway", "DNS", "Lease Time"});
    m_discoverTable->horizontalHeader()->setStretchLastSection(true);
    m_discoverTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_discoverTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_discoverTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_discoverTable->setAlternatingRowColors(true);
    m_discoverTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_discoverTable->verticalHeader()->setVisible(false);
    m_discoverTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 3px 6px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #161B22; }"
    );
    discoverLayout->addWidget(m_discoverTable, 1);

    // 右键菜单
    connect(m_discoverTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem* item = m_discoverTable->itemAt(pos);
        if (!item) return;

        QMenu menu(this);
        QAction* copyAction = menu.addAction("复制单元格");
        QAction* copyRowAction = menu.addAction("复制整行");

        QAction* chosen = menu.exec(m_discoverTable->viewport()->mapToGlobal(pos));
        if (chosen == copyAction) {
            QApplication::clipboard()->setText(item->text());
            Logger::instance().info("DHCP", "已复制单元格: " + item->text());
        } else if (chosen == copyRowAction) {
            int row = item->row();
            QStringList cols;
            for (int col = 0; col < m_discoverTable->columnCount(); ++col) {
                auto* it = m_discoverTable->item(row, col);
                cols << (it ? it->text() : "-");
            }
            QApplication::clipboard()->setText(cols.join('\t'));
            Logger::instance().info("DHCP", "已复制整行: " + cols.join(" | "));
        }
    });

    tabWidget->addTab(discoverTab, "DHCP Discover");

    // ========================
    // Tab 2: DHCP Server Monitor
    // ========================
    auto* serverTab = new QWidget();
    auto* serverLayout = new QVBoxLayout(serverTab);
    serverLayout->setContentsMargins(8, 8, 8, 8);
    serverLayout->setSpacing(8);

    auto* serverConfigGroup = new QGroupBox("DHCP 服务器监视");
    auto* serverConfigLayout = new QHBoxLayout(serverConfigGroup);
    serverConfigLayout->setSpacing(8);

    m_serverStartStopBtn = new QPushButton("启动监听");
    m_serverStartStopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_serverStartStopBtn->setFixedHeight(32);
    serverConfigLayout->addWidget(m_serverStartStopBtn);

    m_serverStatusLabel = new QLabel("● 已停止");
    m_serverStatusLabel->setStyleSheet("font-size: 12px; color: #888; font-weight: bold;");
    serverConfigLayout->addWidget(m_serverStatusLabel);

    m_serverCountLabel = new QLabel("记录数: 0");
    
    serverConfigLayout->addWidget(m_serverCountLabel);

    m_serverClearBtn = new QPushButton("清空");
    m_serverClearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F85149; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #FF7B72; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    serverConfigLayout->addWidget(m_serverClearBtn);

    m_serverExportBtn = new QPushButton("导出 CSV");
    m_serverExportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_serverExportBtn->setEnabled(false);
    serverConfigLayout->addWidget(m_serverExportBtn);

    serverConfigLayout->addStretch();
    serverLayout->addWidget(serverConfigGroup);

    // 服务器记录表
    m_serverTable = new QTableWidget(0, 5);
    m_serverTable->setHorizontalHeaderLabels({"Time", "Client MAC", "Request Type", "Server IP", "Offered IP"});
    m_serverTable->horizontalHeader()->setStretchLastSection(true);
    m_serverTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_serverTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_serverTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_serverTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_serverTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serverTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_serverTable->setAlternatingRowColors(true);
    m_serverTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_serverTable->verticalHeader()->setVisible(false);
    m_serverTable->setColumnWidth(0, 160);
    m_serverTable->setColumnWidth(1, 150);
    m_serverTable->setColumnWidth(2, 120);
    m_serverTable->setColumnWidth(3, 140);
    m_serverTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 3px 6px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #161B22; }"
    );
    serverLayout->addWidget(m_serverTable, 1);

    tabWidget->addTab(serverTab, "DHCP Server");

    // ========================
    // Tab 3: Lease Viewer
    // ========================
    auto* leaseTab = new QWidget();
    auto* leaseLayout = new QVBoxLayout(leaseTab);
    leaseLayout->setContentsMargins(8, 8, 8, 8);
    leaseLayout->setSpacing(8);

    auto* leaseConfigGroup = new QGroupBox("DHCP 租约查看");
    auto* leaseConfigLayout = new QHBoxLayout(leaseConfigGroup);
    leaseConfigLayout->setSpacing(8);

    auto* leaseIfaceLabel = new QLabel("网络接口:");
    
    m_leaseInterfaceCombo = new QComboBox();
    m_leaseInterfaceCombo->setMinimumWidth(180);
    m_leaseInterfaceCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    leaseConfigLayout->addWidget(leaseIfaceLabel);
    leaseConfigLayout->addWidget(m_leaseInterfaceCombo);

    m_leaseRefreshBtn = new QPushButton("刷新");
    m_leaseRefreshBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_leaseRefreshBtn->setFixedHeight(32);
    leaseConfigLayout->addWidget(m_leaseRefreshBtn);

    m_leaseStatusLabel = new QLabel("就绪");
    
    leaseConfigLayout->addWidget(m_leaseStatusLabel);

    m_leaseExportBtn = new QPushButton("导出 CSV");
    m_leaseExportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_leaseExportBtn->setEnabled(false);
    leaseConfigLayout->addWidget(m_leaseExportBtn);

    leaseConfigLayout->addStretch();
    leaseLayout->addWidget(leaseConfigGroup);

    // 租约表
    m_leaseTable = new QTableWidget(0, 7);
    m_leaseTable->setHorizontalHeaderLabels({"Interface", "IP", "Subnet", "Router", "DNS", "Lease Start", "Lease End"});
    m_leaseTable->horizontalHeader()->setStretchLastSection(true);
    m_leaseTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_leaseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_leaseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_leaseTable->setAlternatingRowColors(true);
    m_leaseTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_leaseTable->verticalHeader()->setVisible(false);
    m_leaseTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 3px 6px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #161B22; }"
    );
    leaseLayout->addWidget(m_leaseTable, 1);

    tabWidget->addTab(leaseTab, "Lease Viewer");

    mainLayout->addWidget(tabWidget, 1);
}

// ─── Connections ───────────────────────────────────────────────────────

void DHCPWidget::setupConnections()
{
    // Tab 1: DHCP Discover
    connect(m_discoverBtn, &QPushButton::clicked, this, &DHCPWidget::onDiscover);
    connect(m_discoverExportBtn, &QPushButton::clicked, this, &DHCPWidget::onDiscoverExportCSV);

    // Tab 2: DHCP Server Monitor
    connect(m_serverStartStopBtn, &QPushButton::clicked, this, &DHCPWidget::onServerStartStop);
    connect(m_serverClearBtn, &QPushButton::clicked, this, &DHCPWidget::onServerClear);
    connect(m_serverExportBtn, &QPushButton::clicked, this, &DHCPWidget::onServerExportCSV);

    // Tab 3: Lease Viewer
    connect(m_leaseRefreshBtn, &QPushButton::clicked, this, &DHCPWidget::onRefreshLease);
    connect(m_leaseExportBtn, &QPushButton::clicked, this, &DHCPWidget::onLeaseExportCSV);
}

// ============================================================================
// Tab 1: DHCP Discover
// ============================================================================

void DHCPWidget::onDiscover()
{
    QString iface = m_discoverInterfaceCombo->currentText();
    if (iface.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请选择网络接口。");
        return;
    }

    clearDiscoverTable();
    m_discoverStatusLabel->setText("正在发送 DHCP Discover...");
    m_discoverStatusLabel->setStyleSheet("font-size: 12px; color: #D29922; font-weight: bold;");
    m_discoverBtn->setEnabled(false);
    m_discoverExportBtn->setEnabled(false);

    // 使用 QProcess 运行系统命令
    if (m_discoverProcess) {
        m_discoverProcess->kill();
        m_discoverProcess->waitForFinished(2000);
        delete m_discoverProcess;
        m_discoverProcess = nullptr;
    }

    m_discoverProcess = new QProcess(this);
    connect(m_discoverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DHCPWidget::onDiscoverProcessFinished);

#ifdef Q_OS_MACOS
    // macOS: 使用 ipconfig getpacket 获取 DHCP 信息
    // 这是 macOS 上最可靠的方式查看 DHCP 获取的地址
    QStringList args;
    args << "getpacket" << iface;
    m_discoverProcess->start("/usr/sbin/ipconfig", args);
    Logger::instance().info("DHCP", QString("执行 ipconfig getpacket %1").arg(iface));
#else
    // Linux: 使用 dhclient 或 nmap 进行 DHCP Discover
    QStringList args;
    args << "-1" << "-v" << iface;
    m_discoverProcess->start("dhclient", args);
    Logger::instance().info("DHCP", QString("执行 dhclient -1 -v %1").arg(iface));
#endif
}

void DHCPWidget::onDiscoverProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (!m_discoverProcess) return;

    QString output = QString::fromUtf8(m_discoverProcess->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_discoverProcess->readAllStandardError());
    QString iface = m_discoverInterfaceCombo->currentText();

    m_discoverProcess->deleteLater();
    m_discoverProcess = nullptr;

    if (exitCode != 0 && !errOutput.isEmpty()) {
        Logger::instance().error("DHCP", "Discover 错误: " + errOutput.trimmed());
        m_discoverStatusLabel->setText("发现失败: " + errOutput.trimmed());
        m_discoverStatusLabel->setStyleSheet("font-size: 12px; color: #F85149; font-weight: bold;");
    } else {
        parseDiscoverOutput(output, iface);
        m_discoverStatusLabel->setText(
            QString("发现完成 — %1 条响应").arg(m_discoverTable->rowCount()));
        m_discoverStatusLabel->setStyleSheet("font-size: 12px; color: #3FB950; font-weight: bold;");
    }

    m_discoverBtn->setEnabled(true);
    m_discoverExportBtn->setEnabled(m_discoverTable->rowCount() > 0);
}

void DHCPWidget::parseDiscoverOutput(const QString& output, const QString& iface)
{
    // 解析 macOS ipconfig getpacket 输出
    // 格式示例:
    // dhcp_message_type = 0x05
    // server_identifier = 192.168.1.1
    // yiaddr = 192.168.1.100
    // subnet_mask = 255.255.255.0
    // router = 192.168.1.1
    // domain_name_server = 8.8.8.8, 8.8.4.4
    // lease_time = 86400

    DHCPOffer offer;
    offer.interfaceName = iface;
    offer.time = QDateTime::currentDateTime();

    bool hasOffer = false;

    static const QRegularExpression serverRe(R"(server_identifier\s*(?:=\s*)?(\S+))");
    static const QRegularExpression yiaddrRe(R"(yiaddr\s*(?:=\s*)?(\S+))");
    static const QRegularExpression maskRe(R"(subnet_mask\s*(?:=\s*)?(\S+))");
    static const QRegularExpression routerRe(R"(router\s*(?:=\s*)?(\S+))");
    static const QRegularExpression dnsRe(R"(domain_name_server\s*(?:=\s*)?(.+))");
    static const QRegularExpression leaseRe(R"(lease_time\s*(?:=\s*)?(\S+))");

    QRegularExpressionMatch m;

    m = serverRe.match(output);
    if (m.hasMatch()) {
        offer.serverIP = m.captured(1);
        hasOffer = true;
    }

    m = yiaddrRe.match(output);
    if (m.hasMatch()) {
        offer.offeredIP = m.captured(1);
        hasOffer = true;
    }

    m = maskRe.match(output);
    if (m.hasMatch()) {
        offer.subnetMask = m.captured(1);
    }

    m = routerRe.match(output);
    if (m.hasMatch()) {
        offer.gateway = m.captured(1);
    }

    m = dnsRe.match(output);
    if (m.hasMatch()) {
        QString dnsRaw = m.captured(1).trimmed();
        // 清理格式: 去除 { } 和多余空格
        dnsRaw.remove('{');
        dnsRaw.remove('}');
        dnsRaw = dnsRaw.simplified();
        // 将逗号分隔改为空格分隔
        dnsRaw.replace(", ", "\n");
        dnsRaw.replace(",", "\n");
        offer.dns = dnsRaw;
    }

    m = leaseRe.match(output);
    if (m.hasMatch()) {
        bool ok = false;
        int leaseSec = m.captured(1).toInt(&ok);
        if (ok && leaseSec > 0) {
            if (leaseSec == 0xffffffff) {
                offer.leaseTime = "无限";
            } else {
                int hours = leaseSec / 3600;
                int mins = (leaseSec % 3600) / 60;
                if (hours > 0) {
                    offer.leaseTime = QString("%1 小时 %2 分钟").arg(hours).arg(mins);
                } else {
                    offer.leaseTime = QString("%1 秒").arg(leaseSec);
                }
            }
        } else {
            offer.leaseTime = m.captured(1);
        }
    }

    if (hasOffer) {
        addDiscoverRow(offer);
    } else {
        // 尝试解析 Linux dhclient 输出
        // 格式: DHCPACK from 192.168.1.1 ... bound to 192.168.1.100
        static const QRegularExpression dhcpackRe(R"(DHCP(ACK|OFFER)\s+from\s+(\S+))");
        static const QRegularExpression boundRe(R"(bound\s+to\s+(\S+))");
        static const QRegularExpression subnetRe(R"(subnet\s+mask\s+(\S+))");
        static const QRegularExpression gwRe(R"(routers\s+(\S+))");
        static const QRegularExpression dnsLinuxRe(R"(domain.name.servers?\s+(\S+))");
        static const QRegularExpression leaseLinuxRe(R"(lease.time\s+(\S+))");

        m = dhcpackRe.match(output);
        if (m.hasMatch()) {
            offer.serverIP = m.captured(2);
            hasOffer = true;
        }

        m = boundRe.match(output);
        if (m.hasMatch()) {
            offer.offeredIP = m.captured(1);
            hasOffer = true;
        }

        m = subnetRe.match(output);
        if (m.hasMatch()) {
            offer.subnetMask = m.captured(1);
        }

        m = gwRe.match(output);
        if (m.hasMatch()) {
            offer.gateway = m.captured(1);
        }

        m = dnsLinuxRe.match(output);
        if (m.hasMatch()) {
            offer.dns = m.captured(1);
        }

        m = leaseLinuxRe.match(output);
        if (m.hasMatch()) {
            offer.leaseTime = m.captured(1);
        }

        if (hasOffer) {
            addDiscoverRow(offer);
        }
    }

    // 如果没有任何解析结果，输出原始内容作为提示
    if (m_discoverTable->rowCount() == 0) {
        if (!output.trimmed().isEmpty()) {
            // 输出原始数据供手动查看
            offer.serverIP = "-";
            offer.offeredIP = "见下方原始输出";
            offer.subnetMask = "-";
            offer.gateway = "-";
            offer.dns = "-";
            offer.leaseTime = "-";
            addDiscoverRow(offer);

            Logger::instance().info("DHCP", "原始输出: " + output.trimmed());
        } else {
            m_discoverStatusLabel->setText("未收到 DHCP 响应 — 请确认接口已连接");
            m_discoverStatusLabel->setStyleSheet("font-size: 12px; color: #D29922; font-weight: bold;");
        }
    }
}

void DHCPWidget::addDiscoverRow(const DHCPOffer& offer)
{
    int row = m_discoverTable->rowCount();
    m_discoverTable->insertRow(row);

    m_discoverTable->setItem(row, 0, new QTableWidgetItem(offer.serverIP));
    m_discoverTable->setItem(row, 1, new QTableWidgetItem(offer.offeredIP));
    m_discoverTable->setItem(row, 2, new QTableWidgetItem(offer.subnetMask));
    m_discoverTable->setItem(row, 3, new QTableWidgetItem(offer.gateway));
    m_discoverTable->setItem(row, 4, new QTableWidgetItem(offer.dns));
    m_discoverTable->setItem(row, 5, new QTableWidgetItem(offer.leaseTime));

    m_discoverTable->scrollToBottom();
}

void DHCPWidget::clearDiscoverTable()
{
    m_discoverTable->setRowCount(0);
    m_discoverExportBtn->setEnabled(false);
}

void DHCPWidget::onDiscoverExportCSV()
{
    exportTableToCSV(m_discoverTable, "DHCP Discover 结果",
                     {"Server IP", "Offered IP", "Subnet Mask", "Gateway", "DNS", "Lease Time"});
}

// ============================================================================
// Tab 2: DHCP Server Monitor
// ============================================================================

void DHCPWidget::onServerStartStop()
{
    if (m_serverRunning) {
        // 停止监听
        if (m_serverSocket) {
            m_serverSocket->close();
            delete m_serverSocket;
            m_serverSocket = nullptr;
        }
        m_serverRunning = false;
        updateServerStatus(false);
        Logger::instance().info("DHCP", "DHCP 服务器监视已停止");
    } else {
        // 启动监听
        if (!m_serverSocket) {
            m_serverSocket = new QUdpSocket(this);
            connect(m_serverSocket, &QUdpSocket::readyRead, this, &DHCPWidget::onServerUdpReadyRead);
        }

        // 绑定 DHCP 服务器端口 67
        // 注意: 绑定端口 67 通常需要 root 权限
        if (!m_serverSocket->bind(QHostAddress::Any, 67, QUdpSocket::ShareAddress)) {
            // 端口 67 需要 root 权限，尝试绑定 1067 作为替代
            Logger::instance().warning("DHCP",
                "端口 67 绑定失败 (需要 root 权限): " + m_serverSocket->errorString());
            Logger::instance().info("DHCP", "尝试绑定替代端口 1067...");

            if (!m_serverSocket->bind(QHostAddress::Any, 1067, QUdpSocket::ShareAddress)) {
                QMessageBox::critical(this, "绑定失败",
                    QString("无法绑定端口 67 或 1067:\n%1\n\n"
                            "提示: 监听 DHCP 端口 67 需要 root 权限。\n"
                            "请以 sudo 运行此程序，或确保有足够的权限。")
                        .arg(m_serverSocket->errorString()));
                delete m_serverSocket;
                m_serverSocket = nullptr;
                return;
            }

            m_serverStatusLabel->setText("● 运行中 (端口 1067)");
            m_serverStatusLabel->setStyleSheet("color: #D29922; font-weight: bold;");
        } else {
            m_serverStatusLabel->setText("● 运行中 (端口 67)");
            m_serverStatusLabel->setStyleSheet("color: #0a0; font-weight: bold;");
        }

        m_serverRunning = true;
        updateServerStatus(true);
        Logger::instance().info("DHCP", "DHCP 服务器监视已启动");
    }
}

void DHCPWidget::updateServerStatus(bool running)
{
    if (running) {
        m_serverStartStopBtn->setText("停止监听");
        m_serverStartStopBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #F85149; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #FF7B72; }"
        );
    } else {
        m_serverStartStopBtn->setText("启动监听");
        m_serverStartStopBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #58A6FF; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #79C0FF; }"
        );
        m_serverStatusLabel->setText("● 已停止");
        m_serverStatusLabel->setStyleSheet("color: #888; font-weight: bold;");
    }
}

void DHCPWidget::onServerUdpReadyRead()
{
    while (m_serverSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_serverSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_serverSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        parseDHCPPacket(datagram, sender.toString());
    }
}

void DHCPWidget::parseDHCPPacket(const QByteArray& data, const QString& sourceIP)
{
    // 基本的 DHCP 数据包解析
    // DHCP 包结构 (简化):
    // Byte 0:     op (1=BOOTREQUEST, 2=BOOTREPLY)
    // Byte 1:     htype (硬件类型, 1=Ethernet)
    // Byte 2:     hlen (硬件地址长度, 6 for MAC)
    // Byte 3:     hops
    // Byte 4-7:   xid (事务 ID)
    // Byte 8-9:   secs
    // Byte 10-11: flags
    // Byte 12-15: ciaddr (客户端 IP)
    // Byte 16-19: yiaddr (你的 IP / 分配的 IP)
    // Byte 20-23: siaddr (下一个服务器 IP)
    // Byte 24-27: giaddr (中继代理 IP)
    // Byte 28-43: chaddr (客户端硬件地址 / MAC)
    // ... 选项从 Byte 240 (0xF0) 开始

    if (data.size() < 240) return; // 最基本的 DHCP 头

    DHCPServerRecord record;
    record.time = QDateTime::currentDateTime();

    // 解析操作码
    quint8 op = static_cast<quint8>(data[0]);

    // 解析客户端 MAC (chaddr 在偏移 28, 长度 16 字节, 以太网只用前 6)
    QByteArray macBytes = data.mid(28, 6);
    QStringList macParts;
    for (int i = 0; i < macBytes.size(); ++i) {
        macParts.append(QString("%1").arg(static_cast<quint8>(macBytes[i]), 2, 16, QChar('0')).toUpper());
    }
    record.clientMAC = macParts.join(':');

    // 解析 yiaddr (分配的 IP)
    if (data.size() >= 20) {
        QByteArray yiaddr = data.mid(16, 4);
        QHostAddress yiAddr;
        yiAddr.setAddress(ntohl(*reinterpret_cast<const quint32*>(yiaddr.constData())));
        if (!yiAddr.isNull() && yiAddr != QHostAddress(QHostAddress::Any)) {
            record.offeredIP = yiAddr.toString();
        }
    }

    // 解析 siaddr (服务器 IP)
    if (data.size() >= 24) {
        QByteArray siaddr = data.mid(20, 4);
        QHostAddress siAddr;
        siAddr.setAddress(ntohl(*reinterpret_cast<const quint32*>(siaddr.constData())));
        if (!siAddr.isNull() && siAddr != QHostAddress(QHostAddress::Any)) {
            record.serverIP = siAddr.toString();
        }
    }

    // 解析 DHCP 消息类型 (选项 53) 从 Offset 240 开始
    int offset = 240;
    if (data.size() >= 244 && static_cast<quint8>(data[offset]) == 0x63 &&
        static_cast<quint8>(data[offset + 1]) == 0x82 &&
        static_cast<quint8>(data[offset + 2]) == 0x53 &&
        static_cast<quint8>(data[offset + 3]) == 0x63) {
        // 魔法 cookie 验证通过
        offset += 4;

        while (offset < data.size() - 2) {
            quint8 optionCode = static_cast<quint8>(data[offset]);
            if (optionCode == 255) break; // End option
            if (optionCode == 0) {
                offset++;
                continue; // Pad
            }

            if (offset + 1 >= data.size()) break;
            quint8 optionLen = static_cast<quint8>(data[offset + 1]);

            if (optionCode == 53 && optionLen >= 1 && offset + 2 < data.size()) {
                // DHCP Message Type
                quint8 msgType = static_cast<quint8>(data[offset + 2]);
                switch (msgType) {
                    case 1: record.requestType = "DISCOVER"; break;
                    case 2: record.requestType = "OFFER"; break;
                    case 3: record.requestType = "REQUEST"; break;
                    case 4: record.requestType = "DECLINE"; break;
                    case 5: record.requestType = "ACK"; break;
                    case 6: record.requestType = "NAK"; break;
                    case 7: record.requestType = "RELEASE"; break;
                    case 8: record.requestType = "INFORM"; break;
                    default: record.requestType = QString("Unknown(%1)").arg(msgType); break;
                }
            }

            offset += 2 + optionLen;
        }
    }

    // 根据操作码判断类型 (如果 Options 中没有解析到)
    if (record.requestType.isEmpty()) {
        if (op == 1) {
            record.requestType = "BOOTREQUEST";
        } else if (op == 2) {
            record.requestType = "BOOTREPLY";
        } else {
            record.requestType = "Unknown";
        }
    }

    // 如果 siaddr 没有值，使用源 IP 作为服务器
    if (record.serverIP.isEmpty() && op == 2) {
        record.serverIP = sourceIP;
    }

    addServerRow(record);
    m_serverCountLabel->setText(QString("记录数: %1").arg(m_serverTable->rowCount()));
}

void DHCPWidget::addServerRow(const DHCPServerRecord& record)
{
    int row = m_serverTable->rowCount();
    m_serverTable->insertRow(row);

    m_serverTable->setItem(row, 0, new QTableWidgetItem(record.time.toString("yyyy-MM-dd hh:mm:ss.zzz")));
    m_serverTable->setItem(row, 1, new QTableWidgetItem(record.clientMAC));

    // 请求类型着色
    auto* typeItem = new QTableWidgetItem(record.requestType);
    QColor typeColor;
    if (record.requestType == "DISCOVER") typeColor = QColor(0x58, 0xA6, 0xFF);
    else if (record.requestType == "OFFER") typeColor = QColor(0x3F, 0xB9, 0x50);
    else if (record.requestType == "REQUEST") typeColor = QColor(0xD2, 0x99, 0x22);
    else if (record.requestType == "ACK") typeColor = QColor(0x2C, 0xCC, 0x70);
    else if (record.requestType == "NAK") typeColor = QColor(0xF8, 0x51, 0x49);
    else if (record.requestType == "RELEASE") typeColor = QColor(0x90, 0x90, 0x90);
    else typeColor = QColor(0xDC, 0xDC, 0xDC);
    typeItem->setForeground(typeColor);
    m_serverTable->setItem(row, 2, typeItem);

    m_serverTable->setItem(row, 3, new QTableWidgetItem(record.serverIP.isEmpty() ? "-" : record.serverIP));
    m_serverTable->setItem(row, 4, new QTableWidgetItem(record.offeredIP.isEmpty() ? "-" : record.offeredIP));

    m_serverTable->scrollToBottom();
}

void DHCPWidget::onServerClear()
{
    m_serverTable->setRowCount(0);
    m_serverCountLabel->setText("记录数: 0");
    m_serverExportBtn->setEnabled(false);
    Logger::instance().info("DHCP", "DHCP 服务器监视记录已清空");
}

void DHCPWidget::onServerExportCSV()
{
    exportTableToCSV(m_serverTable, "DHCP Server 监视记录",
                     {"Time", "Client MAC", "Request Type", "Server IP", "Offered IP"});
}

// ============================================================================
// Tab 3: Lease Viewer
// ============================================================================

void DHCPWidget::onRefreshLease()
{
    QString iface = m_leaseInterfaceCombo->currentText();
    if (iface.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请选择网络接口。");
        return;
    }

    clearLeaseTable();
    m_leaseStatusLabel->setText("正在读取租约信息...");
    m_leaseStatusLabel->setStyleSheet("font-size: 12px; color: #D29922; font-weight: bold;");
    m_leaseRefreshBtn->setEnabled(false);
    m_leaseExportBtn->setEnabled(false);

    if (m_leaseProcess) {
        m_leaseProcess->kill();
        m_leaseProcess->waitForFinished(2000);
        delete m_leaseProcess;
        m_leaseProcess = nullptr;
    }

    m_leaseProcess = new QProcess(this);

#ifdef Q_OS_MACOS
    // macOS: 使用 ipconfig getpacket 获取租约详情
    QStringList args;
    args << "getpacket" << iface;
    m_leaseProcess->start("/usr/sbin/ipconfig", args);
    Logger::instance().info("DHCP", QString("读取租约: ipconfig getpacket %1").arg(iface));
#else
    // Linux: 读取 /var/lib/dhcp/dhclient.*.leases 或使用 dhcpcd -U
    QStringList args;
    args << "-c" << QString("cat /var/lib/dhcp/dhclient.%1.leases 2>/dev/null || "
                           "cat /var/lib/dhclient/dhclient-%1.leases 2>/dev/null || "
                           "cat /var/lib/NetworkManager/dhclient-%1-*.lease 2>/dev/null").arg(iface);
    m_leaseProcess->start("/bin/sh", args);
    Logger::instance().info("DHCP", QString("读取租约: 接口 %1").arg(iface));
#endif

    connect(m_leaseProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)

        if (!m_leaseProcess) return;

        QString output = QString::fromUtf8(m_leaseProcess->readAllStandardOutput());
        QString errOutput = QString::fromUtf8(m_leaseProcess->readAllStandardError());

        m_leaseProcess->deleteLater();
        m_leaseProcess = nullptr;

        if (exitCode != 0 && !errOutput.isEmpty() && output.trimmed().isEmpty()) {
            Logger::instance().error("DHCP", "读取租约错误: " + errOutput.trimmed());
            m_leaseStatusLabel->setText("读取失败: " + errOutput.trimmed());
            m_leaseStatusLabel->setStyleSheet("font-size: 12px; color: #F85149; font-weight: bold;");
        } else {
            parseLeaseOutput(output);
            m_leaseStatusLabel->setText(
                QString("读取完成 — %1 条租约").arg(m_leaseTable->rowCount()));
            m_leaseStatusLabel->setStyleSheet("font-size: 12px; color: #3FB950; font-weight: bold;");
        }

        m_leaseRefreshBtn->setEnabled(true);
        m_leaseExportBtn->setEnabled(m_leaseTable->rowCount() > 0);
    });
}

void DHCPWidget::parseLeaseOutput(const QString& output)
{
    // macOS ipconfig getpacket 输出解析
    DHCPLease lease;
    lease.interfaceName = m_leaseInterfaceCombo->currentText();

    static const QRegularExpression yiaddrRe(R"(yiaddr\s*(?:=\s*)?(\S+))");
    static const QRegularExpression maskRe(R"(subnet_mask\s*(?:=\s*)?(\S+))");
    static const QRegularExpression routerRe(R"(router\s*(?:=\s*)?(\S+))");
    static const QRegularExpression dnsRe(R"(domain_name_server\s*(?:=\s*)?(.+))");
    static const QRegularExpression serverRe(R"(server_identifier\s*(?:=\s*)?(\S+))");
    static const QRegularExpression leaseTimeRe(R"(lease_time\s*(?:=\s*)?(\S+))");

    QRegularExpressionMatch m;

    m = yiaddrRe.match(output);
    if (m.hasMatch()) {
        lease.ipAddress = m.captured(1);
    }

    m = maskRe.match(output);
    if (m.hasMatch()) {
        lease.subnetMask = m.captured(1);
    }

    m = routerRe.match(output);
    if (m.hasMatch()) {
        lease.router = m.captured(1);
    }

    m = dnsRe.match(output);
    if (m.hasMatch()) {
        QString dnsRaw = m.captured(1).trimmed();
        dnsRaw.remove('{');
        dnsRaw.remove('}');
        dnsRaw = dnsRaw.simplified();
        dnsRaw.replace(", ", "\n");
        dnsRaw.replace(",", "\n");
        lease.dns = dnsRaw;
    }

    m = serverRe.match(output);
    if (m.hasMatch()) {
        lease.serverIP = m.captured(1);
    }

    m = leaseTimeRe.match(output);
    if (m.hasMatch()) {
        bool ok = false;
        int leaseSec = m.captured(1).toInt(&ok);
        if (ok && leaseSec > 0) {
            QDateTime now = QDateTime::currentDateTime();
            lease.leaseStart = now.toString("yyyy-MM-dd hh:mm:ss");
            lease.leaseEnd = now.addSecs(leaseSec).toString("yyyy-MM-dd hh:mm:ss");
        }
    }

    if (!lease.ipAddress.isEmpty()) {
        addLeaseRow(lease);
    } else {
        // 尝试 Linux DHCP 租约文件格式
        // lease { ... }
        static const QRegularExpression leaseBlockRe(
            R"(lease\s+(\S+)\s*\{([^}]+)\})",
            QRegularExpression::DotMatchesEverythingOption
        );

        auto it = leaseBlockRe.globalMatch(output);
        bool found = false;

        while (it.hasNext()) {
            auto match = it.next();
            DHCPLease linuxLease;
            linuxLease.interfaceName = m_leaseInterfaceCombo->currentText();
            linuxLease.ipAddress = match.captured(1);

            QString block = match.captured(2);

            static const QRegularExpression startRe(R"(starts\s+\d+\s+([^;]+);)");
            static const QRegularExpression endRe(R"(ends\s+\d+\s+([^;]+);)");
            static const QRegularExpression subnetRe(R"(option\s+subnet-mask\s+(\S+);)");
            static const QRegularExpression gwRe(R"(option\s+routers\s+(\S+);)");
            static const QRegularExpression dnsLinuxRe(R"(option\s+domain-name-servers\s+(.+?);)");
            static const QRegularExpression serverLinuxRe(R"(dhcp-server-identifier\s+(\S+);)");

            QRegularExpressionMatch lm;

            lm = startRe.match(block);
            if (lm.hasMatch()) linuxLease.leaseStart = lm.captured(1).trimmed();

            lm = endRe.match(block);
            if (lm.hasMatch()) linuxLease.leaseEnd = lm.captured(1).trimmed();

            lm = subnetRe.match(block);
            if (lm.hasMatch()) linuxLease.subnetMask = lm.captured(1);

            lm = gwRe.match(block);
            if (lm.hasMatch()) linuxLease.router = lm.captured(1);

            lm = dnsLinuxRe.match(block);
            if (lm.hasMatch()) {
                QString dnsList = lm.captured(1).trimmed();
                dnsList.replace(", ", "\n");
                dnsList.replace(",", "\n");
                linuxLease.dns = dnsList;
            }

            lm = serverLinuxRe.match(block);
            if (lm.hasMatch()) linuxLease.serverIP = lm.captured(1);

            addLeaseRow(linuxLease);
            found = true;
        }

        if (!found && !output.trimmed().isEmpty()) {
            // 如果以上都无法解析，直接显示原始输出
            Logger::instance().info("DHCP", "租约原始输出: " + output.trimmed());
            m_leaseStatusLabel->setText("无法解析租约格式 — 请查看日志");
            m_leaseStatusLabel->setStyleSheet("font-size: 12px; color: #D29922; font-weight: bold;");
        }
    }
}

void DHCPWidget::addLeaseRow(const DHCPLease& lease)
{
    int row = m_leaseTable->rowCount();
    m_leaseTable->insertRow(row);

    m_leaseTable->setItem(row, 0, new QTableWidgetItem(lease.interfaceName));
    m_leaseTable->setItem(row, 1, new QTableWidgetItem(lease.ipAddress));
    m_leaseTable->setItem(row, 2, new QTableWidgetItem(lease.subnetMask));
    m_leaseTable->setItem(row, 3, new QTableWidgetItem(lease.router));
    m_leaseTable->setItem(row, 4, new QTableWidgetItem(lease.dns));
    m_leaseTable->setItem(row, 5, new QTableWidgetItem(lease.leaseStart));
    m_leaseTable->setItem(row, 6, new QTableWidgetItem(lease.leaseEnd));

    m_leaseTable->scrollToBottom();
}

void DHCPWidget::clearLeaseTable()
{
    m_leaseTable->setRowCount(0);
    m_leaseExportBtn->setEnabled(false);
}

void DHCPWidget::onLeaseExportCSV()
{
    exportTableToCSV(m_leaseTable, "DHCP 租约信息",
                     {"Interface", "IP", "Subnet", "Router", "DNS", "Lease Start", "Lease End"});
}

// ============================================================================
// 通用 CSV 导出
// ============================================================================

void DHCPWidget::exportTableToCSV(QTableWidget* table, const QString& title, const QStringList& headers)
{
    if (!table || table->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QString("导出 %1").arg(title),
        QString("dhcp_%1_%2.csv")
            .arg(title.toLower().replace(' ', '_'))
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    // BOM for Excel UTF-8
    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    // 表头
    out << headers.join(',') << "\n";

    for (int row = 0; row < table->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < table->columnCount(); ++col) {
            auto* item = table->item(row, col);
            QString val = item ? item->text() : "-";
            if (val.contains(',') || val.contains('\n') || val.contains('"')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            cols << val;
        }
        out << cols.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("DHCP",
        QString("%1 已导出到: %2 (%3 条记录)")
            .arg(title).arg(filePath).arg(table->rowCount()));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(table->rowCount()).arg(filePath));
}