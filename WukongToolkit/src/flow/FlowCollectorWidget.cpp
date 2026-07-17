#include "flow/FlowCollectorWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <QFile>
#include <QApplication>
#include <QSplitter>
#include <QMap>
#include <QSet>
#include <QFrame>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════════
// Protocol name mapping
// ═══════════════════════════════════════════════════════════════════════════════
static QStringList protocolNames()
{
    static const QStringList names = {
        "HOPOPT",   // 0
        "ICMP",     // 1
        "IGMP",     // 2
        "GGP",      // 3
        "IPv4",     // 4
        "ST",       // 5
        "TCP",      // 6
        "CBT",      // 7
        "EGP",      // 8
        "IGP",      // 9
        "BBN-RCC",  // 10
        "NVP-II",   // 11
        "PUP",      // 12
        "ARGUS",    // 13
        "EMCON",    // 14
        "XNET",     // 15
        "CHAOS",    // 16
        "UDP",      // 17
        "MUX",      // 18
    };
    return names;
}

// ═══════════════════════════════════════════════════════════════════════════════
// App detection by port
// ═══════════════════════════════════════════════════════════════════════════════
static QString detectAppByPort(quint16 port, quint8 protocol)
{
    switch (port) {
        case 20:  return QStringLiteral("FTP-DATA");
        case 21:  return QStringLiteral("FTP");
        case 22:  return QStringLiteral("SSH");
        case 23:  return QStringLiteral("Telnet");
        case 25:  return QStringLiteral("SMTP");
        case 53:  return QStringLiteral("DNS");
        case 67:  return QStringLiteral("DHCP-Server");
        case 68:  return QStringLiteral("DHCP-Client");
        case 80:  return QStringLiteral("HTTP");
        case 110: return QStringLiteral("POP3");
        case 123: return QStringLiteral("NTP");
        case 143: return QStringLiteral("IMAP");
        case 161: return QStringLiteral("SNMP");
        case 162: return QStringLiteral("SNMP-Trap");
        case 179: return QStringLiteral("BGP");
        case 389: return QStringLiteral("LDAP");
        case 443: return QStringLiteral("HTTPS");
        case 445: return QStringLiteral("SMB");
        case 514: return QStringLiteral("Syslog");
        case 993: return QStringLiteral("IMAPS");
        case 995: return QStringLiteral("POP3S");
        case 1433: return QStringLiteral("MSSQL");
        case 1521: return QStringLiteral("Oracle");
        case 3306: return QStringLiteral("MySQL");
        case 3389: return QStringLiteral("RDP");
        case 5432: return QStringLiteral("PostgreSQL");
        case 6379: return QStringLiteral("Redis");
        case 8080: return QStringLiteral("HTTP-Alt");
        case 9200: return QStringLiteral("Elasticsearch");
        case 27017: return QStringLiteral("MongoDB");
        default: break;
    }
    if (protocol == 6)  return QStringLiteral("TCP/%1").arg(port);
    if (protocol == 17) return QStringLiteral("UDP/%1").arg(port);
    return QStringLiteral("IP/%1/%2").arg(port).arg(protocol);
}

// ═══════════════════════════════════════════════════════════════════════════════
// FlowCollectorWidget
// ═══════════════════════════════════════════════════════════════════════════════

FlowCollectorWidget::FlowCollectorWidget(QWidget* parent)
    : QWidget(parent)
    , m_portSpin(nullptr)
    , m_startStopBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_flowTable(nullptr)
    , m_topTalkerTable(nullptr)
    , m_topAppTable(nullptr)
    , m_exportBtn(nullptr)
    , m_udpSocket(nullptr)
    , m_statsTimer(nullptr)
    , m_running(false)
    , m_packetCount(0)
    , m_flowCount(0)
{
    setupUI();
    setupConnections();
    loadDemoRecords();
}

FlowCollectorWidget::~FlowCollectorWidget()
{
    if (m_running) {
        m_statsTimer->stop();
        if (m_udpSocket) {
            m_udpSocket->close();
        }
    }
}

// ─── IP / Protocol helpers ──────────────────────────────────────────────────

QString FlowCollectorWidget::ipToString(quint32 addr) const
{
    return QString("%1.%2.%3.%4")
        .arg((addr >> 24) & 0xFF)
        .arg((addr >> 16) & 0xFF)
        .arg((addr >> 8) & 0xFF)
        .arg(addr & 0xFF);
}

QString FlowCollectorWidget::protocolName(quint8 protocol) const
{
    const auto& names = protocolNames();
    if (protocol < static_cast<quint8>(names.size())) {
        return names[protocol];
    }
    return QString::number(protocol);
}

QString FlowCollectorWidget::detectApp(quint16 port, quint8 protocol) const
{
    return detectAppByPort(port, protocol);
}

// ─── UI Setup ───────────────────────────────────────────────────────────────

void FlowCollectorWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 顶部控制区 ──
    auto* controlGroup = new QGroupBox(QStringLiteral("Flow Collector 控制"));
    auto* controlLayout = new QHBoxLayout(controlGroup);
    controlLayout->setSpacing(12);

    auto* portLabel = new QLabel(QStringLiteral("UDP 监听端口:"));
    

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(2055);
    m_portSpin->setFixedWidth(90);
    m_portSpin->setStyleSheet(QStringLiteral(
        "QSpinBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    ));
    controlLayout->addWidget(portLabel);
    controlLayout->addWidget(m_portSpin);

    m_startStopBtn = new QPushButton(QStringLiteral("启动"));
    m_startStopBtn->setFixedHeight(34);
    m_startStopBtn->setMinimumWidth(80);
    m_startStopBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 6px 18px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    ));
    controlLayout->addWidget(m_startStopBtn);

    m_statusLabel = new QLabel(QStringLiteral("已停止"));
    m_statusLabel->setStyleSheet("color: #888; font-weight: bold; font-size: 13px;");
    controlLayout->addWidget(m_statusLabel);

    controlLayout->addStretch();

    m_exportBtn = new QPushButton(QStringLiteral("导出 CSV"));
    m_exportBtn->setFixedHeight(34);
    m_exportBtn->setMinimumWidth(90);
    m_exportBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 6px 16px; border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    ));
    controlLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(controlGroup);

    // ── 中部 Flow 表格 ──
    auto* flowGroup = new QGroupBox(QStringLiteral("流记录"));
    auto* flowLayout = new QVBoxLayout(flowGroup);
    flowLayout->setContentsMargins(4, 4, 4, 4);

    m_flowTable = new QTableWidget();
    m_flowTable->setColumnCount(9);
    QStringList flowHeaders = {
        QStringLiteral("源 IP"),
        QStringLiteral("目标 IP"),
        QStringLiteral("源端口"),
        QStringLiteral("目标端口"),
        QStringLiteral("协议"),
        QStringLiteral("包数"),
        QStringLiteral("字节数"),
        QStringLiteral("开始时间"),
        QStringLiteral("结束时间")
    };
    m_flowTable->setHorizontalHeaderLabels(flowHeaders);
    m_flowTable->horizontalHeader()->setStretchLastSection(true);
    m_flowTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_flowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_flowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_flowTable->setAlternatingRowColors(true);
    m_flowTable->verticalHeader()->setVisible(false);
    m_flowTable->setSortingEnabled(false);
    m_flowTable->setColumnWidth(0, 130);
    m_flowTable->setColumnWidth(1, 130);
    m_flowTable->setColumnWidth(2, 80);
    m_flowTable->setColumnWidth(3, 80);
    m_flowTable->setColumnWidth(4, 80);
    m_flowTable->setColumnWidth(5, 80);
    m_flowTable->setColumnWidth(6, 100);
    m_flowTable->setColumnWidth(7, 150);
    m_flowTable->setColumnWidth(8, 150);
    m_flowTable->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; font-size: 12px;"
        "  gridline-color: #30363D;"
        "}"
        "QTableWidget::item { padding: 3px 4px; }"
        "QTableWidget::item:selected { background-color: #30363D; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
    ));
    flowLayout->addWidget(m_flowTable);
    mainLayout->addWidget(flowGroup, 2);

    // ── 底部：Top Talker + Top Application (左右分栏) ──
    auto* bottomSplitter = new QSplitter(Qt::Horizontal);

    // Top Talker
    auto* ttGroup = new QGroupBox(QStringLiteral("Top Talker"));
    auto* ttLayout = new QVBoxLayout(ttGroup);
    ttLayout->setContentsMargins(4, 4, 4, 4);

    m_topTalkerTable = new QTableWidget();
    m_topTalkerTable->setColumnCount(4);
    QStringList ttHeaders = {
        QStringLiteral("源 IP"),
        QStringLiteral("字节数"),
        QStringLiteral("流数"),
        QStringLiteral("占比")
    };
    m_topTalkerTable->setHorizontalHeaderLabels(ttHeaders);
    m_topTalkerTable->horizontalHeader()->setStretchLastSection(true);
    m_topTalkerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_topTalkerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topTalkerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_topTalkerTable->setAlternatingRowColors(true);
    m_topTalkerTable->verticalHeader()->setVisible(false);
    m_topTalkerTable->setSortingEnabled(false);
    m_topTalkerTable->setColumnWidth(0, 140);
    m_topTalkerTable->setColumnWidth(1, 110);
    m_topTalkerTable->setColumnWidth(2, 80);
    m_topTalkerTable->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; font-size: 12px;"
        "  gridline-color: #30363D;"
        "}"
        "QTableWidget::item { padding: 3px 4px; }"
        "QTableWidget::item:selected { background-color: #30363D; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
    ));
    ttLayout->addWidget(m_topTalkerTable);
    bottomSplitter->addWidget(ttGroup);

    // Top Application
    auto* taGroup = new QGroupBox(QStringLiteral("Top Application"));
    auto* taLayout = new QVBoxLayout(taGroup);
    taLayout->setContentsMargins(4, 4, 4, 4);

    m_topAppTable = new QTableWidget();
    m_topAppTable->setColumnCount(5);
    QStringList taHeaders = {
        QStringLiteral("应用"),
        QStringLiteral("协议"),
        QStringLiteral("端口"),
        QStringLiteral("流数"),
        QStringLiteral("字节数")
    };
    m_topAppTable->setHorizontalHeaderLabels(taHeaders);
    m_topAppTable->horizontalHeader()->setStretchLastSection(true);
    m_topAppTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_topAppTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topAppTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_topAppTable->setAlternatingRowColors(true);
    m_topAppTable->verticalHeader()->setVisible(false);
    m_topAppTable->setSortingEnabled(false);
    m_topAppTable->setColumnWidth(0, 120);
    m_topAppTable->setColumnWidth(1, 80);
    m_topAppTable->setColumnWidth(2, 70);
    m_topAppTable->setColumnWidth(3, 70);
    m_topAppTable->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; font-size: 12px;"
        "  gridline-color: #30363D;"
        "}"
        "QTableWidget::item { padding: 3px 4px; }"
        "QTableWidget::item:selected { background-color: #30363D; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
    ));
    taLayout->addWidget(m_topAppTable);
    bottomSplitter->addWidget(taGroup);

    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 1);
    bottomSplitter->setSizes({400, 400});
    mainLayout->addWidget(bottomSplitter, 1);

    // ── 定时器 ──
    m_statsTimer = new QTimer(this);
}

// ─── Connections ────────────────────────────────────────────────────────────

void FlowCollectorWidget::setupConnections()
{
    connect(m_startStopBtn, &QPushButton::clicked, this, &FlowCollectorWidget::onStartStop);
    connect(m_exportBtn, &QPushButton::clicked, this, &FlowCollectorWidget::onExport);
    connect(m_statsTimer, &QTimer::timeout, this, [this]() {
        updateTopTalker();
        updateTopApp();
        updateStatusLabel();
    });
}

// ─── Start / Stop ───────────────────────────────────────────────────────────

void FlowCollectorWidget::onStartStop()
{
    if (m_running) {
        // Stop
        m_statsTimer->stop();
        if (m_udpSocket) {
            m_udpSocket->close();
        }
        m_running = false;
        m_startStopBtn->setText(QStringLiteral("启动"));
        m_startStopBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #58A6FF; color: white;"
            "  border: none; padding: 6px 18px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #79C0FF; }"
        ));
        m_portSpin->setEnabled(true);
        m_statusLabel->setText(QStringLiteral("已停止"));
        m_statusLabel->setStyleSheet("color: #888; font-weight: bold; font-size: 13px;");
        Logger::instance().info("FlowCollector", QStringLiteral("Flow 监听已停止"));
    } else {
        // Start
        int port = m_portSpin->value();

        if (!m_udpSocket) {
            m_udpSocket = new QUdpSocket(this);
            connect(m_udpSocket, &QUdpSocket::readyRead,
                    this, &FlowCollectorWidget::onProcessDatagram);
        }

        if (!m_udpSocket->bind(QHostAddress::Any, static_cast<quint16>(port))) {
            QMessageBox::critical(this, QStringLiteral("绑定失败"),
                QStringLiteral("无法绑定 UDP 端口 %1:\n%2")
                    .arg(port)
                    .arg(m_udpSocket->errorString()));
            return;
        }

        m_running = true;
        m_statsTimer->start(2000);
        m_startStopBtn->setText(QStringLiteral("停止"));
        m_startStopBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #F85149; color: white;"
            "  border: none; padding: 6px 18px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #FF7B72; }"
        ));
        m_portSpin->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("运行中"));
        m_statusLabel->setStyleSheet("color: #0a0; font-weight: bold; font-size: 13px;");
        Logger::instance().info("FlowCollector",
            QStringLiteral("Flow 监听已启动 — 端口 %1").arg(port));
    }
}

// ─── Status label update ────────────────────────────────────────────────────

void FlowCollectorWidget::updateStatusLabel()
{
    if (m_running) {
        m_statusLabel->setText(QStringLiteral("运行中 | 已接收包: %1 | 流数: %2")
            .arg(m_packetCount)
            .arg(m_flowCount));
    }
}

// ─── Process incoming datagram ──────────────────────────────────────────────

void FlowCollectorWidget::onProcessDatagram()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        m_packetCount++;

        if (datagram.size() < 24) {
            Logger::instance().warning("FlowCollector",
                QStringLiteral("数据报过小 (%1 字节)，跳过").arg(datagram.size()));
            continue;
        }

        QString sourceIP = sender.toString();

        // Check version number from first 2 bytes
        const quint8* buf = reinterpret_cast<const quint8*>(datagram.constData());
        quint16 version = (static_cast<quint16>(buf[0]) << 8) | buf[1];

        switch (version) {
            case 5:
                parseNetFlowV5(datagram, sourceIP);
                break;
            case 9:
                parseNetFlowV9(datagram, sourceIP);
                break;
            case 10:
                parseIPFIX(datagram, sourceIP);
                break;
            default:
                Logger::instance().warning("FlowCollector",
                    QStringLiteral("未知 Flow 版本: %1，来源: %2").arg(version).arg(sourceIP));
                break;
        }
    }
}

// ─── NetFlow v5 parsing ─────────────────────────────────────────────────────
// Header (24 bytes):
//   version (2), count (2), sys_uptime (4), unix_secs (4), unix_nsecs (4),
//   flow_sequence (4), engine_type (1), engine_id (1), sampling_interval (2)
// Flow record (48 bytes):
//   srcaddr (4), dstaddr (4), nexthop (4), input (2), output (2),
//   dPkts (4), dOctets (4), first (4), last (4),
//   srcport (2), dstport (2), pad1 (1), tcp_flags (1), prot (1), tos (1),
//   src_as (2), dst_as (2), src_mask (1), dst_mask (1), pad2 (2)

void FlowCollectorWidget::parseNetFlowV5(const QByteArray& data, const QString& sourceIP)
{
    if (data.size() < 24) return;

    const quint8* buf = reinterpret_cast<const quint8*>(data.constData());

    quint16 count = (static_cast<quint16>(buf[2]) << 8) | buf[3];
    quint32 unixSecs = (static_cast<quint32>(buf[8]) << 24) |
                       (static_cast<quint32>(buf[9]) << 16) |
                       (static_cast<quint32>(buf[10]) << 8) |
                       static_cast<quint32>(buf[11]);

    int expectedSize = 24 + 48 * count;
    if (data.size() < expectedSize) {
        Logger::instance().warning("FlowCollector",
            QStringLiteral("NetFlow v5 数据报不完整 (期望 %1, 实际 %2)")
                .arg(expectedSize).arg(data.size()));
        return;
    }

    Q_UNUSED(sourceIP);

    for (int i = 0; i < count; i++) {
        int offset = 24 + i * 48;

        FlowRec rec;
        quint32 srcAddr = (static_cast<quint32>(buf[offset + 0]) << 24) |
                          (static_cast<quint32>(buf[offset + 1]) << 16) |
                          (static_cast<quint32>(buf[offset + 2]) << 8) |
                          static_cast<quint32>(buf[offset + 3]);
        quint32 dstAddr = (static_cast<quint32>(buf[offset + 4]) << 24) |
                          (static_cast<quint32>(buf[offset + 5]) << 16) |
                          (static_cast<quint32>(buf[offset + 6]) << 8) |
                          static_cast<quint32>(buf[offset + 7]);

        rec.srcIP = ipToString(srcAddr);
        rec.dstIP = ipToString(dstAddr);
        rec.srcPort = (static_cast<quint16>(buf[offset + 32]) << 8) |
                      static_cast<quint16>(buf[offset + 33]);
        rec.dstPort = (static_cast<quint16>(buf[offset + 34]) << 8) |
                      static_cast<quint16>(buf[offset + 35]);
        rec.protocol = buf[offset + 38];

        rec.packets = (static_cast<quint32>(buf[offset + 16]) << 24) |
                      (static_cast<quint32>(buf[offset + 17]) << 16) |
                      (static_cast<quint32>(buf[offset + 18]) << 8) |
                      static_cast<quint32>(buf[offset + 19]);
        rec.bytes = (static_cast<quint32>(buf[offset + 20]) << 24) |
                    (static_cast<quint32>(buf[offset + 21]) << 16) |
                    (static_cast<quint32>(buf[offset + 22]) << 8) |
                    static_cast<quint32>(buf[offset + 23]);

        // NetFlow uptime: first/last relative to sysUptime
        quint32 first = (static_cast<quint32>(buf[offset + 24]) << 24) |
                        (static_cast<quint32>(buf[offset + 25]) << 16) |
                        (static_cast<quint32>(buf[offset + 26]) << 8) |
                        static_cast<quint32>(buf[offset + 27]);
        quint32 last = (static_cast<quint32>(buf[offset + 28]) << 24) |
                       (static_cast<quint32>(buf[offset + 29]) << 16) |
                       (static_cast<quint32>(buf[offset + 30]) << 8) |
                       static_cast<quint32>(buf[offset + 31]);

        // Convert uptime (milliseconds) to absolute time
        QDateTime baseTime = QDateTime::fromSecsSinceEpoch(unixSecs, QTimeZone::UTC);
        rec.startTime = baseTime.addMSecs(-static_cast<qint64>(first));
        rec.endTime = baseTime.addMSecs(-static_cast<qint64>(last));

        addFlowRec(rec);
    }
}

// ─── NetFlow v9 parsing (template-based, simplified) ────────────────────────

void FlowCollectorWidget::parseNetFlowV9(const QByteArray& data, const QString& sourceIP)
{
    // NetFlow v9 header: 20 bytes
    //   version (2), count (2), sys_uptime (4), unix_secs (4),
    //   package_sequence (4), source_id (4)
    if (data.size() < 20) return;

    const quint8* buf = reinterpret_cast<const quint8*>(data.constData());
    quint16 count = (static_cast<quint16>(buf[2]) << 8) | buf[3];
    quint32 unixSecs = (static_cast<quint32>(buf[8]) << 24) |
                       (static_cast<quint32>(buf[9]) << 16) |
                       (static_cast<quint32>(buf[10]) << 8) |
                       static_cast<quint32>(buf[11]);

    // In production, NetFlow v9 requires template management.
    // For this implementation, we log the data and store a summary record.
    Logger::instance().info("FlowCollector",
        QStringLiteral("NetFlow v9 数据报: %1 条, 来源: %2").arg(count).arg(sourceIP));

    // Simplified: store a placeholder record for each v9 flow
    int limit = qMin(static_cast<int>(count), 100);
    for (int i = 0; i < limit; i++) {
        FlowRec rec;
        rec.srcIP = sourceIP;
        rec.dstIP = QStringLiteral("0.0.0.0");
        rec.srcPort = 0;
        rec.dstPort = 0;
        rec.protocol = 0;
        rec.packets = 0;
        rec.bytes = 0;
        rec.startTime = QDateTime::fromSecsSinceEpoch(unixSecs, QTimeZone::UTC);
        rec.endTime  = rec.startTime;
        addFlowRec(rec);
    }
}

// ─── IPFIX parsing (RFC 7011, similar to NetFlow v9) ────────────────────────

void FlowCollectorWidget::parseIPFIX(const QByteArray& data, const QString& sourceIP)
{
    // IPFIX header: 16 bytes
    //   version (2), length (2), export_time (4), sequence_number (4),
    //   observation_domain_id (4)
    if (data.size() < 16) return;

    const quint8* buf = reinterpret_cast<const quint8*>(data.constData());
    quint16 length = (static_cast<quint16>(buf[2]) << 8) | buf[3];
    quint32 exportTime = (static_cast<quint32>(buf[4]) << 24) |
                         (static_cast<quint32>(buf[5]) << 16) |
                         (static_cast<quint32>(buf[6]) << 8) |
                         static_cast<quint32>(buf[7]);

    Logger::instance().info("FlowCollector",
        QStringLiteral("IPFIX 数据报: 长度 %1, 来源: %2").arg(length).arg(sourceIP));

    // Simplified: store a placeholder record
    FlowRec rec;
    rec.srcIP = sourceIP;
    rec.dstIP = QStringLiteral("0.0.0.0");
    rec.srcPort = 0;
    rec.dstPort = 0;
    rec.protocol = 0;
    rec.packets = 0;
    rec.bytes = 0;
    rec.startTime = QDateTime::fromSecsSinceEpoch(exportTime, QTimeZone::UTC);
    rec.endTime = rec.startTime;
    addFlowRec(rec);
}

// ─── Add flow record ────────────────────────────────────────────────────────

void FlowCollectorWidget::addFlowRec(const FlowRec& rec)
{
    m_records.append(rec);
    m_flowCount++;

    while (m_records.size() > MAX_RECORDS) {
        m_records.removeFirst();
    }
}

// ─── Load demo records (20 simulated flows) ─────────────────────────────────

void FlowCollectorWidget::loadDemoRecords()
{
    struct DemoData {
        QString srcIP;
        QString dstIP;
        quint16 srcPort;
        quint16 dstPort;
        quint8  proto;
        quint64 packets;
        quint64 bytes;
        qint64  secondsAgo;
    };

    DemoData demoData[] = {
        {"192.168.1.100", "10.0.0.50",    54321, 443,   6,  1240, 850000,   60},
        {"192.168.1.100", "10.0.0.50",    54322, 80,    6,  890,  450000,   55},
        {"192.168.1.101", "172.16.0.20",  49152, 22,    6,  3200, 1200000,  50},
        {"192.168.1.102", "8.8.8.8",      53001, 53,   17,  45,   3500,     45},
        {"192.168.1.103", "203.0.113.5",  51000, 3306,  6,  5600, 3200000,  40},
        {"192.168.1.100", "172.16.0.20",  54323, 443,   6,  2100, 1500000,  35},
        {"10.0.0.50",     "192.168.1.100",443,   54321, 6,  980,  720000,   30},
        {"192.168.1.104", "10.0.0.100",   50001, 8080,  6,  1500, 980000,   25},
        {"192.168.1.105", "8.8.4.4",      54000, 53,   17,  62,   5100,     20},
        {"192.168.1.101", "10.0.0.50",    49153, 443,   6,  780,  520000,   18},
        {"192.168.1.106", "172.16.0.30",  52000, 1433,  6,  4300, 2800000,  15},
        {"192.168.1.107", "203.0.113.10", 55000, 25,    6,  320,  180000,   12},
        {"10.0.0.100",    "192.168.1.104",8080,  50001, 6,  1200, 1050000,  10},
        {"192.168.1.108", "8.8.8.8",      56000, 443,   6,  670,  410000,   8},
        {"192.168.1.102", "10.0.0.200",   53002, 6379,  6,  8900, 5600000,  7},
        {"192.168.1.100", "203.0.113.5",  54324, 1521,  6,  3400, 2100000,  5},
        {"192.168.1.109", "172.16.0.20",  57000, 22,    6,  1800, 760000,   4},
        {"192.168.1.103", "10.0.0.50",    51001, 443,   6,  1100, 680000,   3},
        {"172.16.0.20",   "192.168.1.101",22,    49152, 6,  2900, 1350000,  2},
        {"192.168.1.110", "8.8.4.4",      58000, 53,   17,  38,   2800,     1},
    };

    QDateTime now = QDateTime::currentDateTime();
    for (const auto& d : demoData) {
        FlowRec rec;
        rec.srcIP = d.srcIP;
        rec.dstIP = d.dstIP;
        rec.srcPort = d.srcPort;
        rec.dstPort = d.dstPort;
        rec.protocol = d.proto;
        rec.packets = d.packets;
        rec.bytes = d.bytes;
        rec.startTime = now.addSecs(-d.secondsAgo - 30);
        rec.endTime = now.addSecs(-d.secondsAgo);
        addFlowRec(rec);
    }

    // Populate tables with demo data
    updateTopTalker();
    updateTopApp();
    updateStatusLabel();
}

// ─── Update Top Talker table ────────────────────────────────────────────────

void FlowCollectorWidget::updateTopTalker()
{
    // Aggregate by source IP
    QMap<QString, TopTalkerEntry> aggregated;
    quint64 totalBytes = 0;

    for (const FlowRec& rec : m_records) {
        totalBytes += rec.bytes;

        if (aggregated.contains(rec.srcIP)) {
            TopTalkerEntry& entry = aggregated[rec.srcIP];
            entry.bytes += rec.bytes;
            entry.flows += 1;
        } else {
            TopTalkerEntry entry;
            entry.srcIP = rec.srcIP;
            entry.bytes = rec.bytes;
            entry.flows = 1;
            entry.percentage = 0.0;
            aggregated[rec.srcIP] = entry;
        }
    }

    // Calculate percentages and sort
    QList<TopTalkerEntry> entries = aggregated.values();
    for (TopTalkerEntry& entry : entries) {
        entry.percentage = (totalBytes > 0) ? (100.0 * entry.bytes / totalBytes) : 0.0;
    }

    std::sort(entries.begin(), entries.end(),
              [](const TopTalkerEntry& a, const TopTalkerEntry& b) {
                  return a.bytes > b.bytes;
              });

    if (entries.size() > 50) {
        entries = entries.mid(0, 50);
    }

    // Populate top talker table
    m_topTalkerTable->setRowCount(0);
    for (const TopTalkerEntry& entry : entries) {
        int row = m_topTalkerTable->rowCount();
        m_topTalkerTable->insertRow(row);

        m_topTalkerTable->setItem(row, 0, new QTableWidgetItem(entry.srcIP));

        auto* bytesItem = new QTableWidgetItem();
        bytesItem->setData(Qt::DisplayRole, static_cast<qulonglong>(entry.bytes));
        m_topTalkerTable->setItem(row, 1, bytesItem);

        auto* flowsItem = new QTableWidgetItem();
        flowsItem->setData(Qt::DisplayRole, entry.flows);
        m_topTalkerTable->setItem(row, 2, flowsItem);

        auto* pctItem = new QTableWidgetItem(
            QStringLiteral("%1%").arg(entry.percentage, 0, 'f', 1));
        m_topTalkerTable->setItem(row, 3, pctItem);
    }

    // Update flow table
    int flowCount = m_records.size();
    int startIdx = qMax(0, flowCount - 200);
    m_flowTable->setRowCount(0);

    for (int i = startIdx; i < flowCount; i++) {
        const FlowRec& rec = m_records[i];
        int row = m_flowTable->rowCount();
        m_flowTable->insertRow(row);

        m_flowTable->setItem(row, 0, new QTableWidgetItem(rec.srcIP));
        m_flowTable->setItem(row, 1, new QTableWidgetItem(rec.dstIP));
        m_flowTable->setItem(row, 2, new QTableWidgetItem(QString::number(rec.srcPort)));
        m_flowTable->setItem(row, 3, new QTableWidgetItem(QString::number(rec.dstPort)));
        m_flowTable->setItem(row, 4, new QTableWidgetItem(protocolName(rec.protocol)));

        auto* pktsItem = new QTableWidgetItem();
        pktsItem->setData(Qt::DisplayRole, static_cast<qulonglong>(rec.packets));
        m_flowTable->setItem(row, 5, pktsItem);

        auto* bytesItem = new QTableWidgetItem();
        bytesItem->setData(Qt::DisplayRole, static_cast<qulonglong>(rec.bytes));
        m_flowTable->setItem(row, 6, bytesItem);

        m_flowTable->setItem(row, 7, new QTableWidgetItem(
            rec.startTime.toString("yyyy-MM-dd hh:mm:ss")));
        m_flowTable->setItem(row, 8, new QTableWidgetItem(
            rec.endTime.toString("yyyy-MM-dd hh:mm:ss")));
    }
}

// ─── Update Top Application table ───────────────────────────────────────────

void FlowCollectorWidget::updateTopApp()
{
    // Aggregate by (app, protocol, port)
    struct AppKey {
        QString app;
        QString protocol;
        quint16 port;
        bool operator<(const AppKey& other) const {
            if (app != other.app) return app < other.app;
            if (protocol != other.protocol) return protocol < other.protocol;
            return port < other.port;
        }
    };
    struct AppAgg {
        int flows;
        quint64 bytes;
    };

    QMap<AppKey, AppAgg> aggregated;

    for (const FlowRec& rec : m_records) {
        QString appName = detectApp(rec.dstPort, rec.protocol);
        AppKey key = {appName, protocolName(rec.protocol), rec.dstPort};

        if (aggregated.contains(key)) {
            AppAgg& agg = aggregated[key];
            agg.flows += 1;
            agg.bytes += rec.bytes;
        } else {
            aggregated[key] = {1, rec.bytes};
        }
    }

    // Convert to list and sort by bytes
    QList<TopAppEntry> entries;
    for (auto it = aggregated.begin(); it != aggregated.end(); ++it) {
        TopAppEntry entry;
        entry.app = it.key().app;
        entry.protocol = it.key().protocol;
        entry.port = it.key().port;
        entry.flows = it.value().flows;
        entry.bytes = it.value().bytes;
        entries.append(entry);
    }

    std::sort(entries.begin(), entries.end(),
              [](const TopAppEntry& a, const TopAppEntry& b) {
                  return a.bytes > b.bytes;
              });

    if (entries.size() > 50) {
        entries = entries.mid(0, 50);
    }

    // Populate top app table
    m_topAppTable->setRowCount(0);
    for (const TopAppEntry& entry : entries) {
        int row = m_topAppTable->rowCount();
        m_topAppTable->insertRow(row);

        m_topAppTable->setItem(row, 0, new QTableWidgetItem(entry.app));
        m_topAppTable->setItem(row, 1, new QTableWidgetItem(entry.protocol));
        m_topAppTable->setItem(row, 2, new QTableWidgetItem(QString::number(entry.port)));

        auto* flowsItem = new QTableWidgetItem();
        flowsItem->setData(Qt::DisplayRole, entry.flows);
        m_topAppTable->setItem(row, 3, flowsItem);

        auto* bytesItem = new QTableWidgetItem();
        bytesItem->setData(Qt::DisplayRole, static_cast<qulonglong>(entry.bytes));
        m_topAppTable->setItem(row, 4, bytesItem);
    }
}

// ─── Export CSV ─────────────────────────────────────────────────────────────

void FlowCollectorWidget::onExport()
{
    if (m_records.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("没有可导出的数据。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 Flow 记录"),
        QStringLiteral("flow_collector_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        QStringLiteral("CSV 文件 (*.csv)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法写入文件: ") + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF"); // BOM for Excel
    QTextStream out(&file);

    out << QStringLiteral("源 IP,目标 IP,源端口,目标端口,协议,包数,字节数,开始时间,结束时间\n");

    for (const FlowRec& rec : m_records) {
        out << rec.srcIP << ","
            << rec.dstIP << ","
            << rec.srcPort << ","
            << rec.dstPort << ","
            << protocolName(rec.protocol) << ","
            << rec.packets << ","
            << rec.bytes << ","
            << rec.startTime.toString("yyyy-MM-dd hh:mm:ss") << ","
            << rec.endTime.toString("yyyy-MM-dd hh:mm:ss") << "\n";
    }

    int count = m_records.size();
    file.close();

    Logger::instance().info("FlowCollector",
        QStringLiteral("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(count));

    QMessageBox::information(this, QStringLiteral("导出成功"),
        QStringLiteral("已导出 %1 条记录到:\n%2").arg(count).arg(filePath));
}