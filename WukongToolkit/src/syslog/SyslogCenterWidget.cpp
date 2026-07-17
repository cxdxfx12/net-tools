#include "syslog/SyslogCenterWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QSplitter>
#include <QRegularExpression>
#include <QHostAddress>
#include <QScrollBar>
#include <QApplication>

// ============================================================================
// Facility / Severity 名称映射
// ============================================================================
static const QStringList& facilityNames()
{
    static const QStringList names = {
        "kern",     // 0
        "user",     // 1
        "mail",     // 2
        "daemon",   // 3
        "auth",     // 4
        "syslog",   // 5
        "lpr",      // 6
        "news",     // 7
        "uucp",     // 8
        "cron",     // 9
        "authpriv", // 10
        "ftp",      // 11
        "ntp",      // 12
        "audit",    // 13
        "alert",    // 14
        "clock",    // 15
        "local0",   // 16
        "local1",   // 17
        "local2",   // 18
        "local3",   // 19
        "local4",   // 20
        "local5",   // 21
        "local6",   // 22
        "local7",   // 23
    };
    return names;
}

static const QStringList& severityNames()
{
    static const QStringList names = {
        "Emergency", // 0
        "Alert",     // 1
        "Critical",  // 2
        "Error",     // 3
        "Warning",   // 4
        "Notice",    // 5
        "Info",      // 6
        "Debug",     // 7
    };
    return names;
}

static int severityIndex(const QString& name)
{
    const auto& names = severityNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names[i] == name) return i;
    }
    return -1;
}

static int facilityIndex(const QString& name)
{
    const auto& names = facilityNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names[i] == name) return i;
    }
    return -1;
}

// ============================================================================
// SyslogCenterWidget 实现
// ============================================================================
SyslogCenterWidget::SyslogCenterWidget(QWidget* parent)
    : QWidget(parent)
    , m_portSpin(nullptr)
    , m_protocolCombo(nullptr)
    , m_startBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_severityFilter(nullptr)
    , m_facilityFilter(nullptr)
    , m_deviceFilter(nullptr)
    , m_searchEdit(nullptr)
    , m_applyFilterBtn(nullptr)
    , m_clearFilterBtn(nullptr)
    , m_logTable(nullptr)
    , m_logDetail(nullptr)
    , m_totalCountLabel(nullptr)
    , m_todayCountLabel(nullptr)
    , m_errorCountLabel(nullptr)
    , m_autoScrollCheck(nullptr)
    , m_exportBtn(nullptr)
    , m_clearBtn(nullptr)
    , m_udpSocket(nullptr)
    , m_tcpServer(nullptr)
    , m_running(false)
{
    setupUI();
    setupConnections();
    addDemoLogs();
}

SyslogCenterWidget::~SyslogCenterWidget()
{
    if (m_running) {
        if (m_udpSocket) {
            m_udpSocket->close();
        }
        if (m_tcpServer) {
            m_tcpServer->close();
            for (QTcpSocket* client : m_tcpClients) {
                client->disconnectFromHost();
                client->deleteLater();
            }
            m_tcpClients.clear();
            m_tcpBuffers.clear();
        }
    }
}

QString SyslogCenterWidget::facilityName(int code)
{
    const auto& names = facilityNames();
    if (code >= 0 && code < names.size()) {
        return names[code];
    }
    return QString::number(code);
}

QString SyslogCenterWidget::severityName(int severity)
{
    const auto& names = severityNames();
    if (severity >= 0 && severity < names.size()) {
        return names[severity];
    }
    return QString::number(severity);
}

// ============================================================================
// UI 构建
// ============================================================================
void SyslogCenterWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ─── 顶部服务器控制区 ───
    auto* serverGroup = new QGroupBox("Syslog 服务器");
    auto* serverLayout = new QHBoxLayout(serverGroup);

    serverLayout->addWidget(new QLabel("监听端口:"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(514);
    m_portSpin->setFixedWidth(90);
    serverLayout->addWidget(m_portSpin);

    serverLayout->addWidget(new QLabel("协议:"));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItem("UDP");
    m_protocolCombo->addItem("TCP");
    m_protocolCombo->setFixedWidth(80);
    serverLayout->addWidget(m_protocolCombo);

    m_startBtn = new QPushButton("启动");
    m_startBtn->setFixedWidth(70);
    m_startBtn->setStyleSheet(
        "QPushButton { background-color: #67C23A; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:pressed { background-color: #5DAF33; }"
    );
    serverLayout->addWidget(m_startBtn);

    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setFixedWidth(70);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet(
        "QPushButton:disabled { background-color: #4A4D52; color: #8C8C8C; }"
        "QPushButton:!disabled { background-color: #F56C6C; color: #FFFFFF; }"
        "QPushButton:!disabled:hover { background-color: #F78989; }"
        "QPushButton:!disabled:pressed { background-color: #DD6161; }"
    );
    serverLayout->addWidget(m_stopBtn);

    m_statusLabel = new QLabel("● 已停止");
    m_statusLabel->setStyleSheet("color: #888; font-weight: bold;");
    serverLayout->addWidget(m_statusLabel);

    serverLayout->addStretch();
    mainLayout->addWidget(serverGroup);

    // ─── 过滤区 ───
    auto* filterGroup = new QGroupBox("日志过滤");
    auto* filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("严重级别:"));
    m_severityFilter = new QComboBox();
    m_severityFilter->addItem("全部");
    m_severityFilter->addItem("Emergency");
    m_severityFilter->addItem("Alert");
    m_severityFilter->addItem("Critical");
    m_severityFilter->addItem("Error");
    m_severityFilter->addItem("Warning");
    m_severityFilter->addItem("Notice");
    m_severityFilter->addItem("Info");
    m_severityFilter->addItem("Debug");
    m_severityFilter->setFixedWidth(110);
    filterLayout->addWidget(m_severityFilter);

    filterLayout->addWidget(new QLabel("设施:"));
    m_facilityFilter = new QComboBox();
    m_facilityFilter->addItem("全部");
    m_facilityFilter->addItem("kern");
    m_facilityFilter->addItem("user");
    m_facilityFilter->addItem("mail");
    m_facilityFilter->addItem("daemon");
    m_facilityFilter->addItem("auth");
    m_facilityFilter->addItem("syslog");
    m_facilityFilter->addItem("lpr");
    m_facilityFilter->addItem("news");
    m_facilityFilter->addItem("uucp");
    m_facilityFilter->addItem("cron");
    m_facilityFilter->addItem("local0");
    m_facilityFilter->addItem("local1");
    m_facilityFilter->addItem("local2");
    m_facilityFilter->addItem("local3");
    m_facilityFilter->addItem("local4");
    m_facilityFilter->addItem("local5");
    m_facilityFilter->addItem("local6");
    m_facilityFilter->addItem("local7");
    m_facilityFilter->setFixedWidth(100);
    filterLayout->addWidget(m_facilityFilter);

    filterLayout->addWidget(new QLabel("设备:"));
    m_deviceFilter = new QComboBox();
    m_deviceFilter->addItem("全部");
    m_deviceFilter->setMinimumWidth(120);
    filterLayout->addWidget(m_deviceFilter);

    filterLayout->addWidget(new QLabel("关键字:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索消息内容...");
    m_searchEdit->setMinimumWidth(150);
    filterLayout->addWidget(m_searchEdit);

    m_applyFilterBtn = new QPushButton("应用过滤");
    filterLayout->addWidget(m_applyFilterBtn);

    m_clearFilterBtn = new QPushButton("清除过滤");
    filterLayout->addWidget(m_clearFilterBtn);

    filterLayout->addStretch();
    mainLayout->addWidget(filterGroup);

    // ─── 中间区域：日志表格 + 详情面板 ───
    auto* splitter = new QSplitter(Qt::Horizontal);

    // 左侧：日志表格
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_logTable = new QTableWidget();
    m_logTable->setColumnCount(5);
    QStringList headers = {"时间", "主机", "严重级别", "设施", "消息"};
    m_logTable->setHorizontalHeaderLabels(headers);
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->setSortingEnabled(false);

    m_logTable->setColumnWidth(0, 160); // 时间
    m_logTable->setColumnWidth(1, 130); // 主机
    m_logTable->setColumnWidth(2, 90);  // 严重级别
    m_logTable->setColumnWidth(3, 80);  // 设施
    m_logTable->setColumnWidth(4, 300); // 消息

    leftLayout->addWidget(m_logTable);

    // 右侧：日志详情
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_logDetail = new QPlainTextEdit();
    m_logDetail->setReadOnly(true);
    m_logDetail->setPlaceholderText("选择日志条目查看详情...");
    m_logDetail->setMaximumWidth(350);
    rightLayout->addWidget(m_logDetail);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // ─── 底部统计区 ───
    auto* statsGroup = new QGroupBox("统计信息");
    auto* statsLayout = new QHBoxLayout(statsGroup);

    m_totalCountLabel = new QLabel("总日志数: 0");
    m_totalCountLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    statsLayout->addWidget(m_totalCountLabel);

    statsLayout->addSpacing(20);
    m_todayCountLabel = new QLabel("今日日志数: 0");
    m_todayCountLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    statsLayout->addWidget(m_todayCountLabel);

    statsLayout->addSpacing(20);
    m_errorCountLabel = new QLabel("错误数: 0");
    m_errorCountLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #F56C6C;");
    statsLayout->addWidget(m_errorCountLabel);

    statsLayout->addStretch();

    m_autoScrollCheck = new QCheckBox("自动滚动");
    m_autoScrollCheck->setChecked(true);
    statsLayout->addWidget(m_autoScrollCheck);

    m_exportBtn = new QPushButton("导出 CSV");
    statsLayout->addWidget(m_exportBtn);

    m_clearBtn = new QPushButton("清空日志");
    m_clearBtn->setStyleSheet(
        "QPushButton { background-color: #F56C6C; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #F78989; }"
        "QPushButton:pressed { background-color: #DD6161; }"
    );
    statsLayout->addWidget(m_clearBtn);

    mainLayout->addWidget(statsGroup);
}

void SyslogCenterWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onStartServer);
    connect(m_stopBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onStopServer);
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onApplyFilter);
    connect(m_clearFilterBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onClearFilter);
    connect(m_exportBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onExport);
    connect(m_clearBtn, &QPushButton::clicked, this, &SyslogCenterWidget::onClearLogs);
    connect(m_logTable, &QTableWidget::itemSelectionChanged, this, &SyslogCenterWidget::onLogSelected);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SyslogCenterWidget::onApplyFilter);
}

// ============================================================================
// 演示日志
// ============================================================================
void SyslogCenterWidget::addDemoLogs()
{
    QDateTime now = QDateTime::currentDateTime();

    struct DemoLog {
        int severity;
        int facility;
        QString host;
        QString message;
    };

    QList<DemoLog> demos = {
        { 6, 1, "router-01",   "Interface GigabitEthernet0/0/1 is up, line protocol is up"},
        { 4, 0, "core-switch", "Link down on port Gi1/0/24 (MAC flap detected)"},
        { 3, 3, "db-server",   "Authentication failure for user 'admin' from 192.168.1.100"},
        { 6, 5, "firewall",    "Connection accepted: TCP 192.168.1.50:443 -> 10.0.0.1:80"},
        { 5, 2, "mail-server", "New mail received for user@example.com (queue ID: 7A8F2C)"},
        { 7, 1, "router-02",   "OSPF neighbor state change: 10.0.0.2 -> FULL"},
        { 2, 0, "core-switch", "CPU utilization exceeds 95% threshold (current: 97%)"},
        { 4, 4, "auth-server", "Failed password for root from 192.168.1.200 port 22 ssh2"},
        { 6, 9, "cron-daemon", "CRON[12345]: (root) CMD (run-parts /etc/cron.hourly)"},
        { 3, 3, "app-server",  "Disk I/O error on /dev/sda1: sector 1234567 unreadable"},
        { 1, 0, "core-router", "Kernel panic - not syncing: Fatal exception in interrupt"},
        { 5, 7, "printer-01",  "Printer paper jam detected in tray 2"},
        { 6, 21, "local-app",  "Backup completed successfully: 1024 files, 2.3 GB"},
        { 4, 16, "switch-03",  "Temperature sensor reading exceeded threshold: 75°C"},
        { 0, 0, "dc-router",   "System power supply failure detected on PSU2"},
    };

    for (const auto& demo : demos) {
        QString sev = severityName(demo.severity);
        QString fac = facilityName(demo.facility);
        QDateTime logTime = now.addSecs(-(demos.size() - m_logEntries.size()) * 180);

        LogEntry entry;
        entry.time = logTime;
        entry.host = demo.host;
        entry.severity = sev;
        entry.facility = fac;
        entry.message = demo.message;
        m_logEntries.append(entry);

        if (!m_deviceList.contains(demo.host)) {
            m_deviceList.append(demo.host);
        }
    }

    // 更新设备过滤下拉框
    m_deviceFilter->clear();
    m_deviceFilter->addItem("全部");
    for (const QString& dev : m_deviceList) {
        m_deviceFilter->addItem(dev);
    }

    // 刷新表格
    onApplyFilter();
    updateStatistics();
}

// ============================================================================
// 启动 / 停止服务器
// ============================================================================
void SyslogCenterWidget::onStartServer()
{
    if (m_running) return;

    int port = m_portSpin->value();
    QString protocol = m_protocolCombo->currentText();

    if (protocol == "UDP") {
        if (!m_udpSocket) {
            m_udpSocket = new QUdpSocket(this);
            connect(m_udpSocket, &QUdpSocket::readyRead, this, &SyslogCenterWidget::onUdpReadyRead);
        }

        if (!m_udpSocket->bind(QHostAddress::Any, static_cast<quint16>(port))) {
            QMessageBox::critical(this, "绑定失败",
                QString("无法绑定 UDP 端口 %1:\n%2").arg(port).arg(m_udpSocket->errorString()));
            return;
        }
    } else {
        // TCP
        if (!m_tcpServer) {
            m_tcpServer = new QTcpServer(this);
            connect(m_tcpServer, &QTcpServer::newConnection, this, &SyslogCenterWidget::onTcpNewConnection);
        }

        if (!m_tcpServer->listen(QHostAddress::Any, static_cast<quint16>(port))) {
            QMessageBox::critical(this, "绑定失败",
                QString("无法绑定 TCP 端口 %1:\n%2").arg(port).arg(m_tcpServer->errorString()));
            return;
        }
    }

    m_running = true;
    updateStatus(true);
    Logger::instance().info("SyslogCenter",
        QString("Syslog 服务器已启动 — %1 端口 %2").arg(protocol).arg(port));
}

void SyslogCenterWidget::onStopServer()
{
    if (!m_running) return;

    if (m_udpSocket) {
        m_udpSocket->close();
    }
    if (m_tcpServer) {
        m_tcpServer->close();
        for (QTcpSocket* client : m_tcpClients) {
            client->disconnectFromHost();
            client->deleteLater();
        }
        m_tcpClients.clear();
        m_tcpBuffers.clear();
    }

    m_running = false;
    updateStatus(false);
    Logger::instance().info("SyslogCenter", "Syslog 服务器已停止");
}

void SyslogCenterWidget::updateStatus(bool running)
{
    if (running) {
        m_statusLabel->setText("● 运行中");
        m_statusLabel->setStyleSheet("color: #67C23A; font-weight: bold;");
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
        m_portSpin->setEnabled(false);
        m_protocolCombo->setEnabled(false);
    } else {
        m_statusLabel->setText("● 已停止");
        m_statusLabel->setStyleSheet("color: #888; font-weight: bold;");
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_portSpin->setEnabled(true);
        m_protocolCombo->setEnabled(true);
    }
}

// ============================================================================
// UDP 接收
// ============================================================================
void SyslogCenterWidget::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString raw = QString::fromUtf8(datagram).trimmed();
        if (raw.isEmpty()) continue;

        processLogMessage(raw, sender.toString());
    }
}

// ============================================================================
// TCP 接收
// ============================================================================
void SyslogCenterWidget::onTcpNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* client = m_tcpServer->nextPendingConnection();
        m_tcpClients.append(client);
        m_tcpBuffers.insert(client, QByteArray());

        connect(client, &QTcpSocket::readyRead, this, &SyslogCenterWidget::onTcpReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &SyslogCenterWidget::onTcpDisconnected);

        Logger::instance().info("SyslogCenter",
            QString("TCP 客户端已连接: %1").arg(client->peerAddress().toString()));
    }
}

void SyslogCenterWidget::onTcpReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    m_tcpBuffers[client].append(client->readAll());

    QByteArray& buffer = m_tcpBuffers[client];
    while (true) {
        int idx = buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = buffer.left(idx).trimmed();
        buffer.remove(0, idx + 1);

        if (line.isEmpty()) continue;
        if (line.endsWith('\r')) line.chop(1);

        QString raw = QString::fromUtf8(line).trimmed();
        if (raw.isEmpty()) continue;

        processLogMessage(raw, client->peerAddress().toString());
    }
}

void SyslogCenterWidget::onTcpDisconnected()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    Logger::instance().info("SyslogCenter",
        QString("TCP 客户端已断开: %1").arg(client->peerAddress().toString()));

    m_tcpClients.removeAll(client);
    m_tcpBuffers.remove(client);
    client->deleteLater();
}

// ============================================================================
// 日志消息处理
// ============================================================================
void SyslogCenterWidget::processLogMessage(const QString& rawMessage, const QString& sourceHost)
{
    QString timestamp, host, facility, severity, message;

    // 先尝试 RFC5424，再尝试 RFC3164
    parseRFC5424(rawMessage, timestamp, host, facility, severity, message);
    if (timestamp.isEmpty() && host.isEmpty() && message.isEmpty()) {
        parseRFC3164(rawMessage, timestamp, host, facility, severity, message);
    }

    // 如果都无法解析，作为原始消息处理
    if (timestamp.isEmpty()) {
        timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    }
    if (host.isEmpty()) {
        host = sourceHost;
    }
    if (message.isEmpty()) {
        message = rawMessage;
    }
    if (severity.isEmpty()) {
        severity = "Info";
    }
    if (facility.isEmpty()) {
        facility = "user";
    }

    // 检查是否匹配过滤条件
    if (!matchesFilter(severity, facility, host, message)) {
        return;
    }

    // 存储
    {
        QMutexLocker lock(&m_mutex);
        LogEntry entry;
        entry.time = QDateTime::currentDateTime();
        entry.host = host;
        entry.severity = severity;
        entry.facility = facility;
        entry.message = message;
        m_logEntries.append(entry);

        // 更新设备列表
        if (!m_deviceList.contains(host)) {
            m_deviceList.append(host);
            // 在主线程更新 UI
            QMetaObject::invokeMethod(this, [this, host]() {
                m_deviceFilter->addItem(host);
            }, Qt::QueuedConnection);
        }

        // 裁剪旧数据
        while (m_logEntries.size() > MAX_ENTRIES) {
            m_logEntries.removeFirst();
        }
    }

    // 添加到表格
    QMetaObject::invokeMethod(this, [this, timestamp, host, severity, facility, message]() {
        addLogEntry(timestamp, host, severity, facility, message);
        updateStatistics();
    }, Qt::QueuedConnection);
}

// ============================================================================
// RFC 3164 解析
// 格式: <PRI>MMM DD HH:MM:SS HOST MSG
// PRI = Facility * 8 + Severity
// ============================================================================
void SyslogCenterWidget::parseRFC3164(const QString& msg, QString& timestamp,
                                       QString& host, QString& facility,
                                       QString& severity, QString& message)
{
    QString remaining = msg;

    // 解析 <PRI>
    if (remaining.startsWith('<')) {
        int end = remaining.indexOf('>');
        if (end > 1) {
            bool ok = false;
            int pri = remaining.mid(1, end - 1).toInt(&ok);
            if (ok) {
                int fac = pri / 8;
                int sev = pri % 8;
                facility = facilityName(fac);
                severity = severityName(sev);
            }
            remaining = remaining.mid(end + 1).trimmed();
        }
    }

    // 解析 TIMESTAMP: Mmm dd hh:mm:ss
    static const QRegularExpression tsRegex(
        R"(^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s{1,2}(\d{1,2})\s(\d{2}):(\d{2}):(\d{2}))"
    );

    QRegularExpressionMatch match = tsRegex.match(remaining);
    if (match.hasMatch()) {
        static const QStringList months = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        int month = months.indexOf(match.captured(1)) + 1;
        int day = match.captured(2).toInt();
        int hour = match.captured(3).toInt();
        int min = match.captured(4).toInt();
        int sec = match.captured(5).toInt();

        QDate now = QDate::currentDate();
        QDateTime parsed(QDate(now.year(), month, day), QTime(hour, min, sec));
        if (parsed.isValid()) {
            timestamp = parsed.toString("yyyy-MM-dd hh:mm:ss");
        }

        remaining = remaining.mid(match.capturedLength()).trimmed();
    }

    // 解析 HOSTNAME + MESSAGE
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            host = remaining.left(spaceIdx);
            message = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            host = remaining;
        }
    }
}

// ============================================================================
// RFC 5424 解析
// 格式: <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID STRUCTURED-DATA MSG
// ============================================================================
void SyslogCenterWidget::parseRFC5424(const QString& msg, QString& timestamp,
                                       QString& host, QString& facility,
                                       QString& severity, QString& message)
{
    QString remaining = msg;

    // 解析 <PRI>
    if (remaining.startsWith('<')) {
        int end = remaining.indexOf('>');
        if (end > 1) {
            bool ok = false;
            int pri = remaining.mid(1, end - 1).toInt(&ok);
            if (ok) {
                int fac = pri / 8;
                int sev = pri % 8;
                facility = facilityName(fac);
                severity = severityName(sev);
            }
            remaining = remaining.mid(end + 1);
        }
    }

    // 解析 VERSION (必须跟在 PRI 后面，以空格分隔)
    if (remaining.startsWith('1')) {
        // 跳过版本号和空格
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            remaining = remaining.mid(spaceIdx + 1);
        } else {
            return; // 不符合 RFC 5424 格式
        }
    } else {
        return; // 不是 RFC 5424 格式
    }

    // 解析 TIMESTAMP (RFC 3339 / ISO 8601 格式)
    // 格式: 2024-01-15T14:30:45.123Z 或 2024-01-15T14:30:45+08:00
    static const QRegularExpression ts5424Regex(
        R"(^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})?))"
    );

    QRegularExpressionMatch tsMatch = ts5424Regex.match(remaining);
    if (tsMatch.hasMatch()) {
        QString ts = tsMatch.captured(1);
        // 转换为可读格式
        QDateTime dt = QDateTime::fromString(ts, Qt::ISODate);
        if (dt.isValid()) {
            timestamp = dt.toString("yyyy-MM-dd hh:mm:ss");
        } else {
            // 尝试去掉毫秒和时区
            dt = QDateTime::fromString(ts.left(19), "yyyy-MM-ddThh:mm:ss");
            if (dt.isValid()) {
                timestamp = dt.toString("yyyy-MM-dd hh:mm:ss");
            }
        }
        remaining = remaining.mid(tsMatch.capturedLength()).trimmed();
    }

    // 解析 HOSTNAME
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            host = remaining.left(spaceIdx);
            remaining = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            host = remaining;
            return;
        }
    }

    // 解析 APP-NAME (可选)
    QString appName;
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            appName = remaining.left(spaceIdx);
            remaining = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            appName = remaining;
            remaining.clear();
        }
    }

    // 解析 PROCID (可选)
    QString procId;
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            procId = remaining.left(spaceIdx);
            remaining = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            procId = remaining;
            remaining.clear();
        }
    }

    // 解析 MSGID (可选)
    QString msgId;
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            msgId = remaining.left(spaceIdx);
            remaining = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            msgId = remaining;
            remaining.clear();
        }
    }

    // 解析 STRUCTURED-DATA (可选, 以 [ 开头)
    if (!remaining.isEmpty() && remaining.startsWith('[')) {
        int endBracket = remaining.indexOf(']');
        if (endBracket >= 0) {
            // 跳过 structured data
            remaining = remaining.mid(endBracket + 1).trimmed();
        }
    }

    // 剩余部分为 MSG
    if (!remaining.isEmpty()) {
        // 如果有 BOM, 跳过
        if (remaining.startsWith("\xEF\xBB\xBF")) {
            remaining = remaining.mid(3);
        }
        message = remaining.trimmed();
    }

    // 如果没有解析到消息，使用 app-name 作为部分消息
    if (message.isEmpty() && !appName.isEmpty()) {
        message = appName;
        if (!procId.isEmpty() && procId != "-") {
            message += "[" + procId + "]";
        }
    }
}

// ============================================================================
// 过滤匹配
// ============================================================================
bool SyslogCenterWidget::matchesFilter(const QString& severity, const QString& facility,
                                        const QString& host, const QString& message) const
{
    // 严重级别过滤
    QString sevFilter = m_severityFilter->currentText();
    if (sevFilter != "全部" && sevFilter != severity) {
        return false;
    }

    // 设施过滤
    QString facFilter = m_facilityFilter->currentText();
    if (facFilter != "全部" && facFilter != facility) {
        return false;
    }

    // 设备过滤
    QString devFilter = m_deviceFilter->currentText();
    if (devFilter != "全部" && devFilter != host) {
        return false;
    }

    // 关键字搜索
    QString searchText = m_searchEdit->text().trimmed();
    if (!searchText.isEmpty()) {
        if (!message.contains(searchText, Qt::CaseInsensitive) &&
            !host.contains(searchText, Qt::CaseInsensitive) &&
            !severity.contains(searchText, Qt::CaseInsensitive) &&
            !facility.contains(searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// 添加日志条目到表格
// ============================================================================
void SyslogCenterWidget::addLogEntry(const QString& time, const QString& host,
                                      const QString& severity, const QString& facility,
                                      const QString& message)
{
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);

    QColor color = severityToColor(severity);

    auto* timeItem = new QTableWidgetItem(time);
    timeItem->setForeground(color);
    m_logTable->setItem(row, 0, timeItem);

    auto* hostItem = new QTableWidgetItem(host);
    m_logTable->setItem(row, 1, hostItem);

    auto* sevItem = new QTableWidgetItem(severity);
    sevItem->setForeground(color);
    sevItem->setTextAlignment(Qt::AlignCenter);
    sevItem->setFont(QFont(sevItem->font().family(), -1, QFont::Bold));
    m_logTable->setItem(row, 2, sevItem);

    auto* facItem = new QTableWidgetItem(facility);
    m_logTable->setItem(row, 3, facItem);

    auto* msgItem = new QTableWidgetItem(message);
    msgItem->setForeground(color);
    m_logTable->setItem(row, 4, msgItem);

    // 自动滚动
    if (m_autoScrollCheck->isChecked()) {
        m_logTable->scrollToBottom();
    }
}

// ============================================================================
// 完全刷新表格（应用过滤时使用）
// ============================================================================
void SyslogCenterWidget::onApplyFilter()
{
    m_logTable->setRowCount(0);

    QMutexLocker lock(&m_mutex);
    for (const LogEntry& entry : m_logEntries) {
        if (matchesFilter(entry.severity, entry.facility, entry.host, entry.message)) {
            addLogEntry(entry.time.toString("yyyy-MM-dd hh:mm:ss"),
                        entry.host, entry.severity, entry.facility, entry.message);
        }
    }

    updateStatistics();
}

void SyslogCenterWidget::onClearFilter()
{
    m_severityFilter->setCurrentIndex(0);
    m_facilityFilter->setCurrentIndex(0);
    m_deviceFilter->setCurrentIndex(0);
    m_searchEdit->clear();
    onApplyFilter();
}

// ============================================================================
// 选择日志显示详情
// ============================================================================
void SyslogCenterWidget::onLogSelected()
{
    QList<QTableWidgetItem*> selected = m_logTable->selectedItems();
    if (selected.isEmpty()) {
        m_logDetail->clear();
        return;
    }

    int row = selected.first()->row();

    QString time = m_logTable->item(row, 0)->text();
    QString host = m_logTable->item(row, 1)->text();
    QString severity = m_logTable->item(row, 2)->text();
    QString facility = m_logTable->item(row, 3)->text();
    QString message = m_logTable->item(row, 4)->text();

    QString detail;
    detail += "══════════════════════════════\n";
    detail += "  时间: " + time + "\n";
    detail += "  主机: " + host + "\n";
    detail += "  严重级别: " + severity + "\n";
    detail += "  设施: " + facility + "\n";
    detail += "══════════════════════════════\n\n";
    detail += "消息内容:\n";
    detail += message + "\n";

    m_logDetail->setPlainText(detail);
}

// ============================================================================
// 统计信息
// ============================================================================
void SyslogCenterWidget::updateStatistics()
{
    QMutexLocker lock(&m_mutex);

    int total = m_logEntries.size();
    int today = 0;
    int errors = 0;

    QDate todayDate = QDate::currentDate();
    for (const LogEntry& entry : m_logEntries) {
        if (entry.time.date() == todayDate) {
            today++;
        }
        int sevIdx = severityIndex(entry.severity);
        if (sevIdx >= 0 && sevIdx <= 3) {
            // Emergency, Alert, Critical, Error
            errors++;
        }
    }

    m_totalCountLabel->setText(QString("总日志数: %1").arg(total));
    m_todayCountLabel->setText(QString("今日日志数: %1").arg(today));
    m_errorCountLabel->setText(QString("错误数: %1").arg(errors));
}

// ============================================================================
// 严重级别颜色映射
// ============================================================================
QString SyslogCenterWidget::severityToColor(const QString& severity) const
{
    int idx = severityIndex(severity);
    switch (idx) {
        case 0: return "#FF0000"; // Emergency — 红色
        case 1: return "#8B0000"; // Alert — 深红
        case 2: return "#FF8C00"; // Critical — 橙色
        case 3: return "#FF4444"; // Error — 红色
        case 4: return "#FFD700"; // Warning — 黄色
        case 5: return "#6495ED"; // Notice — 蓝色
        case 6: return "#DCDCDC"; // Info — 白色
        case 7: return "#808080"; // Debug — 灰色
        default: return "#DCDCDC";
    }
}

// ============================================================================
// 导出 CSV
// ============================================================================
void SyslogCenterWidget::onExport()
{
    QMutexLocker lock(&m_mutex);
    if (m_logEntries.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV",
        QString("syslog_export_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    // BOM for Excel UTF-8
    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "时间,主机,严重级别,设施,消息\n";

    auto csvEscape = [](const QString& val) -> QString {
        QString v = val;
        if (v.contains(',') || v.contains('\n') || v.contains('"')) {
            v.replace("\"", "\"\"");
            v = "\"" + v + "\"";
        }
        return v;
    };

    for (const LogEntry& entry : m_logEntries) {
        out << csvEscape(entry.time.toString("yyyy-MM-dd hh:mm:ss")) << ","
            << csvEscape(entry.host) << ","
            << csvEscape(entry.severity) << ","
            << csvEscape(entry.facility) << ","
            << csvEscape(entry.message) << "\n";
    }

    file.close();
    Logger::instance().info("SyslogCenter",
        QString("日志已导出到: %1 (%2 条记录)").arg(filePath).arg(m_logEntries.size()));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(m_logEntries.size()).arg(filePath));
}

// ============================================================================
// 清空日志
// ============================================================================
void SyslogCenterWidget::onClearLogs()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认清空",
        "确定要清空所有日志记录吗？此操作不可撤销。",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    {
        QMutexLocker lock(&m_mutex);
        m_logEntries.clear();
        m_deviceList.clear();
    }

    m_logTable->setRowCount(0);
    m_logDetail->clear();
    m_deviceFilter->clear();
    m_deviceFilter->addItem("全部");

    updateStatistics();
    Logger::instance().info("SyslogCenter", "日志已清空");
}