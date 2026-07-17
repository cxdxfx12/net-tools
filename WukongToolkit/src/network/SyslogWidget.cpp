#include "network/SyslogWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QMenu>
#include <QRegularExpression>
#include <QHostAddress>

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

// ============================================================================
// SyslogWidget 实现
// ============================================================================
SyslogWidget::SyslogWidget(QWidget* parent)
    : QWidget(parent)
    , m_portSpin(nullptr)
    , m_protocolCombo(nullptr)
    , m_startStopBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_countLabel(nullptr)
    , m_facilityFilter(nullptr)
    , m_severityFilter(nullptr)
    , m_searchEdit(nullptr)
    , m_autoScrollCheck(nullptr)
    , m_clearBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_messageTable(nullptr)
    , m_udpSocket(nullptr)
    , m_tcpServer(nullptr)
    , m_running(false)
{
    setupUI();
    setupConnections();
}

SyslogWidget::~SyslogWidget()
{
    if (m_running) {
        // 停止服务器
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
        }
    }
}

QString SyslogWidget::facilityName(int facility)
{
    const auto& names = facilityNames();
    if (facility >= 0 && facility < names.size()) {
        return names[facility];
    }
    return QString::number(facility);
}

QString SyslogWidget::severityName(int severity)
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
void SyslogWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // --- 服务器配置 ---
    auto* configGroup = new QGroupBox("服务器配置");
    auto* configLayout = new QHBoxLayout(configGroup);

    configLayout->addWidget(new QLabel("监听端口:"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(514);
    configLayout->addWidget(m_portSpin);

    configLayout->addWidget(new QLabel("协议:"));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItem("UDP");
    m_protocolCombo->addItem("TCP");
    configLayout->addWidget(m_protocolCombo);

    m_startStopBtn = new QPushButton("启动");
    m_startStopBtn->setMinimumWidth(80);
    configLayout->addWidget(m_startStopBtn);

    m_statusLabel = new QLabel("● 已停止");
    m_statusLabel->setStyleSheet("color: #888; font-weight: bold;");
    configLayout->addWidget(m_statusLabel);

    m_countLabel = new QLabel("消息数: 0");
    configLayout->addWidget(m_countLabel);

    configLayout->addStretch();
    mainLayout->addWidget(configGroup);

    // --- 过滤器 ---
    auto* filterGroup = new QGroupBox("过滤");
    auto* filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("Facility:"));
    m_facilityFilter = new QComboBox();
    m_facilityFilter->addItem("ALL");
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
    filterLayout->addWidget(m_facilityFilter);

    filterLayout->addWidget(new QLabel("Severity:"));
    m_severityFilter = new QComboBox();
    m_severityFilter->addItem("ALL");
    m_severityFilter->addItem("Emergency");
    m_severityFilter->addItem("Alert");
    m_severityFilter->addItem("Critical");
    m_severityFilter->addItem("Error");
    m_severityFilter->addItem("Warning");
    m_severityFilter->addItem("Notice");
    m_severityFilter->addItem("Info");
    m_severityFilter->addItem("Debug");
    filterLayout->addWidget(m_severityFilter);

    filterLayout->addWidget(new QLabel("搜索:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索消息文本...");
    m_searchEdit->setMinimumWidth(150);
    filterLayout->addWidget(m_searchEdit);

    m_autoScrollCheck = new QCheckBox("自动滚动");
    m_autoScrollCheck->setChecked(true);
    filterLayout->addWidget(m_autoScrollCheck);

    m_clearBtn = new QPushButton("清空");
    filterLayout->addWidget(m_clearBtn);

    m_exportBtn = new QPushButton("导出 CSV");
    filterLayout->addWidget(m_exportBtn);

    filterLayout->addStretch();
    mainLayout->addWidget(filterGroup);

    // --- 消息表 ---
    m_messageTable = new QTableWidget();
    m_messageTable->setColumnCount(6);
    QStringList headers = {"时间", "来源 IP", "Facility", "Severity", "主机名", "消息"};
    m_messageTable->setHorizontalHeaderLabels(headers);
    m_messageTable->horizontalHeader()->setStretchLastSection(true);
    m_messageTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_messageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_messageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_messageTable->setAlternatingRowColors(true);
    m_messageTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_messageTable->verticalHeader()->setVisible(false);
    m_messageTable->setSortingEnabled(false);

    m_messageTable->setColumnWidth(0, 160); // 时间
    m_messageTable->setColumnWidth(1, 130); // 来源 IP
    m_messageTable->setColumnWidth(2, 80);  // Facility
    m_messageTable->setColumnWidth(3, 80);  // Severity
    m_messageTable->setColumnWidth(4, 120); // 主机名
    m_messageTable->setColumnWidth(5, 350); // 消息

    mainLayout->addWidget(m_messageTable);
}

void SyslogWidget::setupConnections()
{
    connect(m_startStopBtn, &QPushButton::clicked, this, &SyslogWidget::onStartStop);
    connect(m_clearBtn, &QPushButton::clicked, this, &SyslogWidget::onClear);
    connect(m_exportBtn, &QPushButton::clicked, this, &SyslogWidget::onExportCSV);
    connect(m_facilityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SyslogWidget::onFilterChanged);
    connect(m_severityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SyslogWidget::onFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SyslogWidget::onFilterChanged);
}

// ============================================================================
// 启动 / 停止
// ============================================================================
void SyslogWidget::onStartStop()
{
    if (m_running) {
        // 停止
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
        Logger::instance().info("Syslog", "服务器已停止");
    } else {
        // 启动
        int port = m_portSpin->value();
        QString protocol = m_protocolCombo->currentText();

        if (protocol == "UDP") {
            if (!m_udpSocket) {
                m_udpSocket = new QUdpSocket(this);
                connect(m_udpSocket, &QUdpSocket::readyRead, this, &SyslogWidget::onUdpReadyRead);
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
                connect(m_tcpServer, &QTcpServer::newConnection, this, &SyslogWidget::onTcpNewConnection);
            }

            if (!m_tcpServer->listen(QHostAddress::Any, static_cast<quint16>(port))) {
                QMessageBox::critical(this, "绑定失败",
                    QString("无法绑定 TCP 端口 %1:\n%2").arg(port).arg(m_tcpServer->errorString()));
                return;
            }
        }

        m_running = true;
        updateStatus(true);
        Logger::instance().info("Syslog",
            QString("服务器已启动 — %1 端口 %2").arg(protocol).arg(port));
    }
}

void SyslogWidget::updateStatus(bool running)
{
    if (running) {
        m_statusLabel->setText("● 运行中");
        m_statusLabel->setStyleSheet("color: #0a0; font-weight: bold;");
        m_startStopBtn->setText("停止");
        m_portSpin->setEnabled(false);
        m_protocolCombo->setEnabled(false);
    } else {
        m_statusLabel->setText("● 已停止");
        m_statusLabel->setStyleSheet("color: #888; font-weight: bold;");
        m_startStopBtn->setText("启动");
        m_portSpin->setEnabled(true);
        m_protocolCombo->setEnabled(true);
    }
}

// ============================================================================
// UDP 接收
// ============================================================================
void SyslogWidget::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString raw = QString::fromUtf8(datagram).trimmed();
        if (raw.isEmpty()) continue;

        SyslogMessage msg;
        parseSyslog(raw, sender.toString(), msg);

        {
            QMutexLocker lock(&m_mutex);
            m_messages.append(msg);
            cleanupOldMessages();
        }

        addMessageRow(msg);
        m_countLabel->setText(QString("消息数: %1").arg(m_messages.size()));
    }
}

// ============================================================================
// TCP 接收
// ============================================================================
void SyslogWidget::onTcpNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* client = m_tcpServer->nextPendingConnection();
        m_tcpClients.append(client);
        m_tcpBuffers.insert(client, QByteArray());

        connect(client, &QTcpSocket::readyRead, this, &SyslogWidget::onTcpReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &SyslogWidget::onTcpDisconnected);

        Logger::instance().info("Syslog",
            QString("TCP 客户端已连接: %1").arg(client->peerAddress().toString()));
    }
}

void SyslogWidget::onTcpReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    m_tcpBuffers[client].append(client->readAll());

    // 按换行分割消息
    QByteArray& buffer = m_tcpBuffers[client];
    while (true) {
        int idx = buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = buffer.left(idx).trimmed();
        buffer.remove(0, idx + 1);

        if (line.isEmpty()) continue;
        // 去除 \r
        if (line.endsWith('\r')) line.chop(1);

        QString raw = QString::fromUtf8(line).trimmed();
        if (raw.isEmpty()) continue;

        SyslogMessage msg;
        parseSyslog(raw, client->peerAddress().toString(), msg);

        {
            QMutexLocker lock(&m_mutex);
            m_messages.append(msg);
            cleanupOldMessages();
        }

        addMessageRow(msg);
        m_countLabel->setText(QString("消息数: %1").arg(m_messages.size()));
    }
}

void SyslogWidget::onTcpDisconnected()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    Logger::instance().info("Syslog",
        QString("TCP 客户端已断开: %1").arg(client->peerAddress().toString()));

    m_tcpClients.removeAll(client);
    m_tcpBuffers.remove(client);
    client->deleteLater();
}

// ============================================================================
// RFC 3164 解析
// 格式: <PRI>TIMESTAMP HOSTNAME MSG
// PRI = Facility * 8 + Severity
// ============================================================================
void SyslogWidget::parseSyslog(const QString& raw, const QString& sourceIP, SyslogMessage& outMsg)
{
    outMsg.rawData = raw;
    outMsg.sourceIP = sourceIP;
    outMsg.time = QDateTime::currentDateTime(); // 默认使用接收时间
    outMsg.facility = 0;
    outMsg.severity = 0;

    QString remaining = raw;

    // 解析 <PRI>
    if (remaining.startsWith('<')) {
        int end = remaining.indexOf('>');
        if (end > 1) {
            bool ok = false;
            int pri = remaining.mid(1, end - 1).toInt(&ok);
            if (ok) {
                outMsg.facility = pri / 8;
                outMsg.severity = pri % 8;
            }
            remaining = remaining.mid(end + 1).trimmed();
        }
    }

    // 尝试解析 TIMESTAMP (RFC 3164 格式: Mmm dd hh:mm:ss)
    // 例如: "Jan  1 00:00:00" 或 "Oct 15 14:30:45"
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
            outMsg.time = parsed;
        }

        remaining = remaining.mid(match.capturedLength()).trimmed();
    }

    // 解析 HOSTNAME (下一个空格前的 token)
    if (!remaining.isEmpty()) {
        int spaceIdx = remaining.indexOf(' ');
        if (spaceIdx > 0) {
            outMsg.hostname = remaining.left(spaceIdx);
            outMsg.message = remaining.mid(spaceIdx + 1).trimmed();
        } else {
            outMsg.hostname = remaining;
        }
    }
}

// ============================================================================
// 颜色映射
// ============================================================================
QColor SyslogWidget::severityColor(int severity) const
{
    switch (severity) {
        case 0: return QColor(255, 0, 0);       // Emergency — 红色
        case 1: return QColor(255, 50, 0);       // Alert — 深橙红
        case 2: return QColor(200, 30, 0);       // Critical — 深红
        case 3: return QColor(255, 140, 0);      // Error — 橙色
        case 4: return QColor(200, 180, 0);      // Warning — 黄色
        case 5: return QColor(80, 140, 200);     // Notice — 蓝色
        case 6: return QColor(0, 0, 0);          // Info — 默认(黑色)
        case 7: return QColor(120, 120, 120);    // Debug — 灰色
        default: return QColor(0, 0, 0);
    }
}

// ============================================================================
// 消息表操作
// ============================================================================
void SyslogWidget::addMessageRow(const SyslogMessage& msg)
{
    // 检查是否需要过滤
    int facilityIdx = m_facilityFilter->currentIndex();
    int severityIdx = m_severityFilter->currentIndex();
    QString searchText = m_searchEdit->text().trimmed();

    bool visible = true;
    if (facilityIdx > 0) {
        QString filterFac = m_facilityFilter->currentText();
        if (facilityName(msg.facility) != filterFac) {
            visible = false;
        }
    }
    if (severityIdx > 0) {
        QString filterSev = m_severityFilter->currentText();
        if (severityName(msg.severity) != filterSev) {
            visible = false;
        }
    }
    if (!searchText.isEmpty()) {
        if (!msg.message.contains(searchText, Qt::CaseInsensitive) &&
            !msg.hostname.contains(searchText, Qt::CaseInsensitive) &&
            !msg.sourceIP.contains(searchText, Qt::CaseInsensitive)) {
            visible = false;
        }
    }

    if (!visible) return;

    int row = m_messageTable->rowCount();
    m_messageTable->insertRow(row);

    QColor color = severityColor(msg.severity);

    // 时间
    auto* timeItem = new QTableWidgetItem(msg.time.toString("yyyy-MM-dd hh:mm:ss"));
    timeItem->setForeground(color);
    m_messageTable->setItem(row, 0, timeItem);

    // 来源 IP
    m_messageTable->setItem(row, 1, new QTableWidgetItem(msg.sourceIP));

    // Facility
    auto* facItem = new QTableWidgetItem(facilityName(msg.facility));
    m_messageTable->setItem(row, 2, facItem);

    // Severity
    auto* sevItem = new QTableWidgetItem(severityName(msg.severity));
    sevItem->setForeground(color);
    sevItem->setTextAlignment(Qt::AlignCenter);
    m_messageTable->setItem(row, 3, sevItem);

    // 主机名
    m_messageTable->setItem(row, 4, new QTableWidgetItem(msg.hostname));

    // 消息
    auto* msgItem = new QTableWidgetItem(msg.message);
    msgItem->setForeground(color);
    m_messageTable->setItem(row, 5, msgItem);

    // 自动滚动
    if (m_autoScrollCheck->isChecked()) {
        m_messageTable->scrollToBottom();
    }
}

void SyslogWidget::applyFilters()
{
    // 清空表格并重新填充
    m_messageTable->setRowCount(0);

    QMutexLocker lock(&m_mutex);
    for (const SyslogMessage& msg : m_messages) {
        addMessageRow(msg);
    }
}

void SyslogWidget::onFilterChanged()
{
    applyFilters();
}

void SyslogWidget::cleanupOldMessages()
{
    while (m_messages.size() > MAX_MESSAGES) {
        m_messages.removeFirst();
    }
}

void SyslogWidget::onClear()
{
    {
        QMutexLocker lock(&m_mutex);
        m_messages.clear();
    }
    m_messageTable->setRowCount(0);
    m_countLabel->setText("消息数: 0");
    Logger::instance().info("Syslog", "消息已清空");
}

// ============================================================================
// 导出 CSV
// ============================================================================
void SyslogWidget::onExportCSV()
{
    if (m_messages.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", "syslog_export.csv",
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

    out << "时间,来源IP,Facility,Severity,主机名,消息,原始数据\n";

    QMutexLocker lock(&m_mutex);
    for (const SyslogMessage& msg : m_messages) {
        auto csvEscape = [](const QString& val) -> QString {
            QString v = val;
            if (v.contains(',') || v.contains('\n') || v.contains('"')) {
                v.replace("\"", "\"\"");
                v = "\"" + v + "\"";
            }
            return v;
        };

        out << csvEscape(msg.time.toString("yyyy-MM-dd hh:mm:ss")) << ","
            << csvEscape(msg.sourceIP) << ","
            << csvEscape(facilityName(msg.facility)) << ","
            << csvEscape(severityName(msg.severity)) << ","
            << csvEscape(msg.hostname) << ","
            << csvEscape(msg.message) << ","
            << csvEscape(msg.rawData) << "\n";
    }

    file.close();
    Logger::instance().info("Syslog",
        QString("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(m_messages.size()));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(m_messages.size()).arg(filePath));
}