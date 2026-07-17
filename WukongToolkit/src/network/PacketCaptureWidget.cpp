#include "network/PacketCaptureWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTimer>
#include <QDir>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QFont>
#include <QFontDatabase>

// ============================================================================
// 辅助: 已知协议端口映射
// ============================================================================
static QString protocolFromPort(const QString& srcPort, const QString& dstPort, const QString& baseProto)
{
    struct PortProto { int port; QString proto; };
    static const QVector<PortProto> knownPorts = {
        {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"},
        {53, "DNS"}, {67, "DHCP"}, {68, "DHCP"}, {69, "TFTP"},
        {80, "HTTP"}, {110, "POP3"}, {123, "NTP"}, {143, "IMAP"},
        {161, "SNMP"}, {162, "SNMP-Trap"}, {179, "BGP"},
        {389, "LDAP"}, {443, "HTTPS"}, {445, "SMB"},
        {465, "SMTPS"}, {514, "Syslog"}, {520, "RIP"},
        {587, "SMTP"}, {636, "LDAPS"}, {993, "IMAPS"},
        {995, "POP3S"}, {1080, "SOCKS"}, {1194, "OpenVPN"},
        {1433, "MSSQL"}, {1521, "Oracle"}, {1723, "PPTP"},
        {1883, "MQTT"}, {2049, "NFS"}, {2181, "ZooKeeper"},
        {2375, "Docker"}, {3128, "Squid"}, {3306, "MySQL"},
        {3389, "RDP"}, {3478, "STUN"}, {4500, "IPsec-NAT"},
        {5060, "SIP"}, {5061, "SIP-TLS"}, {5222, "XMPP"},
        {5353, "mDNS"}, {5432, "PostgreSQL"}, {5555, "ADB"},
        {5672, "RabbitMQ"}, {5900, "VNC"}, {5985, "WinRM"},
        {6379, "Redis"}, {6443, "K8s-API"}, {6667, "IRC"},
        {8080, "HTTP-Proxy"}, {8443, "HTTPS-Alt"},
        {9090, "Prometheus"}, {9092, "Kafka"}, {9200, "Elasticsearch"},
        {10050, "Zabbix"}, {10051, "Zabbix"}, {11211, "Memcached"},
        {27017, "MongoDB"},
    };

    bool srcOk = false, dstOk = false;
    int sp = srcPort.toInt(&srcOk);
    int dp = dstPort.toInt(&dstOk);

    auto findProto = [](int p) -> QString {
        for (const auto& pp : knownPorts) {
            if (pp.port == p) return pp.proto;
        }
        return {};
    };

    if (srcOk) {
        QString p = findProto(sp);
        if (!p.isEmpty()) return p;
    }
    if (dstOk) {
        QString p = findProto(dp);
        if (!p.isEmpty()) return p;
    }
    return baseProto;
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

PacketCaptureWidget::PacketCaptureWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
{
    setupUI();
    setupConnections();
    detectInterfaces();

    // 生成临时 pcap 文件路径
    m_tempPcapPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                     + "/wukong_capture_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".pcap";
}

PacketCaptureWidget::~PacketCaptureWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }

    // 清理临时文件
    if (!m_tempPcapPath.isEmpty()) {
        QFile::remove(m_tempPcapPath);
    }
}

// ============================================================================
// UI 构建
// ============================================================================

void PacketCaptureWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 捕获配置区 ──
    auto* configGroup = new QGroupBox("捕获配置");
    auto* configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(6);

    // 第一行: 网卡选择 + 刷新
    auto* ifaceRow = new QHBoxLayout();
    ifaceRow->setSpacing(8);

    auto* ifaceLabel = new QLabel("网卡:");
    ifaceLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_interfaceCombo = new QComboBox();
    m_interfaceCombo->setMinimumWidth(180);
    m_interfaceCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #409EFF;"
        "  border: 1px solid #3C3F41;"
        "}"
    );
    ifaceRow->addWidget(ifaceLabel);
    ifaceRow->addWidget(m_interfaceCombo, 1);

    m_refreshInterfaceBtn = new QPushButton("刷新");
    m_refreshInterfaceBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3C3F41; color: #DCDCDC;"
        "  border: none; padding: 6px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #4A4D50; }"
    );
    m_refreshInterfaceBtn->setFixedHeight(30);
    ifaceRow->addWidget(m_refreshInterfaceBtn);

    configLayout->addLayout(ifaceRow);

    // 第二行: 过滤器 + 包数量限制
    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(8);

    auto* filterLabel = new QLabel("过滤器:");
    filterLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_filterEdit = new QLineEdit();
    m_filterEdit->setPlaceholderText("例如: tcp port 80, host 192.168.1.1, icmp, udp port 53");
    m_filterEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    filterRow->addWidget(filterLabel);
    filterRow->addWidget(m_filterEdit, 3);

    auto* countLabel = new QLabel("包数量:");
    countLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_packetCountSpin = new QSpinBox();
    m_packetCountSpin->setRange(0, 99999);
    m_packetCountSpin->setValue(100);
    m_packetCountSpin->setSpecialValueText("无限制");
    m_packetCountSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    filterRow->addWidget(countLabel);
    filterRow->addWidget(m_packetCountSpin);

    configLayout->addLayout(filterRow);

    // 第三行: 按钮
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_startBtn = new QPushButton("开始捕获");
    m_startBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_startBtn->setFixedHeight(32);

    m_stopBtn = new QPushButton("停止捕获");
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #F78989; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_stopBtn->setFixedHeight(32);

    m_clearBtn = new QPushButton("清除");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #E6A23C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #EBB563; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_clearBtn->setFixedHeight(32);

    m_exportBtn = new QPushButton("导出 PCAP");
    m_exportBtn->setEnabled(false);
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_exportBtn->setFixedHeight(32);

    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_exportBtn);

    configLayout->addLayout(btnRow);

    mainLayout->addWidget(configGroup);

    // ── 状态栏 ──
    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(16);

    m_statusLabel = new QLabel("就绪 — 等待开始捕获");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #67C23A; font-weight: bold;");
    statusRow->addWidget(m_statusLabel);

    m_packetCountLabel = new QLabel("捕获: 0 包");
    m_packetCountLabel->setStyleSheet("font-size: 13px; color: #DCDCDC;");
    statusRow->addWidget(m_packetCountLabel);
    statusRow->addStretch();

    mainLayout->addLayout(statusRow);

    // ── 中间: 数据包列表 + 详情面板 (上下分栏) ──
    auto* mainSplitter = new QSplitter(Qt::Vertical);

    // 上半部分: 左侧数据包列表 + 右侧协议统计
    auto* topSplitter = new QSplitter(Qt::Horizontal);

    // 数据包列表
    auto* packetListWidget = new QWidget();
    auto* packetListLayout = new QVBoxLayout(packetListWidget);
    packetListLayout->setContentsMargins(0, 0, 0, 0);
    packetListLayout->setSpacing(4);

    auto* packetListTitle = new QLabel("数据包列表");
    packetListTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #DCDCDC; padding: 2px 0;");
    packetListLayout->addWidget(packetListTitle);

    m_packetTable = new QTableWidget(0, 7);
    m_packetTable->setHorizontalHeaderLabels({"#", "时间", "源地址", "目标地址", "协议", "长度", "信息"});
    m_packetTable->horizontalHeader()->setStretchLastSection(true);
    m_packetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_packetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_packetTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_packetTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_packetTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_packetTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_packetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packetTable->setAlternatingRowColors(true);
    m_packetTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_packetTable->verticalHeader()->setVisible(false);
    m_packetTable->setColumnWidth(2, 160);
    m_packetTable->setColumnWidth(3, 160);
    m_packetTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  gridline-color: #3C3F41; border: 1px solid #3C3F41;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 3px 6px; }"
        "QHeaderView::section {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  border: none; border-bottom: 2px solid #3C3F41;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #25262B; }"
        "QTableWidget::item:selected { background-color: #409EFF; color: white; }"
    );
    packetListLayout->addWidget(m_packetTable, 1);

    topSplitter->addWidget(packetListWidget);

    // 协议统计 (右侧)
    auto* statWidget = new QWidget();
    auto* statLayout = new QVBoxLayout(statWidget);
    statLayout->setContentsMargins(0, 0, 0, 0);
    statLayout->setSpacing(4);

    auto* statTitle = new QLabel("协议层次统计");
    statTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #DCDCDC; padding: 2px 0;");
    statLayout->addWidget(statTitle);

    m_protocolTree = new QTreeWidget();
    m_protocolTree->setHeaderLabels({"协议", "数量", "占比"});
    m_protocolTree->setColumnWidth(0, 120);
    m_protocolTree->setColumnWidth(1, 60);
    m_protocolTree->setColumnWidth(2, 60);
    m_protocolTree->setAlternatingRowColors(true);
    m_protocolTree->setRootIsDecorated(true);
    m_protocolTree->setStyleSheet(
        "QTreeWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41;"
        "  font-size: 12px;"
        "}"
        "QTreeWidget::item { padding: 3px 6px; }"
        "QHeaderView::section {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  border: none; border-bottom: 2px solid #3C3F41;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
        "QTreeWidget::item:alternate { background-color: #25262B; }"
    );
    statLayout->addWidget(m_protocolTree, 1);

    topSplitter->addWidget(statWidget);
    topSplitter->setStretchFactor(0, 3);
    topSplitter->setStretchFactor(1, 1);

    mainSplitter->addWidget(topSplitter);

    // 下半部分: 数据包详情 (hex dump + ASCII)
    auto* detailWidget = new QWidget();
    auto* detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(4);

    auto* detailTitle = new QLabel("数据包详情 (Hex Dump)");
    detailTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #DCDCDC; padding: 2px 0;");
    detailLayout->addWidget(detailTitle);

    m_detailView = new QPlainTextEdit();
    m_detailView->setReadOnly(true);
    m_detailView->setPlaceholderText("选择数据包列表中的条目以查看 HEX 转储和 ASCII 内容");
    m_detailView->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41;"
        "  font-family: 'Menlo', 'Consolas', 'Courier New', monospace;"
        "  font-size: 12px;"
        "  padding: 8px;"
        "}"
    );
    m_detailView->setMinimumHeight(120);
    detailLayout->addWidget(m_detailView, 1);

    mainSplitter->addWidget(detailWidget);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter, 1);
}

// ============================================================================
// 信号连接
// ============================================================================

void PacketCaptureWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &PacketCaptureWidget::onStartCapture);
    connect(m_stopBtn, &QPushButton::clicked, this, &PacketCaptureWidget::onStopCapture);
    connect(m_clearBtn, &QPushButton::clicked, this, &PacketCaptureWidget::onClearCapture);
    connect(m_exportBtn, &QPushButton::clicked, this, &PacketCaptureWidget::onExportPcap);
    connect(m_refreshInterfaceBtn, &QPushButton::clicked, this, &PacketCaptureWidget::onInterfaceRefresh);
    connect(m_packetTable, &QTableWidget::cellClicked, this, &PacketCaptureWidget::onPacketSelected);
    connect(m_filterEdit, &QLineEdit::returnPressed, this, &PacketCaptureWidget::onStartCapture);

    // 右键菜单
    connect(m_packetTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem* item = m_packetTable->itemAt(pos);
        if (!item) return;

        int row = item->row();
        if (row < 0 || row >= m_packets.size()) return;

        QMenu menu(this);
        QAction* copySrcAction = menu.addAction("复制源地址");
        QAction* copyDstAction = menu.addAction("复制目标地址");
        QAction* copyInfoAction = menu.addAction("复制信息");
        QAction* copyRowAction = menu.addAction("复制整行");
        menu.addSeparator();
        QAction* copyFilterAction = menu.addAction("设为过滤器（源地址）");

        QAction* chosen = menu.exec(m_packetTable->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        const PacketInfo& pkt = m_packets[row];
        if (chosen == copySrcAction) {
            QApplication::clipboard()->setText(pkt.source);
            Logger::instance().info("PacketCapture", "已复制源地址: " + pkt.source);
        } else if (chosen == copyDstAction) {
            QApplication::clipboard()->setText(pkt.destination);
            Logger::instance().info("PacketCapture", "已复制目标地址: " + pkt.destination);
        } else if (chosen == copyInfoAction) {
            QApplication::clipboard()->setText(pkt.info);
            Logger::instance().info("PacketCapture", "已复制信息: " + pkt.info);
        } else if (chosen == copyRowAction) {
            QString rowText = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
                                  .arg(pkt.number)
                                  .arg(pkt.timestamp)
                                  .arg(pkt.source)
                                  .arg(pkt.destination)
                                  .arg(pkt.protocol)
                                  .arg(pkt.length)
                                  .arg(pkt.info);
            QApplication::clipboard()->setText(rowText);
            Logger::instance().info("PacketCapture", "已复制整行");
        } else if (chosen == copyFilterAction) {
            // 提取纯 IP 地址
            QString srcIp = pkt.source;
            if (srcIp.contains('.')) {
                // 去除端口号
                int dotIdx = srcIp.lastIndexOf('.');
                if (dotIdx > 0) {
                    // 检查最后一个点后是否是端口号
                    QString afterDot = srcIp.mid(dotIdx + 1);
                    bool isPort = true;
                    for (const QChar& c : afterDot) {
                        if (!c.isDigit()) { isPort = false; break; }
                    }
                    if (isPort) {
                        srcIp = srcIp.left(dotIdx);
                    }
                }
            }
            m_filterEdit->setText("host " + srcIp);
            Logger::instance().info("PacketCapture", "过滤器已设置: host " + srcIp);
        }
    });
}

// ============================================================================
// 网卡检测
// ============================================================================

void PacketCaptureWidget::detectInterfaces()
{
    m_interfaceCombo->clear();

    // macOS / Linux 通用
    QProcess proc;
    QStringList ifaces;

    // 优先尝试 ip link (Linux)
    proc.start("ip", QStringList() << "-br" << "link" << "show");
    if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 2) {
                QString iface = parts[0];
                if (iface != "lo" && !iface.isEmpty()) {
                    ifaces.append(iface);
                }
            }
        }
    }

    // 回退到 ifconfig (macOS)
    if (ifaces.isEmpty()) {
        proc.start("ifconfig", QStringList());
        if (proc.waitForFinished(3000)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput());
            QStringList lines = output.split('\n');
            for (const QString& line : lines) {
                if (line.isEmpty() || line.startsWith('\t') || line.startsWith(' ')) continue;
                int colonIdx = line.indexOf(':');
                if (colonIdx > 0) {
                    QString iface = line.left(colonIdx).trimmed();
                    if (iface != "lo0" && !iface.isEmpty()) {
                        ifaces.append(iface);
                    }
                }
            }
        }
    }

    // 回退到 -l 参数 (列出可用网卡, macOS tcpdump)
    if (ifaces.isEmpty()) {
        proc.start("tcpdump", QStringList() << "-D");
        if (proc.waitForFinished(3000)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            for (const QString& line : lines) {
                // 格式: "1.en0 (Wi-Fi)" 或 "1.en0"
                static QRegularExpression re(R"(^\d+\.(\S+))");
                QRegularExpressionMatch match = re.match(line.trimmed());
                if (match.hasMatch()) {
                    ifaces.append(match.captured(1));
                }
            }
        }
    }

    if (ifaces.isEmpty()) {
        m_interfaceCombo->addItem("any (所有网卡)", "any");
    } else {
        m_interfaceCombo->addItem("any (所有网卡)", "any");
        for (const QString& iface : ifaces) {
            m_interfaceCombo->addItem(iface, iface);
        }
    }

    // 默认选择 en0 (macOS) 或 eth0 (Linux)
    int en0Idx = m_interfaceCombo->findData("en0");
    if (en0Idx >= 0) {
        m_interfaceCombo->setCurrentIndex(en0Idx);
    } else {
        int eth0Idx = m_interfaceCombo->findData("eth0");
        if (eth0Idx >= 0) {
            m_interfaceCombo->setCurrentIndex(eth0Idx);
        }
    }

    Logger::instance().info("PacketCapture", QString("检测到 %1 个网卡").arg(ifaces.size()));
}

void PacketCaptureWidget::onInterfaceRefresh()
{
    detectInterfaces();
    m_statusLabel->setText("网卡列表已刷新");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #409EFF; font-weight: bold;");
}

// ============================================================================
// sudo 权限检查
// ============================================================================

bool PacketCaptureWidget::checkSudoWarning()
{
    // 检查 tcpdump 是否需要 sudo (macOS 通常需要)
    // 通过尝试运行 tcpdump -D 来检测权限
    QProcess testProc;
    testProc.start("tcpdump", QStringList() << "-D");
    testProc.waitForFinished(3000);

    QString stdErr = QString::fromUtf8(testProc.readAllStandardError());
    if (testProc.exitCode() != 0 && stdErr.contains("permission", Qt::CaseInsensitive)) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("权限提示");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("tcpdump 需要管理员权限才能捕获数据包。");
        msgBox.setInformativeText("请确保:\n"
                                  "1. 当前用户具有 sudo 权限\n"
                                  "2. 或已将 tcpdump 设置为无需密码运行\n"
                                  "3. 或使用 root 用户运行本程序\n\n"
                                  "是否继续尝试启动捕获？");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        return msgBox.exec() == QMessageBox::Yes;
    }
    return true;
}

// ============================================================================
// 构建 tcpdump 参数
// ============================================================================

QString PacketCaptureWidget::buildTcpdumpArgs()
{
    // 使用 tcpdump -n -l -XX 获取 hex dump
    // -n: 不解析主机名
    // -l: 行缓冲输出
    // -XX: 显示 hex + ASCII (含链路层头)
    return QString("tcpdump -n -l -XX");
}

// ============================================================================
// 开始捕获
// ============================================================================

void PacketCaptureWidget::onStartCapture()
{
    if (m_capturing) return;

    if (!checkSudoWarning()) {
        return;
    }

    QString iface = m_interfaceCombo->currentData().toString();
    m_currentFilter = m_filterEdit->text().trimmed();
    m_maxPackets = m_packetCountSpin->value();

    // 清理旧进程
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    // 清理上次捕获数据
    m_packets.clear();
    m_rawOutput.clear();
    m_packetCount = 0;
    m_protocolCounts.clear();
    m_currentBuf = PacketBuffer();
    m_inHexDump = false;

    m_packetTable->setRowCount(0);
    m_protocolTree->clear();
    m_detailView->clear();
    m_packetCountLabel->setText("捕获: 0 包");

    // 构建参数
    QStringList args;
    args << "-n" << "-l" << "-X";

    // 网卡
    if (iface != "any") {
        args << "-i" << iface;
    }

    // 包数量限制
    if (m_maxPackets > 0) {
        args << "-c" << QString::number(m_maxPackets);
    }

    // 过滤器
    if (!m_currentFilter.isEmpty()) {
        // 按空格分割过滤器
        QStringList filterParts = m_currentFilter.split(' ', Qt::SkipEmptyParts);
        args << filterParts;
    }

    // 启动进程
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &PacketCaptureWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PacketCaptureWidget::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &PacketCaptureWidget::onProcessError);

    m_process->start("sudo", QStringList() << "tcpdump" << args);

    m_capturing = true;
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_exportBtn->setEnabled(false);
    m_statusLabel->setText("正在捕获...");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #67C23A; font-weight: bold;");

    Logger::instance().info("PacketCapture",
                            QString("开始捕获: sudo tcpdump %1").arg(args.join(' ')));
}

// ============================================================================
// 停止捕获
// ============================================================================

void PacketCaptureWidget::onStopCapture()
{
    if (!m_capturing || !m_process) return;

    m_process->kill();
    m_process->waitForFinished(2000);
    m_capturing = false;

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_exportBtn->setEnabled(m_packets.size() > 0);
    m_statusLabel->setText(QString("已停止 — 捕获 %1 包").arg(m_packetCount));
    m_statusLabel->setStyleSheet("font-size: 13px; color: #E6A23C; font-weight: bold;");

    Logger::instance().info("PacketCapture",
                            QString("捕获已停止，共 %1 包").arg(m_packetCount));
}

// ============================================================================
// 清除捕获
// ============================================================================

void PacketCaptureWidget::onClearCapture()
{
    if (m_capturing) {
        onStopCapture();
    }

    m_packets.clear();
    m_rawOutput.clear();
    m_packetCount = 0;
    m_protocolCounts.clear();
    m_currentBuf = PacketBuffer();
    m_inHexDump = false;

    m_packetTable->setRowCount(0);
    m_protocolTree->clear();
    m_detailView->clear();
    m_packetCountLabel->setText("捕获: 0 包");
    m_statusLabel->setText("就绪 — 等待开始捕获");
    m_statusLabel->setStyleSheet("font-size: 13px; color: #67C23A; font-weight: bold;");
    m_exportBtn->setEnabled(false);

    Logger::instance().info("PacketCapture", "捕获数据已清除");
}

// ============================================================================
// 导出 PCAP
// ============================================================================

void PacketCaptureWidget::onExportPcap()
{
    if (m_packets.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    // 导出为原始 tcpdump 文本格式 (近似 PCAP 文本表示)
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出捕获数据",
        QString("capture_%1.pcap").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "PCAP 文件 (*.pcap);;文本文件 (*.txt);;所有文件 (*)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "# Wukong Network Toolkit - Packet Capture\n";
    out << "# Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "# Filter: " << (m_currentFilter.isEmpty() ? "none" : m_currentFilter) << "\n";
    out << "# Packets: " << m_packetCount << "\n";
    out << "#\n";
    out << "# Format: tcpdump -n -l -X output\n";
    out << "#\n\n";

    for (const PacketInfo& pkt : m_packets) {
        out << pkt.rawLine << "\n";
        if (!pkt.hexData.isEmpty()) {
            out << QString::fromUtf8(pkt.hexData) << "\n";
        }
    }

    file.close();
    Logger::instance().info("PacketCapture",
                            QString("捕获数据已导出到: %1 (%2 包)").arg(filePath).arg(m_packets.size()));
    QMessageBox::information(this, "导出成功",
                             QString("已导出 %1 个数据包到:\n%2").arg(m_packets.size()).arg(filePath));
}

// ============================================================================
// 数据包选择 → 显示 Hex Dump
// ============================================================================

void PacketCaptureWidget::onPacketSelected(int row, int col)
{
    Q_UNUSED(col)
    if (row < 0 || row >= m_packets.size()) return;

    showHexDump(m_packets[row]);
}

void PacketCaptureWidget::showHexDump(const PacketInfo& pkt)
{
    if (pkt.hexData.isEmpty()) {
        m_detailView->setPlainText("该数据包没有 HEX 转储数据。\n"
                                   "请确保使用 -X 参数捕获。");
        return;
    }

    m_detailView->setPlainText(QString::fromUtf8(pkt.hexData));
}

// ============================================================================
// 解析 tcpdump 输出
// ============================================================================

void PacketCaptureWidget::onProcessReadyRead()
{
    if (!m_process) return;

    QByteArray data = m_process->readAll();
    m_rawOutput.append(QString::fromUtf8(data));

    // 逐行解析
    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n');

    for (const QString& line : lines) {
        if (line.isEmpty()) continue;

        QString trimmed = line;

        // 跳过 tcpdump 的头部信息
        if (trimmed.startsWith("tcpdump:") || trimmed.startsWith("listening on")) {
            continue;
        }

        // 判断是否是 hex dump 行 (以 tab 或空格开头，包含 0x)
        if (trimmed.startsWith('\t') || trimmed.startsWith("    0x")) {
            // hex dump 行
            m_inHexDump = true;
            m_currentBuf.hexLines.append(trimmed);
            continue;
        }

        // 不是 hex dump 行，且之前有缓冲的 hex dump → 完成上一个数据包
        if (m_inHexDump && !m_currentBuf.summaryLine.isEmpty()) {
            // 将 hex dump 附加到上一个数据包
            if (!m_packets.isEmpty()) {
                PacketInfo& lastPkt = m_packets.last();
                lastPkt.hexData = m_currentBuf.hexLines.join('\n').toUtf8();
            }
            m_currentBuf = PacketBuffer();
            m_inHexDump = false;
        }

        // 检查是否是数据包摘要行 (以时间戳开头)
        // tcpdump -n 格式: "HH:MM:SS.micros IP src > dst: ..."
        // 或 "HH:MM:SS.micros ARP, ..."
        static QRegularExpression tsRe(R"(^(\d{2}:\d{2}:\d{2}\.\d+)\s+(IP6?|ARP|)\s)");
        QRegularExpressionMatch tsMatch = tsRe.match(trimmed);
        if (tsMatch.hasMatch()) {
            // 如果有未完成的 hex 缓冲，先保存到上一个包
            if (!m_currentBuf.hexLines.isEmpty() && !m_packets.isEmpty()) {
                PacketInfo& lastPkt = m_packets.last();
                lastPkt.hexData = m_currentBuf.hexLines.join('\n').toUtf8();
                m_currentBuf = PacketBuffer();
            }
            m_inHexDump = false;

            // 解析新数据包
            m_currentBuf.summaryLine = trimmed;
            parsePacketLine(trimmed);
        }
    }
}

void PacketCaptureWidget::parsePacketLine(const QString& line)
{
    PacketInfo pkt;
    pkt.number = ++m_packetCount;
    pkt.rawLine = line;

    // 解析时间戳
    static QRegularExpression tsRe(R"(^(\d{2}:\d{2}:\d{2}\.\d+))");
    QRegularExpressionMatch tsMatch = tsRe.match(line);
    if (tsMatch.hasMatch()) {
        pkt.timestamp = tsMatch.captured(1);
    }

    // 解析协议
    pkt.protocol = parseProtocol(line);

    // 解析源/目标
    parseSourceDest(line, pkt.source, pkt.destination);

    // 解析长度
    pkt.length = parseLength(line);

    // 解析信息
    pkt.info = parseInfo(line);

    // 如果协议是 TCP/UDP，尝试根据端口细化
    if (pkt.protocol == "TCP" || pkt.protocol == "UDP") {
        QString srcPort, dstPort;
        static QRegularExpression portRe(R"((\d+)\.(\d+)\s+>\s+(\d+)\.(\d+))");
        QRegularExpressionMatch portMatch = portRe.match(line);
        if (portMatch.hasMatch()) {
            srcPort = portMatch.captured(2);
            dstPort = portMatch.captured(4);
        }
        QString refined = protocolFromPort(srcPort, dstPort, pkt.protocol);
        if (refined != pkt.protocol) {
            pkt.protocol = refined;
        }
    }

    // 统计协议
    m_protocolCounts[pkt.protocol]++;

    // 添加到列表
    m_packets.append(pkt);
    addPacketRow(pkt);
    updateProtocolStats();

    m_packetCountLabel->setText(QString("捕获: %1 包").arg(m_packetCount));
}

// ============================================================================
// 协议解析
// ============================================================================

QString PacketCaptureWidget::parseProtocol(const QString& line)
{
    if (line.contains("IP6")) return "IPv6";
    if (line.contains("ARP")) return "ARP";
    if (line.contains("ICMP6")) return "ICMPv6";
    if (line.contains("ICMP")) return "ICMP";
    if (line.contains("UDP")) return "UDP";
    if (line.contains("TCP")) return "TCP";
    if (line.contains("IGMP")) return "IGMP";
    if (line.contains("STP")) return "STP";
    if (line.contains("LLDP")) return "LLDP";
    if (line.contains("CDP")) return "CDP";
    if (line.contains("OSPF")) return "OSPF";
    if (line.contains("BGP")) return "BGP";
    if (line.contains("VRRP")) return "VRRP";
    if (line.contains("DHCP")) return "DHCP";
    return "Other";
}

// ============================================================================
// 源/目标地址解析
// ============================================================================

void PacketCaptureWidget::parseSourceDest(const QString& line, QString& src, QString& dst)
{
    src = "-";
    dst = "-";

    // ARP 格式: "ARP, Request who-has 192.168.1.1 tell 192.168.1.2"
    if (line.contains("ARP")) {
        static QRegularExpression arpWho(R"(who-has\s+(\S+))");
        static QRegularExpression arpTell(R"(tell\s+(\S+))");
        static QRegularExpression arpReply(R"((\S+)\s+is-at\s+(\S+))");

        QRegularExpressionMatch whoMatch = arpWho.match(line);
        QRegularExpressionMatch tellMatch = arpTell.match(line);
        QRegularExpressionMatch replyMatch = arpReply.match(line);

        if (whoMatch.hasMatch() && tellMatch.hasMatch()) {
            src = tellMatch.captured(1);
            dst = whoMatch.captured(1);
        } else if (replyMatch.hasMatch()) {
            src = replyMatch.captured(1);
            dst = "Broadcast";
        }
        return;
    }

    // IP 格式: "IP src.port > dst.port: ..." 或 "IP src > dst: ..."
    static QRegularExpression ipRe(R"(IP6?\s+(\S+)\s+>\s+(\S+):)");
    QRegularExpressionMatch ipMatch = ipRe.match(line);
    if (ipMatch.hasMatch()) {
        src = ipMatch.captured(1);
        dst = ipMatch.captured(2);
        return;
    }

    // 其他格式: 尝试提取 IP 地址
    static QRegularExpression anyIpRe(R"((\d+\.\d+\.\d+\.\d+))");
    QRegularExpressionMatchIterator it = anyIpRe.globalMatch(line);
    QStringList ips;
    while (it.hasNext()) {
        ips.append(it.next().captured(1));
    }
    if (ips.size() >= 2) {
        src = ips[0];
        dst = ips[1];
    } else if (ips.size() == 1) {
        src = ips[0];
    }
}

// ============================================================================
// 长度解析
// ============================================================================

int PacketCaptureWidget::parseLength(const QString& line)
{
    // tcpdump 格式: "... length 64" 或 "... length: 64"
    static QRegularExpression lenRe(R"(length[:\s]+(\d+))");
    QRegularExpressionMatch match = lenRe.match(line);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 0;
}

// ============================================================================
// 信息解析
// ============================================================================

QString PacketCaptureWidget::parseInfo(const QString& line)
{
    // 提取冒号后面的信息部分
    int colonIdx = line.indexOf(": ");
    if (colonIdx < 0) {
        // 对于 ARP 等格式，整行都是信息
        if (line.contains("ARP")) {
            static QRegularExpression arpRe(R"(ARP,\s+(.+)$)");
            QRegularExpressionMatch m = arpRe.match(line);
            if (m.hasMatch()) return m.captured(1);
        }
        return line;
    }

    QString info = line.mid(colonIdx + 2).trimmed();

    // 截断过长的信息
    if (info.length() > 200) {
        info = info.left(200) + "...";
    }

    return info;
}

// ============================================================================
// 添加数据包行到表格
// ============================================================================

void PacketCaptureWidget::addPacketRow(const PacketInfo& pkt)
{
    int row = m_packetTable->rowCount();
    m_packetTable->insertRow(row);

    // #
    auto* numItem = new QTableWidgetItem(QString::number(pkt.number));
    numItem->setTextAlignment(Qt::AlignCenter);
    m_packetTable->setItem(row, 0, numItem);

    // 时间
    m_packetTable->setItem(row, 1, new QTableWidgetItem(pkt.timestamp));

    // 源地址
    m_packetTable->setItem(row, 2, new QTableWidgetItem(pkt.source));

    // 目标地址
    m_packetTable->setItem(row, 3, new QTableWidgetItem(pkt.destination));

    // 协议 (着色)
    auto* protoItem = new QTableWidgetItem(pkt.protocol);
    protoItem->setTextAlignment(Qt::AlignCenter);
    QColor protoColor;
    if (pkt.protocol == "TCP") protoColor = QColor(0x40, 0x9E, 0xFF);
    else if (pkt.protocol == "UDP") protoColor = QColor(0x67, 0xC2, 0x3A);
    else if (pkt.protocol == "ICMP" || pkt.protocol == "ICMPv6") protoColor = QColor(0xE6, 0xA2, 0x3C);
    else if (pkt.protocol == "ARP") protoColor = QColor(0x9B, 0x59, 0xB6);
    else if (pkt.protocol == "DNS") protoColor = QColor(0xF5, 0x6C, 0x6C);
    else if (pkt.protocol == "HTTP" || pkt.protocol == "HTTPS") protoColor = QColor(0x00, 0xB4, 0xD8);
    else if (pkt.protocol == "TLS" || pkt.protocol == "SSL") protoColor = QColor(0xFF, 0x85, 0x1B);
    else if (pkt.protocol == "SSH") protoColor = QColor(0x2C, 0xCC, 0x70);
    else protoColor = QColor(0xDC, 0xDC, 0xDC);
    protoItem->setForeground(protoColor);
    m_packetTable->setItem(row, 4, protoItem);

    // 长度
    auto* lenItem = new QTableWidgetItem(pkt.length > 0 ? QString::number(pkt.length) : "-");
    lenItem->setTextAlignment(Qt::AlignCenter);
    m_packetTable->setItem(row, 5, lenItem);

    // 信息
    m_packetTable->setItem(row, 6, new QTableWidgetItem(pkt.info));

    m_packetTable->scrollToBottom();
}

// ============================================================================
// 更新协议统计
// ============================================================================

void PacketCaptureWidget::updateProtocolStats()
{
    m_protocolTree->clear();

    int total = m_packetCount;
    if (total == 0) return;

    // 按数量降序排列
    QList<QPair<QString, int>> sorted;
    for (auto it = m_protocolCounts.begin(); it != m_protocolCounts.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for (const auto& pair : sorted) {
        auto* item = new QTreeWidgetItem(m_protocolTree);
        item->setText(0, pair.first);
        item->setText(1, QString::number(pair.second));
        double pct = (double)pair.second / total * 100.0;
        item->setText(2, QString("%1%").arg(pct, 0, 'f', 1));

        // 着色
        QColor c;
        if (pair.first == "TCP") c = QColor(0x40, 0x9E, 0xFF);
        else if (pair.first == "UDP") c = QColor(0x67, 0xC2, 0x3A);
        else if (pair.first == "ICMP" || pair.first == "ICMPv6") c = QColor(0xE6, 0xA2, 0x3C);
        else if (pair.first == "ARP") c = QColor(0x9B, 0x59, 0xB6);
        else if (pair.first == "DNS") c = QColor(0xF5, 0x6C, 0x6C);
        else if (pair.first == "HTTP" || pair.first == "HTTPS") c = QColor(0x00, 0xB4, 0xD8);
        else c = QColor(0xDC, 0xDC, 0xDC);
        item->setForeground(0, c);
    }

    m_protocolTree->expandAll();
}

// ============================================================================
// 进程回调
// ============================================================================

void PacketCaptureWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    Q_UNUSED(exitCode)

    m_capturing = false;

    // 处理剩余的缓冲数据
    if (m_inHexDump && !m_currentBuf.hexLines.isEmpty() && !m_packets.isEmpty()) {
        PacketInfo& lastPkt = m_packets.last();
        lastPkt.hexData = m_currentBuf.hexLines.join('\n').toUtf8();
        m_currentBuf = PacketBuffer();
        m_inHexDump = false;
    }

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_exportBtn->setEnabled(m_packets.size() > 0);

    if (m_packetCount > 0) {
        m_statusLabel->setText(QString("捕获完成 — 共 %1 包").arg(m_packetCount));
        m_statusLabel->setStyleSheet("font-size: 13px; color: #409EFF; font-weight: bold;");
    } else {
        m_statusLabel->setText("捕获完成 — 无数据包");
        m_statusLabel->setStyleSheet("font-size: 13px; color: #909090; font-weight: bold;");
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    Logger::instance().info("PacketCapture",
                            QString("捕获进程结束，共 %1 包").arg(m_packetCount));
}

void PacketCaptureWidget::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)

    QString errMsg = m_process ? m_process->errorString() : "未知错误";

    if (m_capturing) {
        m_capturing = false;
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);

        m_statusLabel->setText("捕获失败: " + errMsg);
        m_statusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; font-weight: bold;");

        Logger::instance().error("PacketCapture", "捕获进程错误: " + errMsg);

        QMessageBox::warning(this, "捕获错误",
                             QString("tcpdump 启动失败:\n%1\n\n"
                                     "请检查:\n"
                                     "1. tcpdump 是否已安装\n"
                                     "2. 是否有 sudo 权限\n"
                                     "3. 网卡名称是否正确").arg(errMsg));
    }
}