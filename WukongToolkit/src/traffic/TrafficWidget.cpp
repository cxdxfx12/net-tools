#include "traffic/TrafficWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QLabel>
#include <QFrame>
#include <QSplitter>
#include <QRandomGenerator>
#include <QDateTime>
#include <QGridLayout>
#include <QtMath>

// ═══════════════════════════════════════════════════════════════════════════════
// Helper: format traffic bytes to human-readable string
// ═══════════════════════════════════════════════════════════════════════════════

static QString formatTraffic(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024);
    } else if (bytes < 1024LL * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024 * 1024));
    } else {
        return QString("%.2f GB").arg(bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// BandwidthTrendWidget — bar chart with in/out dual-color bars
// ═══════════════════════════════════════════════════════════════════════════════

BandwidthTrendWidget::BandwidthTrendWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    
}

void BandwidthTrendWidget::setData(const QList<TrafficData>& data)
{
    m_data = data;
    update();
}

void BandwidthTrendWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    if (m_data.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int marginLeft = 70;
    int marginRight = 20;
    int marginTop = 20;
    int marginBottom = 50;

    // Background
    painter.fillRect(rect(), QColor("#0D1117"));

    int plotW = w - marginLeft - marginRight;
    int plotH = h - marginTop - marginBottom;

    // Find max value for scaling
    qint64 maxVal = 0;
    for (const auto& d : m_data) {
        maxVal = std::max(maxVal, std::max(d.inBytes, d.outBytes));
    }
    if (maxVal == 0) maxVal = 1;

    // Grid lines
    painter.setPen(QPen(QColor("#30363D"), 1, Qt::DotLine));
    for (int i = 0; i <= 4; ++i) {
        int y = marginTop + plotH * i / 4;
        painter.drawLine(marginLeft, y, w - marginRight, y);
    }

    // Y-axis labels
    painter.setPen(QColor("#8B949E"));
    QFont labelFont = painter.font();
    labelFont.setPixelSize(10);
    painter.setFont(labelFont);
    for (int i = 0; i <= 4; ++i) {
        qint64 val = maxVal * (4 - i) / 4;
        int y = marginTop + plotH * i / 4;
        painter.drawText(QRect(0, y - 8, marginLeft - 6, 16), Qt::AlignRight | Qt::AlignVCenter, formatTraffic(val));
    }

    // Bar chart
    int barCount = m_data.size();
    int groupWidth = plotW / barCount;
    int barWidth = qMax(4, groupWidth / 3);

    for (int i = 0; i < barCount; ++i) {
        const auto& d = m_data[i];

        qreal inH = (qreal)d.inBytes / maxVal * plotH;
        qreal outH = (qreal)d.outBytes / maxVal * plotH;

        int groupX = marginLeft + i * groupWidth;
        int inX = groupX + groupWidth / 2 - barWidth - 2;
        int outX = groupX + groupWidth / 2 + 2;

        // Inbound bar (blue)
        painter.setBrush(QColor("#58A6FF"));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRectF(inX, marginTop + plotH - inH, barWidth, inH));

        // Outbound bar (green)
        painter.setBrush(QColor("#3FB950"));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRectF(outX, marginTop + plotH - outH, barWidth, outH));

        // Interface name label (rotated or abbreviated)
        QString ifName = d.interface;
        if (ifName.length() > 8) ifName = ifName.left(7) + ".";
        painter.setPen(QColor("#8B949E"));
        painter.drawText(QRect(groupX, marginTop + plotH + 4, groupWidth, marginBottom - 8),
                         Qt::AlignHCenter | Qt::AlignTop, ifName);
    }

    // Legend
    int legendY = marginTop - 16;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#58A6FF"));
    painter.drawRect(marginLeft, legendY + 2, 10, 10);
    painter.setPen(QColor("#E6EDF3"));
    painter.drawText(marginLeft + 14, legendY + 2, 10, 10, Qt::AlignVCenter, "入");
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#3FB950"));
    painter.drawRect(marginLeft + 40, legendY + 2, 10, 10);
    painter.setPen(QColor("#E6EDF3"));
    painter.drawText(marginLeft + 54, legendY + 2, 10, 10, Qt::AlignVCenter, "出");
}

// ═══════════════════════════════════════════════════════════════════════════════
// ProtocolPieWidget — pie chart for protocol distribution
// ═══════════════════════════════════════════════════════════════════════════════

ProtocolPieWidget::ProtocolPieWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
    
}

void ProtocolPieWidget::setData(qint64 tcpBytes, qint64 udpBytes, qint64 icmpBytes, qint64 othersBytes)
{
    m_tcpBytes = tcpBytes;
    m_udpBytes = udpBytes;
    m_icmpBytes = icmpBytes;
    m_othersBytes = othersBytes;
    update();
}

void ProtocolPieWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int side = qMin(w, h);
    int pieSize = side * 0.55;
    int cx = w / 2;
    int cy = h / 2;
    int legendX = cx + pieSize / 2 + 30;

    // Background
    painter.fillRect(rect(), QColor("#0D1117"));

    qint64 total = m_tcpBytes + m_udpBytes + m_icmpBytes + m_othersBytes;
    if (total == 0) {
        painter.setPen(QColor("#8B949E"));
        painter.drawText(rect(), Qt::AlignCenter, "无数据");
        return;
    }

    struct Slice {
        QString label;
        qint64 value;
        QColor color;
    };

    QList<Slice> slices = {
        {"TCP",    m_tcpBytes,    QColor("#58A6FF")},
        {"UDP",    m_udpBytes,    QColor("#3FB950")},
        {"ICMP",   m_icmpBytes,   QColor("#D29922")},
        {"Others", m_othersBytes, QColor("#8B949E")},
    };

    // Draw pie slices
    QRectF pieRect(cx - pieSize / 2, cy - pieSize / 2, pieSize, pieSize);
    qreal startAngle = 90.0; // Start from top (12 o'clock)

    for (const auto& slice : slices) {
        if (slice.value == 0) continue;

        qreal spanAngle = (qreal)slice.value / total * 360.0 * 16.0;
        painter.setBrush(slice.color);
        painter.setPen(QPen(QColor("#0D1117"), 2));
        painter.drawPie(pieRect, static_cast<int>(startAngle * 16), static_cast<int>(spanAngle));
        startAngle += spanAngle / 16.0;
    }

    // Legend
    int legendItemH = 22;
    int legendStartY = cy - (slices.size() * legendItemH) / 2;

    QFont legendFont = painter.font();
    legendFont.setPixelSize(11);
    painter.setFont(legendFont);

    for (int i = 0; i < slices.size(); ++i) {
        const auto& slice = slices[i];
        if (slice.value == 0 && total > 0) continue;

        int itemY = legendStartY + i * legendItemH;

        // Color box
        painter.setPen(Qt::NoPen);
        painter.setBrush(slice.color);
        painter.drawRect(legendX, itemY + 3, 12, 12);

        // Label
        double pct = (total > 0) ? (100.0 * slice.value / total) : 0.0;
        painter.setPen(QColor("#E6EDF3"));
        painter.drawText(legendX + 18, itemY, 120, legendItemH, Qt::AlignVCenter,
                         QString("%1 (%2%)").arg(slice.label).arg(pct, 0, 'f', 1));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// TrafficWidget
// ═══════════════════════════════════════════════════════════════════════════════

TrafficWidget::TrafficWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();
    generateMockData();
    updateTopTalker();
}

TrafficWidget::~TrafficWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void TrafficWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Table helper (layout config only, styling via global QSS) ──
    auto configTable = [](QTableWidget* table) {
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
    };

    // ── Top: Device selection and time range ──
    auto* topGroup = new QGroupBox("流量分析中心");
    topGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(12);

    auto* deviceLabel = new QLabel("设备:");
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(200);
    m_deviceCombo->addItems({"Core-Switch-01", "Core-Switch-02", "Agg-Switch-01",
                             "Agg-Switch-02", "Access-Switch-01", "Router-01",
                             "Firewall-01", "Firewall-02"});

    auto* timeLabel = new QLabel("时间范围:");
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setMinimumWidth(140);
    m_timeRangeCombo->addItems({"最近 5 分钟", "最近 15 分钟", "最近 1 小时",
                                "最近 6 小时", "最近 24 小时", "最近 7 天"});
    m_timeRangeCombo->setCurrentIndex(2);

    auto* refreshBtn = new QPushButton("立即刷新");
    refreshBtn->setStyleSheet("QPushButton { background-color: #3FB950; color: white; border: none; } QPushButton:hover { background-color: #56D364; }");
    refreshBtn->setFixedHeight(34);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet("QPushButton { background-color: #D29922; color: white; border: none; } QPushButton:hover { background-color: #DBAB4A; }");
    m_exportBtn->setFixedHeight(34);

    topLayout->addWidget(deviceLabel);
    topLayout->addWidget(m_deviceCombo);
    topLayout->addWidget(timeLabel);
    topLayout->addWidget(m_timeRangeCombo);
    topLayout->addWidget(refreshBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(topGroup);

    connect(refreshBtn, &QPushButton::clicked, this, &TrafficWidget::onRefresh);

    // ── Middle: Four-panel grid layout ──
    auto* gridWidget = new QWidget();
    auto* grid = new QGridLayout(gridWidget);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(8);

    // Top-left: Bandwidth trend chart
    auto* bwGroup = new QGroupBox("带宽趋势");
    bwGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* bwLayout = new QVBoxLayout(bwGroup);
    bwLayout->setContentsMargins(4, 8, 4, 4);
    m_bandwidthChart = new BandwidthTrendWidget();
    bwLayout->addWidget(m_bandwidthChart);

    // Top-right: Protocol pie chart
    auto* protoGroup = new QGroupBox("协议分布");
    protoGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* protoLayout = new QVBoxLayout(protoGroup);
    protoLayout->setContentsMargins(4, 8, 4, 4);
    m_protocolPie = new ProtocolPieWidget();
    protoLayout->addWidget(m_protocolPie);

    // Bottom-left: Top Interface table
    auto* ifGroup = new QGroupBox("Top Interface");
    ifGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* ifLayout = new QVBoxLayout(ifGroup);
    ifLayout->setContentsMargins(4, 8, 4, 4);
    m_topInterfaceTable = new QTableWidget(0, 5);
    m_topInterfaceTable->setHorizontalHeaderLabels({"接口", "入流量", "出流量", "利用率", "状态"});
    m_topInterfaceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_topInterfaceTable->setColumnWidth(1, 100);
    m_topInterfaceTable->setColumnWidth(2, 100);
    m_topInterfaceTable->setColumnWidth(3, 80);
    m_topInterfaceTable->setColumnWidth(4, 70);
    configTable(m_topInterfaceTable);
    ifLayout->addWidget(m_topInterfaceTable);

    // Bottom-right: Top Talker table
    auto* talkerGroup = new QGroupBox("Top Talker");
    talkerGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* talkerLayout = new QVBoxLayout(talkerGroup);
    talkerLayout->setContentsMargins(4, 8, 4, 4);
    m_topTalkerTable = new QTableWidget(0, 5);
    m_topTalkerTable->setHorizontalHeaderLabels({"源IP", "目标IP", "协议", "流量", "会话数"});
    m_topTalkerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_topTalkerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_topTalkerTable->setColumnWidth(2, 70);
    m_topTalkerTable->setColumnWidth(3, 90);
    m_topTalkerTable->setColumnWidth(4, 70);
    configTable(m_topTalkerTable);
    talkerLayout->addWidget(m_topTalkerTable);

    grid->addWidget(bwGroup, 0, 0);
    grid->addWidget(protoGroup, 0, 1);
    grid->addWidget(ifGroup, 1, 0);
    grid->addWidget(talkerGroup, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 3);
    grid->setColumnStretch(1, 2);

    mainLayout->addWidget(gridWidget, 1);

    // ── Bottom: Broadcast/Multicast statistics ──
    auto* bcastGroup = new QGroupBox("广播/组播统计");
    bcastGroup->setStyleSheet("QGroupBox { color: #58A6FF; font-weight: bold; }");
    auto* bcastLayout = new QVBoxLayout(bcastGroup);
    bcastLayout->setContentsMargins(4, 8, 4, 4);

    m_broadcastTable = new QTableWidget(0, 4);
    m_broadcastTable->setHorizontalHeaderLabels({"类型", "包数", "字节数", "占比"});
    m_broadcastTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_broadcastTable->setColumnWidth(1, 120);
    m_broadcastTable->setColumnWidth(2, 120);
    m_broadcastTable->setColumnWidth(3, 80);
    configTable(m_broadcastTable);
    m_broadcastTable->setMaximumHeight(160);
    bcastLayout->addWidget(m_broadcastTable);

    mainLayout->addWidget(bcastGroup);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void TrafficWidget::setupConnections()
{
    connect(m_exportBtn, &QPushButton::clicked, this, &TrafficWidget::onExport);

    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        onRefresh();
    });

    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        onRefresh();
    });
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void TrafficWidget::onRefresh()
{
    generateMockData();
    updateTopTalker();

    Logger::instance().info("Traffic Analysis",
        QString("已刷新设备 %1 的流量数据").arg(m_deviceCombo->currentText()));
}

// ─── Generate Mock Data ──────────────────────────────────────────────────────

void TrafficWidget::generateMockData()
{
    auto* rng = QRandomGenerator::global();

    m_data.clear();

    struct SimInterface {
        QString name;
    };
    QList<SimInterface> interfaces = {
        {"GigabitEthernet0/0"},
        {"GigabitEthernet0/1"},
        {"GigabitEthernet0/2"},
        {"GigabitEthernet0/3"},
        {"TenGigabitEthernet1/0"},
        {"TenGigabitEthernet1/1"},
        {"Vlan100"},
        {"Vlan200"},
        {"Port-Channel1"},
        {"Loopback0"},
    };

    for (const auto& iface : interfaces) {
        TrafficData d;
        d.interface = iface.name;
        d.inBytes = rng->bounded(5000000LL, 80000000000LL);
        d.outBytes = rng->bounded(5000000LL, 60000000000LL);
        d.utilization = rng->bounded(5, 95);
        d.status = (rng->bounded(0, 100) < 90) ? "Up" : "Down";
        m_data.append(d);
    }

    // Sort by total bytes descending
    std::sort(m_data.begin(), m_data.end(), [](const TrafficData& a, const TrafficData& b) {
        return (a.inBytes + a.outBytes) > (b.inBytes + b.outBytes);
    });

    // Update bandwidth chart
    m_bandwidthChart->setData(m_data);

    // Update top interface table
    m_topInterfaceTable->setRowCount(m_data.size());
    for (int i = 0; i < m_data.size(); ++i) {
        const auto& d = m_data[i];

        m_topInterfaceTable->setItem(i, 0, new QTableWidgetItem(d.interface));
        m_topInterfaceTable->setItem(i, 1, new QTableWidgetItem(formatTraffic(d.inBytes)));
        m_topInterfaceTable->setItem(i, 2, new QTableWidgetItem(formatTraffic(d.outBytes)));

        auto* utilItem = new QTableWidgetItem(QString("%1%").arg(d.utilization, 0, 'f', 1));
        if (d.utilization > 80) {
            utilItem->setForeground(QColor("#F85149"));
        } else if (d.utilization > 60) {
            utilItem->setForeground(QColor("#D29922"));
        } else {
            utilItem->setForeground(QColor("#3FB950"));
        }
        m_topInterfaceTable->setItem(i, 3, utilItem);

        auto* statusItem = new QTableWidgetItem(d.status);
        if (d.status == "Up") {
            statusItem->setForeground(QColor("#3FB950"));
        } else {
            statusItem->setForeground(QColor("#F85149"));
        }
        m_topInterfaceTable->setItem(i, 4, statusItem);
    }

    // Update protocol pie chart
    qint64 tcpBytes = rng->bounded(10000000000LL, 500000000000LL);
    qint64 udpBytes = rng->bounded(5000000000LL, 200000000000LL);
    qint64 icmpBytes = rng->bounded(100000000LL, 10000000000LL);
    qint64 othersBytes = rng->bounded(50000000LL, 5000000000LL);
    m_protocolPie->setData(tcpBytes, udpBytes, icmpBytes, othersBytes);

    // Generate broadcast/multicast data
    m_broadcasts.clear();
    m_broadcasts.append({"ARP 广播", rng->bounded(1000LL, 50000LL), rng->bounded(50000LL, 5000000LL), 0.0});
    m_broadcasts.append({"DHCP 广播", rng->bounded(500LL, 10000LL), rng->bounded(50000LL, 2000000LL), 0.0});
    m_broadcasts.append({"ICMPv6 ND", rng->bounded(200LL, 5000LL), rng->bounded(20000LL, 500000LL), 0.0});
    m_broadcasts.append({"mDNS 组播", rng->bounded(100LL, 3000LL), rng->bounded(10000LL, 300000LL), 0.0});
    m_broadcasts.append({"IGMP 组播", rng->bounded(50LL, 2000LL), rng->bounded(5000LL, 200000LL), 0.0});
    m_broadcasts.append({"STP BPDU", rng->bounded(500LL, 10000LL), rng->bounded(30000LL, 600000LL), 0.0});

    qint64 totalBcastBytes = 0;
    for (const auto& b : m_broadcasts) {
        totalBcastBytes += b.bytes;
    }
    for (auto& b : m_broadcasts) {
        b.percentage = (totalBcastBytes > 0) ? (100.0 * b.bytes / totalBcastBytes) : 0.0;
    }

    // Update broadcast table
    m_broadcastTable->setRowCount(m_broadcasts.size());
    for (int i = 0; i < m_broadcasts.size(); ++i) {
        const auto& b = m_broadcasts[i];
        m_broadcastTable->setItem(i, 0, new QTableWidgetItem(b.type));
        m_broadcastTable->setItem(i, 1, new QTableWidgetItem(QString::number(b.packets)));
        m_broadcastTable->setItem(i, 2, new QTableWidgetItem(formatTraffic(b.bytes)));

        auto* pctItem = new QTableWidgetItem(QString("%1%").arg(b.percentage, 0, 'f', 1));
        if (b.percentage > 30) {
            pctItem->setForeground(QColor("#F85149"));
        } else if (b.percentage > 15) {
            pctItem->setForeground(QColor("#D29922"));
        } else {
            pctItem->setForeground(QColor("#3FB950"));
        }
        m_broadcastTable->setItem(i, 3, pctItem);
    }
}

// ─── Update Top Talker ───────────────────────────────────────────────────────

void TrafficWidget::updateTopTalker()
{
    auto* rng = QRandomGenerator::global();

    m_topTalkers.clear();

    struct ProtoInfo {
        QString name;
    };
    QList<ProtoInfo> protocols = {{"TCP"}, {"TCP"}, {"TCP"}, {"TCP"}, {"UDP"},
                                  {"UDP"}, {"UDP"}, {"ICMP"}, {"TCP"}, {"UDP"}};

    for (int i = 0; i < 10; ++i) {
        TopTalkerData t;
        t.srcIP = QString("192.168.%1.%2").arg(rng->bounded(1, 10)).arg(rng->bounded(2, 254));
        t.dstIP = QString("10.%1.%2.%3")
                      .arg(rng->bounded(0, 255))
                      .arg(rng->bounded(0, 255))
                      .arg(rng->bounded(1, 254));
        t.protocol = protocols[i].name;
        t.traffic = rng->bounded(1000000LL, 50000000000LL);
        t.sessions = rng->bounded(1, 500);
        m_topTalkers.append(t);
    }

    // Sort by traffic descending
    std::sort(m_topTalkers.begin(), m_topTalkers.end(), [](const TopTalkerData& a, const TopTalkerData& b) {
        return a.traffic > b.traffic;
    });

    m_topTalkerTable->setRowCount(m_topTalkers.size());
    for (int i = 0; i < m_topTalkers.size(); ++i) {
        const auto& t = m_topTalkers[i];

        m_topTalkerTable->setItem(i, 0, new QTableWidgetItem(t.srcIP));
        m_topTalkerTable->setItem(i, 1, new QTableWidgetItem(t.dstIP));

        auto* protoItem = new QTableWidgetItem(t.protocol);
        if (t.protocol == "TCP") {
            protoItem->setForeground(QColor("#58A6FF"));
        } else if (t.protocol == "UDP") {
            protoItem->setForeground(QColor("#3FB950"));
        } else if (t.protocol == "ICMP") {
            protoItem->setForeground(QColor("#D29922"));
        }
        m_topTalkerTable->setItem(i, 2, protoItem);

        m_topTalkerTable->setItem(i, 3, new QTableWidgetItem(formatTraffic(t.traffic)));
        m_topTalkerTable->setItem(i, 4, new QTableWidgetItem(QString::number(t.sessions)));
    }
}

// ─── Paint Bandwidth Chart ───────────────────────────────────────────────────

void TrafficWidget::paintBandwidthChart()
{
    m_bandwidthChart->update();
}

// ─── Paint Protocol Pie ──────────────────────────────────────────────────────

void TrafficWidget::paintProtocolPie()
{
    m_protocolPie->update();
}

// ─── Export CSV ──────────────────────────────────────────────────────────────

void TrafficWidget::onExport()
{
    if (m_topInterfaceTable->rowCount() == 0 && m_topTalkerTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出流量分析数据",
        QString("traffic_analysis_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("Traffic Analysis", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // Write device info
    stream << "设备," << m_deviceCombo->currentText() << "\n";
    stream << "时间范围," << m_timeRangeCombo->currentText() << "\n";
    stream << "导出时间," << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";

    // Write Top Interface table
    stream << "=== Top Interface ===\n";
    QStringList ifHeaders;
    for (int col = 0; col < m_topInterfaceTable->columnCount(); ++col) {
        auto* headerItem = m_topInterfaceTable->horizontalHeaderItem(col);
        ifHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << ifHeaders.join(",") << "\n";

    for (int row = 0; row < m_topInterfaceTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_topInterfaceTable->columnCount(); ++col) {
            auto* item = m_topInterfaceTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // Write Top Talker table
    stream << "\n=== Top Talker ===\n";
    QStringList talkerHeaders;
    for (int col = 0; col < m_topTalkerTable->columnCount(); ++col) {
        auto* headerItem = m_topTalkerTable->horizontalHeaderItem(col);
        talkerHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << talkerHeaders.join(",") << "\n";

    for (int row = 0; row < m_topTalkerTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_topTalkerTable->columnCount(); ++col) {
            auto* item = m_topTalkerTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // Write Broadcast/Multicast table
    stream << "\n=== 广播/组播统计 ===\n";
    QStringList bcastHeaders;
    for (int col = 0; col < m_broadcastTable->columnCount(); ++col) {
        auto* headerItem = m_broadcastTable->horizontalHeaderItem(col);
        bcastHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << bcastHeaders.join(",") << "\n";

    for (int row = 0; row < m_broadcastTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_broadcastTable->columnCount(); ++col) {
            auto* item = m_broadcastTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    file.close();

    int totalRows = m_topInterfaceTable->rowCount() + m_topTalkerTable->rowCount() + m_broadcastTable->rowCount();
    Logger::instance().info("Traffic Analysis",
        QString("导出 %1 条记录到: %2").arg(totalRows).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(totalRows).arg(filePath));
}