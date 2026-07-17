#include "network/FlowWidget.h"
#include "log/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// ─── ChartWidget ──────────────────────────────────────────────────────

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(150);
    
}

void ChartWidget::addDataPoint(qreal value)
{
    m_data.append(value);
    if (m_data.size() > MAX_POINTS) {
        m_data.removeFirst();
    }
    update();
}

void ChartWidget::clear()
{
    m_data.clear();
    update();
}

void ChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    if (m_data.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 40;

    // Background
    painter.fillRect(rect(), QColor("#0D1117"));

    // Find max
    qreal maxVal = *std::max_element(m_data.begin(), m_data.end());
    maxVal = std::max(maxVal, (qreal)1.0);

    // Grid
    painter.setPen(QPen(QColor("#30363D"), 1, Qt::DotLine));
    for (int i = 0; i < 5; ++i) {
        int y = margin + (h - margin * 2) * i / 4;
        painter.drawLine(margin, y, w - margin, y);
    }

    // Axis
    painter.setPen(QColor("#8B949E"));
    painter.drawLine(margin, margin, margin, h - margin);
    painter.drawLine(margin, h - margin, w - margin, h - margin);

    // Y-axis labels
    for (int i = 0; i < 5; ++i) {
        int y = margin + (h - margin * 2) * i / 4;
        qreal val = maxVal * (4 - i) / 4;
        painter.drawText(2, y + 4, QString::number(val, 'f', 0));
    }

    // Data line
    if (m_data.size() >= 2) {
        QPainterPath path;
        qreal stepX = (qreal)(w - margin * 2) / std::max(1, (int)m_data.size() - 1);

        for (int i = 0; i < m_data.size(); ++i) {
            qreal x = margin + i * stepX;
            qreal y = margin + (h - margin * 2) * (1.0 - m_data[i] / maxVal);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }

        painter.setPen(QPen(QColor("#58A6FF"), 2));
        painter.drawPath(path);

        // Fill
        QPainterPath fillPath = path;
        fillPath.lineTo(margin + (m_data.size() - 1) * stepX, h - margin);
        fillPath.lineTo(margin, h - margin);
        fillPath.closeSubpath();
        painter.fillPath(fillPath, QColor(64, 158, 255, 30));
    }
}

// ─── FlowWidget ───────────────────────────────────────────────────────

FlowWidget::FlowWidget(QWidget* parent)
    : QWidget(parent)
    , m_socket(new QUdpSocket(this))
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
}

FlowWidget::~FlowWidget()
{
    if (m_running) {
        m_socket->close();
    }
}

void FlowWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Control panel
    auto* controlGroup = new QGroupBox("流量监控");
    auto* controlLayout = new QHBoxLayout(controlGroup);

    controlLayout->addWidget(new QLabel("监听端口:"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(2055);
    controlLayout->addWidget(m_portSpin);

    controlLayout->addWidget(new QLabel("协议:"));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItems({"NetFlow v5", "NetFlow v9", "sFlow"});
    controlLayout->addWidget(m_protocolCombo);

    m_startStopBtn = new QPushButton("启动");
    controlLayout->addWidget(m_startStopBtn);

    m_statusLabel = new QLabel("● 已停止");
    controlLayout->addWidget(m_statusLabel);

    controlLayout->addStretch();
    mainLayout->addWidget(controlGroup);

    // Summary panel
    auto* summaryLayout = new QHBoxLayout();
    auto makeCard = [](const QString& title, QLabel*& valueLabel) -> QWidget* {
        auto* card = new QWidget();
        auto* layout = new QVBoxLayout(card);
        layout->setSpacing(4);
        auto* titleLabel = new QLabel(title);
        
        valueLabel = new QLabel("0");
        valueLabel->setStyleSheet("color: #58A6FF; font-size: 20px; font-weight: bold; border: none;");
        layout->addWidget(titleLabel);
        layout->addWidget(valueLabel);
        return card;
    };

    summaryLayout->addWidget(makeCard("Total Flows", m_totalFlowsLabel));
    summaryLayout->addWidget(makeCard("Total Packets", m_totalPacketsLabel));
    summaryLayout->addWidget(makeCard("Total Bytes", m_totalBytesLabel));
    summaryLayout->addWidget(makeCard("Active Sources", m_activeSourcesLabel));
    mainLayout->addLayout(summaryLayout);

    // Top Talkers
    auto* topGroup = new QGroupBox("Top Talkers");
    auto* topLayout = new QVBoxLayout(topGroup);
    m_topTalkersTable = new QTableWidget();
    m_topTalkersTable->setColumnCount(7);
    m_topTalkersTable->setHorizontalHeaderLabels({"Source IP", "Dest IP", "Protocol", "Packets", "Bytes", "Port Src", "Port Dst"});
    m_topTalkersTable->horizontalHeader()->setStretchLastSection(true);
    m_topTalkersTable->setAlternatingRowColors(true);
    m_topTalkersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    topLayout->addWidget(m_topTalkersTable);
    mainLayout->addWidget(topGroup);

    // Chart
    m_chartWidget = new ChartWidget();
    mainLayout->addWidget(m_chartWidget);

    // Flow records
    auto* flowGroup = new QGroupBox("Flow Records");
    auto* flowLayout = new QVBoxLayout(flowGroup);
    m_flowTable = new QTableWidget();
    m_flowTable->setColumnCount(9);
    m_flowTable->setHorizontalHeaderLabels({"Time", "Source IP", "Source Addr", "Dest Addr", "Protocol", "Src Port", "Dst Port", "Packets", "Bytes"});
    m_flowTable->horizontalHeader()->setStretchLastSection(true);
    m_flowTable->setAlternatingRowColors(true);
    flowLayout->addWidget(m_flowTable);
    mainLayout->addWidget(flowGroup);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    m_clearBtn = new QPushButton("清空");
    m_exportBtn = new QPushButton("导出 CSV");
    btnLayout->addStretch();
    btnLayout->addWidget(m_clearBtn);
    btnLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(btnLayout);
}

void FlowWidget::setupConnections()
{
    connect(m_startStopBtn, &QPushButton::clicked, this, &FlowWidget::onStartStop);
    connect(m_socket, &QUdpSocket::readyRead, this, &FlowWidget::onReadDatagram);
    connect(m_refreshTimer, &QTimer::timeout, this, &FlowWidget::onRefreshTimer);
    connect(m_clearBtn, &QPushButton::clicked, this, &FlowWidget::onClear);
    connect(m_exportBtn, &QPushButton::clicked, this, &FlowWidget::onExportCSV);

    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx == 2) m_portSpin->setValue(6343); // sFlow
        else m_portSpin->setValue(2055); // NetFlow
    });
}

void FlowWidget::onStartStop()
{
    if (!m_running) {
        if (m_socket->bind(QHostAddress::AnyIPv4, m_portSpin->value())) {
            m_running = true;
            m_startStopBtn->setText("停止");
            m_statusLabel->setText("● 运行中");
            m_portSpin->setEnabled(false);
            m_protocolCombo->setEnabled(false);
            m_refreshTimer->start(1000);
            Logger::instance().info("FLOW", "Listener started on port " + QString::number(m_portSpin->value()));
        } else {
            QMessageBox::warning(this, "错误", "无法绑定端口 " + QString::number(m_portSpin->value()));
        }
    } else {
        m_running = false;
        m_socket->close();
        m_refreshTimer->stop();
        m_startStopBtn->setText("启动");
        m_statusLabel->setText("● 已停止");
        m_portSpin->setEnabled(true);
        m_protocolCombo->setEnabled(true);
        Logger::instance().info("FLOW", "Listener stopped");
    }
}

void FlowWidget::onReadDatagram()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray buffer;
        buffer.resize(static_cast<int>(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
        parseNetFlowV5(buffer, sender.toString());
    }
}

void FlowWidget::parseNetFlowV5(const QByteArray& data, const QString& sourceIP)
{
    if (data.size() < 24) return; // Header too small

    // NetFlow v5 Header (24 bytes)
    // version(2) count(2) sys_uptime(4) unix_secs(4) unix_nsecs(4) flow_sequence(4) engine_type(1) engine_id(1) sampling(2)
    quint16 version = (data[0] << 8) | static_cast<quint8>(data[1]);
    if (version != 5) return;

    quint16 count = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    quint32 unixSecs = (static_cast<quint8>(data[8]) << 24) | (static_cast<quint8>(data[9]) << 16) |
                       (static_cast<quint8>(data[10]) << 8) | static_cast<quint8>(data[11]);

    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < count && (24 + i * 48 + 48) <= data.size(); ++i) {
        const uchar* rec = reinterpret_cast<const uchar*>(data.constData()) + 24 + i * 48;

        FlowRecord record;
        record.srcaddr = (rec[0] << 24) | (rec[1] << 16) | (rec[2] << 8) | rec[3];
        record.dstaddr = (rec[4] << 24) | (rec[5] << 16) | (rec[6] << 8) | rec[7];
        record.srcport = (rec[32] << 8) | rec[33];
        record.dstport = (rec[34] << 8) | rec[35];
        record.protocol = rec[38];
        record.dPkts = (rec[16] << 24) | (rec[17] << 16) | (rec[18] << 8) | rec[19];
        record.dOctets = (rec[20] << 24) | (rec[21] << 16) | (rec[22] << 8) | rec[23];
        record.timestamp = unixSecs;
        record.sourceIP = sourceIP;

        m_flowRecords.append(record);
        m_activeSources.insert(sourceIP);

        // Update talker map
        QString key = QString("%1_%2_%3_%4_%5")
                          .arg(ipToString(record.srcaddr), ipToString(record.dstaddr))
                          .arg(record.protocol).arg(record.srcport).arg(record.dstport);

        if (m_talkerMap.contains(key)) {
            TopTalker& t = m_talkerMap[key];
            t.packets += record.dPkts;
            t.bytes += record.dOctets;
        } else {
            TopTalker t;
            t.srcIP = ipToString(record.srcaddr);
            t.dstIP = ipToString(record.dstaddr);
            t.protocol = record.protocol;
            t.srcPort = record.srcport;
            t.dstPort = record.dstport;
            t.packets = record.dPkts;
            t.bytes = record.dOctets;
            m_talkerMap.insert(key, t);
        }
    }
}

QString FlowWidget::ipToString(quint32 ip) const
{
    return QString("%1.%2.%3.%4")
        .arg((ip >> 24) & 0xFF)
        .arg((ip >> 16) & 0xFF)
        .arg((ip >> 8) & 0xFF)
        .arg(ip & 0xFF);
}

QString FlowWidget::protocolName(quint8 proto) const
{
    switch (proto) {
    case 1: return "ICMP";
    case 6: return "TCP";
    case 17: return "UDP";
    case 2: return "IGMP";
    case 47: return "GRE";
    case 50: return "ESP";
    case 51: return "AH";
    default: return QString::number(proto);
    }
}

void FlowWidget::onRefreshTimer()
{
    QMutexLocker locker(&m_mutex);
    updateTopTalkers();
    updateSummary();
}

void FlowWidget::updateTopTalkers()
{
    // Sort talkers by bytes
    QList<TopTalker> talkers;
    for (auto it = m_talkerMap.begin(); it != m_talkerMap.end(); ++it) {
        talkers.append(it.value());
    }
    std::sort(talkers.begin(), talkers.end(), [](const TopTalker& a, const TopTalker& b) {
        return a.bytes > b.bytes;
    });

    m_topTalkersTable->setRowCount(std::min(100, (int)talkers.size()));
    for (int i = 0; i < m_topTalkersTable->rowCount(); ++i) {
        const auto& t = talkers[i];
        m_topTalkersTable->setItem(i, 0, new QTableWidgetItem(t.srcIP));
        m_topTalkersTable->setItem(i, 1, new QTableWidgetItem(t.dstIP));
        m_topTalkersTable->setItem(i, 2, new QTableWidgetItem(protocolName(t.protocol)));
        m_topTalkersTable->setItem(i, 3, new QTableWidgetItem(QString::number(t.packets)));
        m_topTalkersTable->setItem(i, 4, new QTableWidgetItem(QString::number(t.bytes)));
        m_topTalkersTable->setItem(i, 5, new QTableWidgetItem(QString::number(t.srcPort)));
        m_topTalkersTable->setItem(i, 6, new QTableWidgetItem(QString::number(t.dstPort)));
    }

    // Update chart
    quint64 totalBytes = 0;
    for (const auto& t : talkers) totalBytes += t.bytes;
    m_chartWidget->addDataPoint(totalBytes);
}

void FlowWidget::updateSummary()
{
    m_totalFlowsLabel->setText(QString::number(m_flowRecords.size()));
    m_activeSourcesLabel->setText(QString::number(m_activeSources.size()));

    quint64 totalPackets = 0, totalBytes = 0;
    for (const auto& record : m_flowRecords) {
        totalPackets += record.dPkts;
        totalBytes += record.dOctets;
    }
    m_totalPacketsLabel->setText(QString::number(totalPackets));
    m_totalBytesLabel->setText(QString::number(totalBytes));

    // Update flow table (last 1000)
    m_flowTable->setRowCount(std::min(1000, (int)m_flowRecords.size()));
    for (int i = 0; i < m_flowTable->rowCount(); ++i) {
        const auto& r = m_flowRecords[i];
        m_flowTable->setItem(i, 0, new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(r.timestamp).toString("hh:mm:ss")));
        m_flowTable->setItem(i, 1, new QTableWidgetItem(r.sourceIP));
        m_flowTable->setItem(i, 2, new QTableWidgetItem(ipToString(r.srcaddr)));
        m_flowTable->setItem(i, 3, new QTableWidgetItem(ipToString(r.dstaddr)));
        m_flowTable->setItem(i, 4, new QTableWidgetItem(protocolName(r.protocol)));
        m_flowTable->setItem(i, 5, new QTableWidgetItem(QString::number(r.srcport)));
        m_flowTable->setItem(i, 6, new QTableWidgetItem(QString::number(r.dstport)));
        m_flowTable->setItem(i, 7, new QTableWidgetItem(QString::number(r.dPkts)));
        m_flowTable->setItem(i, 8, new QTableWidgetItem(QString::number(r.dOctets)));
    }
}

void FlowWidget::onExportCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出 CSV", "flows.csv", "CSV (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF"; // BOM

    out << "Time,Source IP,Source Addr,Dest Addr,Protocol,Src Port,Dst Port,Packets,Bytes\n";
    for (int i = 0; i < m_flowRecords.size(); ++i) {
        const auto& r = m_flowRecords[i];
        out << QDateTime::fromSecsSinceEpoch(r.timestamp).toString("hh:mm:ss") << ","
            << r.sourceIP << "," << ipToString(r.srcaddr) << "," << ipToString(r.dstaddr) << ","
            << protocolName(r.protocol) << "," << r.srcport << "," << r.dstport << ","
            << r.dPkts << "," << r.dOctets << "\n";
    }

    Logger::instance().info("FLOW", "Exported to " + fileName);
}

void FlowWidget::onClear()
{
    QMutexLocker locker(&m_mutex);
    m_flowRecords.clear();
    m_activeSources.clear();
    m_talkerMap.clear();
    m_topTalkersTable->setRowCount(0);
    m_flowTable->setRowCount(0);
    m_chartWidget->clear();
    updateSummary();
    Logger::instance().info("FLOW", "Data cleared");
}