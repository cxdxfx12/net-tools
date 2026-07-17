#include "network/PortScannerWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QMetaObject>
#include <QRegularExpression>

// ============================================================================
// 内置端口 → 服务名映射表
// ============================================================================
static const QHash<int, QString>& serviceTable()
{
    static const QHash<int, QString> t = {
        // 经典服务
        {7,    "Echo"},
        {9,    "Discard"},
        {13,   "Daytime"},
        {17,   "QOTD"},
        {19,   "Chargen"},
        {20,   "FTP-Data"},
        {21,   "FTP"},
        {22,   "SSH"},
        {23,   "Telnet"},
        {25,   "SMTP"},
        {37,   "Time"},
        {42,   "WINS"},
        {43,   "WHOIS"},
        {49,   "TACACS"},
        {53,   "DNS"},
        {67,   "DHCP-Server"},
        {68,   "DHCP-Client"},
        {69,   "TFTP"},
        {70,   "Gopher"},
        {79,   "Finger"},
        {80,   "HTTP"},
        {88,   "Kerberos"},
        {110,  "POP3"},
        {111,  "RPC"},
        {113,  "Ident"},
        {119,  "NNTP"},
        {123,  "NTP"},
        {135,  "MS-RPC"},
        {137,  "NetBIOS-NS"},
        {138,  "NetBIOS-DGM"},
        {139,  "NetBIOS-SSN"},
        {143,  "IMAP"},
        {161,  "SNMP"},
        {162,  "SNMP-Trap"},
        {179,  "BGP"},
        {194,  "IRC"},
        {389,  "LDAP"},
        {443,  "HTTPS"},
        {445,  "SMB"},
        {464,  "Kerberos-Admin"},
        {465,  "SMTPS"},
        {500,  "IKE"},
        {514,  "Syslog"},
        {515,  "LPD"},
        {520,  "RIP"},
        {546,  "DHCPv6-Client"},
        {547,  "DHCPv6-Server"},
        {554,  "RTSP"},
        {587,  "SMTP-Submission"},
        {636,  "LDAPS"},
        {873,  "Rsync"},
        {993,  "IMAPS"},
        {995,  "POP3S"},
        {1080, "SOCKS"},
        {1194, "OpenVPN"},
        {1433, "MSSQL"},
        {1434, "MSSQL-UDP"},
        {1521, "Oracle"},
        {1701, "L2TP"},
        {1723, "PPTP"},
        {1812, "RADIUS"},
        {1813, "RADIUS-Acct"},
        {1883, "MQTT"},
        {2049, "NFS"},
        {2082, "cPanel"},
        {2083, "cPanel-SSL"},
        {2181, "ZooKeeper"},
        {2375, "Docker"},
        {2376, "Docker-SSL"},
        {3128, "Squid"},
        {3260, "iSCSI"},
        {3306, "MySQL"},
        {3389, "RDP"},
        {3478, "STUN"},
        {4000, "Diablo2"},
        {4369, "Erlang-EPMD"},
        {4444, "Metasploit"},
        {4500, "IPsec-NAT"},
        {4567, "Tram"},
        {4848, "GlassFish"},
        {5000, "UPnP"},
        {5001, "UPnP"},
        {5060, "SIP"},
        {5061, "SIP-TLS"},
        {5222, "XMPP"},
        {5269, "XMPP-S2S"},
        {5353, "mDNS"},
        {5432, "PostgreSQL"},
        {5555, "Android-ADB"},
        {5672, "RabbitMQ"},
        {5683, "CoAP"},
        {5900, "VNC"},
        {5901, "VNC-1"},
        {5938, "TeamViewer"},
        {5984, "CouchDB"},
        {5985, "WinRM-HTTP"},
        {5986, "WinRM-HTTPS"},
        {6000, "X11"},
        {6379, "Redis"},
        {6443, "K8s-API"},
        {6666, "IRC-SSL"},
        {6667, "IRC"},
        {6697, "IRC-SSL"},
        {7000, "Cassandra"},
        {7001, "Cassandra-TLS"},
        {7070, "RealServer"},
        {7474, "Neo4j"},
        {7687, "Neo4j-Bolt"},
        {8000, "HTTP-Alt"},
        {8008, "HTTP-Alt"},
        {8009, "AJP"},
        {8080, "HTTP-Proxy"},
        {8081, "HTTP-Alt"},
        {8443, "HTTPS-Alt"},
        {8888, "HTTP-Alt"},
        {9000, "SonarQube"},
        {9042, "Cassandra"},
        {9090, "Prometheus"},
        {9092, "Kafka"},
        {9100, "Prom-Node"},
        {9200, "Elasticsearch"},
        {9300, "Elasticsearch"},
        {9418, "Git"},
        {9999, "HTTP-Alt"},
        {10000, "Webmin"},
        {10050, "Zabbix-Agent"},
        {10051, "Zabbix-Server"},
        {11211, "Memcached"},
        {15672, "RabbitMQ-Mgmt"},
        {16379, "Redis-Sentinel"},
        {26379, "Redis-Sentinel"},
        {27017, "MongoDB"},
        {27018, "MongoDB"},
        {27019, "MongoDB"},
        {28017, "MongoDB-Web"},
        {50000, "DB2"},
        {50010, "Hadoop-DFS"},
        {50030, "Hadoop"},
        {50070, "Hadoop-Web"},
        {50075, "Hadoop-DFS"},
        {50090, "Hadoop"},
        {61616, "ActiveMQ"},
    };
    return t;
}

// ============================================================================
// PortScanWorker 实现
// ============================================================================
PortScanWorker::PortScanWorker(const QString& host, int port, int timeoutMs,
                               QObject* receiver, QAtomicInt* scanningFlag,
                               QAtomicInt* progressCounter)
    : m_host(host)
    , m_port(port)
    , m_timeoutMs(timeoutMs)
    , m_receiver(receiver)
    , m_scanning(scanningFlag)
    , m_progress(progressCounter)
{
    setAutoDelete(true);
}

QString PortScanWorker::readBanner(QTcpSocket* socket, int timeoutMs)
{
    if (!socket->waitForReadyRead(timeoutMs)) {
        return {};
    }

    QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        return {};
    }

    // 清理不可打印字符，截断到合理长度
    QString banner = QString::fromUtf8(data).trimmed();
    QString clean;
    clean.reserve(banner.size());
    for (const QChar& ch : banner) {
        ushort uc = ch.unicode();
        if (uc >= 0x20 && uc <= 0x7E) {
            clean += ch;
        } else if (ch == '\n' || ch == '\r' || ch == '\t') {
            clean += ' ';
        }
    }
    clean = clean.simplified();

    if (clean.length() > 200) {
        clean = clean.left(200) + "...";
    }
    return clean;
}

void PortScanWorker::run()
{
    if (m_scanning->loadRelaxed() == 0) return;

    QElapsedTimer timer;
    timer.start();

    QTcpSocket socket;
    socket.connectToHost(m_host, static_cast<quint16>(m_port));

    bool connected = socket.waitForConnected(m_timeoutMs);
    qint64 elapsed = timer.elapsed();

    QString service = PortScannerWidget::lookupService(m_port);
    QString state;
    QString banner;

    if (connected) {
        state = "开放";
        // 尝试读取 Banner
        banner = readBanner(&socket, qMin(2000, m_timeoutMs));
        socket.disconnectFromHost();
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            socket.waitForDisconnected(1000);
        }
    } else {
        state = "关闭";
        elapsed = -1;
    }

    QMetaObject::invokeMethod(m_receiver, "onResultReceived", Qt::QueuedConnection,
                              Q_ARG(int, m_port),
                              Q_ARG(QString, service),
                              Q_ARG(qint64, elapsed),
                              Q_ARG(QString, state),
                              Q_ARG(QString, banner));

    m_progress->fetchAndAddRelaxed(1);
}

// ============================================================================
// PortScannerWidget 实现
// ============================================================================
PortScannerWidget::PortScannerWidget(QWidget* parent)
    : QWidget(parent)
    , m_threadPool(new QThreadPool(this))
    , m_scanning(0)
    , m_scannedCount(0)
    , m_totalCount(0)
    , m_openCount(0)
{
    m_threadPool->setMaxThreadCount(32);
    setupUI();
    setupConnections();
}

PortScannerWidget::~PortScannerWidget()
{
    m_scanning.storeRelaxed(0);
    m_threadPool->clear();
    m_threadPool->waitForDone(5000);
}

QString PortScannerWidget::lookupService(int port)
{
    return serviceTable().value(port);
}

void PortScannerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // --- 扫描配置 ---
    auto* configGroup = new QGroupBox("扫描配置");
    auto* configForm = new QFormLayout(configGroup);
    configForm->setSpacing(6);

    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("例如: 192.168.1.1 或 example.com");
    configForm->addRow("目标主机:", m_hostEdit);

    m_portRangeEdit = new QLineEdit();
    m_portRangeEdit->setPlaceholderText("例如: 1-1024 或 22,80,443,8000-9000");
    configForm->addRow("端口范围:", m_portRangeEdit);

    m_scanTypeCombo = new QComboBox();
    m_scanTypeCombo->addItem("TCP Connect");
    configForm->addRow("扫描类型:", m_scanTypeCombo);

    m_threadCountSpin = new QSpinBox();
    m_threadCountSpin->setRange(1, 256);
    m_threadCountSpin->setValue(32);
    m_threadCountSpin->setSuffix(" 线程");
    configForm->addRow("线程数:", m_threadCountSpin);

    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(100, 30000);
    m_timeoutSpin->setValue(2000);
    m_timeoutSpin->setSingleStep(100);
    m_timeoutSpin->setSuffix(" ms");
    configForm->addRow("超时:", m_timeoutSpin);

    mainLayout->addWidget(configGroup);

    // --- 按钮栏 ---
    auto* btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton("开始扫描");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setEnabled(false);

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(btnLayout);

    // --- 进度条 ---
    auto* progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("就绪");
    progressLayout->addWidget(m_progressBar);
    mainLayout->addLayout(progressLayout);

    // --- 结果表 ---
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(5);
    QStringList headers = {"端口", "服务", "状态", "响应时间", "Banner"};
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setSortingEnabled(false);

    // 设置列宽
    m_resultTable->setColumnWidth(0, 80);
    m_resultTable->setColumnWidth(1, 160);
    m_resultTable->setColumnWidth(2, 80);
    m_resultTable->setColumnWidth(3, 120);
    m_resultTable->setColumnWidth(4, 300);

    mainLayout->addWidget(m_resultTable);
}

void PortScannerWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &PortScannerWidget::onStartScan);
    connect(m_stopBtn, &QPushButton::clicked, this, &PortScannerWidget::onStopScan);
    connect(m_exportBtn, &QPushButton::clicked, this, &PortScannerWidget::onExportCSV);
    connect(m_resultTable, &QTableWidget::customContextMenuRequested,
            this, &PortScannerWidget::onContextMenu);
}

// ============================================================================
// 端口范围解析
// ============================================================================
void PortScannerWidget::parsePortRange(const QString& input, QList<int>& outPorts)
{
    outPorts.clear();
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) return;

    // 按逗号分割
    QStringList parts = trimmed.split(',', Qt::SkipEmptyParts);

    for (const QString& part : parts) {
        QString p = part.trimmed();

        // 判断是否为范围 "start-end"
        if (p.contains('-')) {
            QStringList range = p.split('-');
            if (range.size() == 2) {
                bool ok1 = false, ok2 = false;
                int start = range[0].trimmed().toInt(&ok1);
                int end = range[1].trimmed().toInt(&ok2);
                if (ok1 && ok2 && start >= 1 && end <= 65535 && start <= end) {
                    for (int port = start; port <= end; ++port) {
                        outPorts.append(port);
                    }
                }
            }
        } else {
            // 单个端口号
            bool ok = false;
            int port = p.toInt(&ok);
            if (ok && port >= 1 && port <= 65535) {
                outPorts.append(port);
            }
        }
    }

    // 去重并排序
    std::sort(outPorts.begin(), outPorts.end());
    outPorts.erase(std::unique(outPorts.begin(), outPorts.end()), outPorts.end());
}

// ============================================================================
// 扫描控制
// ============================================================================
void PortScannerWidget::onStartScan()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入目标主机地址。");
        return;
    }

    QList<int> ports;
    parsePortRange(m_portRangeEdit->text(), ports);
    if (ports.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "无法解析端口范围，请检查输入格式。\n"
                             "支持格式: 1-1024, 22,80,443, 20-25,80,443,8000-9000");
        return;
    }

    clearResults();
    m_scanning.storeRelaxed(1);
    m_scannedCount.storeRelaxed(0);
    m_totalCount = ports.size();
    m_openCount = 0;

    m_threadPool->setMaxThreadCount(m_threadCountSpin->value());

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_exportBtn->setEnabled(false);
    m_progressBar->setRange(0, m_totalCount);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("扫描中... %v / %m");

    int timeout = m_timeoutSpin->value();

    Logger::instance().info("PortScanner",
                            QString("开始扫描 %1，%2 个端口，%3 线程，超时 %4 ms")
                                .arg(host).arg(m_totalCount).arg(m_threadCountSpin->value()).arg(timeout));

    // 提交所有任务
    for (int port : ports) {
        auto* worker = new PortScanWorker(host, port, timeout, this, &m_scanning, &m_scannedCount);
        m_threadPool->start(worker);
    }

    // 定时器检查扫描完成
    auto* timer = new QTimer(this);
    timer->setObjectName("scanCompleteTimer");
    connect(timer, &QTimer::timeout, this, [this, timer]() {
        int scanned = m_scannedCount.loadRelaxed();
        m_progressBar->setValue(scanned);
        if (scanned >= m_totalCount) {
            timer->stop();
            timer->deleteLater();
            m_scanning.storeRelaxed(0);
            m_startBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            m_exportBtn->setEnabled(true);
            m_progressBar->setFormat(QString("完成 — 发现 %1 个开放端口").arg(m_openCount));
            Logger::instance().info("PortScanner",
                                    QString("扫描完成: %1/%2 个端口开放").arg(m_openCount).arg(m_totalCount));
            emit scanFinished(m_openCount);
        }
    });
    timer->start(200);
}

void PortScannerWidget::onStopScan()
{
    m_scanning.storeRelaxed(0);
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_exportBtn->setEnabled(m_openCount > 0);
    m_progressBar->setFormat(QString("已停止 — 发现 %1 个开放端口").arg(m_openCount));
    Logger::instance().info("PortScanner", "扫描已手动停止");
}

// ============================================================================
// 结果处理
// ============================================================================
void PortScannerWidget::onResultReceived(int port, QString service, qint64 responseTime,
                                          QString state, QString banner)
{
    QMutexLocker lock(&m_resultMutex);

    if (state == "开放") {
        m_openCount++;
    }

    addResultRow(port, service, state, responseTime, banner);

    if (state == "开放") {
        emit portFound(port, service, responseTime);
    }
}

void PortScannerWidget::addResultRow(int port, const QString& service, const QString& state,
                                      qint64 responseTime, const QString& banner)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    // 端口
    auto* portItem = new QTableWidgetItem(QString::number(port));
    portItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 0, portItem);

    // 服务
    m_resultTable->setItem(row, 1, new QTableWidgetItem(service.isEmpty() ? "-" : service));

    // 状态
    auto* stateItem = new QTableWidgetItem(state);
    if (state == "开放") {
        stateItem->setForeground(QColor(0, 150, 0));
    } else {
        stateItem->setForeground(QColor(180, 0, 0));
    }
    stateItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 2, stateItem);

    // 响应时间
    QString rtStr = (responseTime >= 0) ? QString("%1 ms").arg(responseTime) : "-";
    auto* rtItem = new QTableWidgetItem(rtStr);
    rtItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 3, rtItem);

    // Banner
    m_resultTable->setItem(row, 4, new QTableWidgetItem(banner.isEmpty() ? "-" : banner));

    m_resultTable->scrollToBottom();
}

void PortScannerWidget::clearResults()
{
    m_resultTable->setRowCount(0);
    m_openCount = 0;
}

// ============================================================================
// 导出 CSV
// ============================================================================
void PortScannerWidget::onExportCSV()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", "port_scan_result.csv",
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

    // 表头
    out << "端口,服务,状态,响应时间,Banner\n";

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* item = m_resultTable->item(row, col);
            QString val = item ? item->text() : "-";
            // 如果值包含逗号或换行则加引号
            if (val.contains(',') || val.contains('\n') || val.contains('"')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            cols << val;
        }
        out << cols.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("PortScanner",
                            QString("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(m_resultTable->rowCount()));
    QMessageBox::information(this, "导出成功",
                              QString("已导出 %1 条记录到:\n%2").arg(m_resultTable->rowCount()).arg(filePath));
}

// ============================================================================
// 右键菜单
// ============================================================================
void PortScannerWidget::onContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_resultTable->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString portStr = m_resultTable->item(row, 0)->text();
    QString service = m_resultTable->item(row, 1)->text();
    QString banner = m_resultTable->item(row, 4)->text();
    QString host = m_hostEdit->text().trimmed();

    QMenu menu(this);
    QAction* copyPortAction = menu.addAction("复制端口号");
    QAction* copyServiceAction = menu.addAction("复制服务名");
    QAction* copyBannerAction = menu.addAction("复制 Banner");
    QAction* copyHostPortAction = menu.addAction(QString("复制 %1:%2").arg(host, portStr));
    menu.addSeparator();
    QAction* copyRowAction = menu.addAction("复制整行");

    copyServiceAction->setEnabled(service != "-");
    copyBannerAction->setEnabled(banner != "-");

    QAction* chosen = menu.exec(m_resultTable->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == copyPortAction) {
        QApplication::clipboard()->setText(portStr);
        Logger::instance().info("PortScanner", "已复制端口: " + portStr);
    } else if (chosen == copyServiceAction) {
        QApplication::clipboard()->setText(service);
        Logger::instance().info("PortScanner", "已复制服务名: " + service);
    } else if (chosen == copyBannerAction) {
        QApplication::clipboard()->setText(banner);
        Logger::instance().info("PortScanner", "已复制 Banner: " + banner);
    } else if (chosen == copyHostPortAction) {
        QString hp = host + ":" + portStr;
        QApplication::clipboard()->setText(hp);
        Logger::instance().info("PortScanner", "已复制: " + hp);
    } else if (chosen == copyRowAction) {
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* it = m_resultTable->item(row, col);
            cols << (it ? it->text() : "-");
        }
        QApplication::clipboard()->setText(cols.join('\t'));
        Logger::instance().info("PortScanner", "已复制整行: " + cols.join(" | "));
    }
}