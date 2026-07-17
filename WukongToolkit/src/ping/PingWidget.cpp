#include "ping/PingWidget.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QPainter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QPen>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════
// PingChartWidget
// ═══════════════════════════════════════════════════════════════════════════

PingChartWidget::PingChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxPoints(200)
{
    setMinimumHeight(160);
    
}

void PingChartWidget::addPoint(double value)
{
    m_points.append(value);
    if (m_points.size() > m_maxPoints) {
        m_points.remove(0, m_points.size() - m_maxPoints);
    }
    update();
}

void PingChartWidget::clear()
{
    m_points.clear();
    update();
}

void PingChartWidget::setMaxPoints(int max)
{
    m_maxPoints = max;
}

void PingChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int margin = 30;
    int plotW = w - margin * 2;
    int plotH = h - margin * 2;

    if (plotW <= 0 || plotH <= 0) return;

    // Background grid
    painter.setPen(QPen(QColor(0x3C, 0x3F, 0x41), 1, Qt::DotLine));
    int gridLines = 5;
    for (int i = 0; i <= gridLines; ++i) {
        int y = margin + plotH * i / gridLines;
        painter.drawLine(margin, y, margin + plotW, y);
    }

    // Border
    painter.setPen(QPen(QColor(0x3C, 0x3F, 0x41), 1));
    painter.drawRect(margin, margin, plotW, plotH);

    if (m_points.isEmpty()) {
        painter.setPen(QColor(0x8B, 0x94, 0x9E));
        painter.setFont(QFont("Arial", 10));
        painter.drawText(rect(), Qt::AlignCenter, "等待数据...");
        return;
    }

    // Compute value range
    double minVal = m_points[0];
    double maxVal = m_points[0];
    for (double v : m_points) {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    double range = maxVal - minVal;
    if (range < 0.001) {
        range = 1.0;
        minVal = minVal - 0.5;
    }

    double pad = range * 0.1;
    minVal -= pad;
    maxVal += pad;
    range = maxVal - minVal;

    // Draw line
    painter.setPen(QPen(QColor(0x58, 0xA6, 0xFF), 2));
    int count = m_points.size();
    for (int i = 0; i < count - 1; ++i) {
        double x1 = margin + (double)i / (count - 1) * plotW;
        double y1 = margin + plotH - (m_points[i] - minVal) / range * plotH;
        double x2 = margin + (double)(i + 1) / (count - 1) * plotW;
        double y2 = margin + plotH - (m_points[i + 1] - minVal) / range * plotH;
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // Draw dots
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x58, 0xA6, 0xFF));
    for (int i = 0; i < count; ++i) {
        double x = margin + (double)i / (count - 1) * plotW;
        double y = margin + plotH - (m_points[i] - minVal) / range * plotH;
        if (count > 100 && i % 5 != 0) continue; // Skip dots when too many points
        painter.drawEllipse(QPointF(x, y), 3, 3);
    }

    // Axis labels
    painter.setPen(QColor(0x8B, 0x94, 0x9E));
    painter.setFont(QFont("Arial", 8));

    QString maxLabel = QString::number(maxVal, 'f', 1) + " ms";
    QString minLabel = QString::number(minVal, 'f', 1) + " ms";
    painter.drawText(2, margin - 4, maxLabel);
    painter.drawText(2, margin + plotH + 12, minLabel);

    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "延迟图表 (ms)");
}

// ═══════════════════════════════════════════════════════════════════════════
// PingWidget
// ═══════════════════════════════════════════════════════════════════════════

PingWidget::PingWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_currentHostIndex(0)
    , m_sent(0)
    , m_received(0)
    , m_minTime(0)
    , m_maxTime(0)
    , m_totalTime(0)
    , m_running(false)
    , m_stopping(false)
    , m_seq(0)
    , m_pendingStatisticsSent(0)
    , m_pendingStatisticsReceived(0)
    , m_collectingStats(false)
{
    setupUI();
    setupConnections();
}

PingWidget::~PingWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void PingWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Top: configuration panel ──
    auto* configGroup = new QGroupBox("Ping 配置");
    auto* configLayout = new QHBoxLayout(configGroup);
    configLayout->setSpacing(12);

    auto* hostLayout = new QVBoxLayout();
    auto* hostLabel = new QLabel("目标主机:");
    
    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("多个主机用逗号分隔, 如: 8.8.8.8, 1.1.1.1");
    m_hostEdit->setMinimumWidth(280);
    m_hostEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(m_hostEdit);
    configLayout->addLayout(hostLayout, 2);

    auto addSpinBox = [&](const QString& label, int min, int max, int val, const QString& suffix) -> QSpinBox* {
        auto* layout = new QVBoxLayout();
        auto* lbl = new QLabel(label);
        
        auto* spin = new QSpinBox();
        spin->setRange(min, max);
        spin->setValue(val);
        spin->setSuffix(suffix);
        spin->setFixedWidth(100);
        spin->setStyleSheet(
            "QSpinBox {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 4px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        );
        layout->addWidget(lbl);
        layout->addWidget(spin);
        configLayout->addLayout(layout);
        return spin;
    };

    m_countSpin = addSpinBox("次数:", 1, 10000, 4, " 次");
    m_intervalSpin = addSpinBox("间隔:", 1, 10000, 1000, " ms");
    m_sizeSpin = addSpinBox("包大小:", 16, 65500, 56, " B");
    m_timeoutSpin = addSpinBox("超时:", 100, 60000, 5000, " ms");

    configLayout->addStretch();

    m_startBtn = new QPushButton("开始 Ping");
    m_startBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_startBtn->setFixedHeight(36);

    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F85149; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #FF7B72; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_stopBtn->setFixedHeight(36);
    m_stopBtn->setEnabled(false);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_exportBtn->setFixedHeight(36);

    auto* btnLayout = new QVBoxLayout();
    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(m_exportBtn);
    btnLayout->addLayout(btnRow);
    btnLayout->addStretch();
    configLayout->addLayout(btnLayout);

    mainLayout->addWidget(configGroup);

    // ── Middle: chart + results table ──
    auto* splitter = new QSplitter(Qt::Vertical);

    // Chart
    m_chartWidget = new PingChartWidget();
    splitter->addWidget(m_chartWidget);

    // Results table
    m_resultTable = new QTableWidget(0, 5);
    m_resultTable->setHorizontalHeaderLabels({"主机", "序号", "TTL", "延迟(ms)", "状态"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setStyleSheet(
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
    splitter->addWidget(m_resultTable);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    // ── Bottom: statistics panel ──
    auto* statsGroup = new QGroupBox("统计信息");
    auto* statsLayout = new QHBoxLayout(statsGroup);
    statsLayout->setSpacing(24);

    auto makeStatWidget = [](const QString& title, const QString& initialValue) -> QLabel* {
        Q_UNUSED(title) // title is used in the layout below
        return new QLabel(initialValue);
    };

    auto addStatItem = [&](const QString& title, QLabel*& valueLabel) {
        auto* layout = new QVBoxLayout();
        auto* titleLbl = new QLabel(title);
        
        valueLabel = new QLabel("-");
        valueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #58A6FF;");
        layout->addWidget(titleLbl);
        layout->addWidget(valueLabel);
        statsLayout->addLayout(layout);
    };

    addStatItem("发送", m_sentLabel);
    addStatItem("接收", m_receivedLabel);
    addStatItem("丢失", m_lostLabel);
    addStatItem("最小", m_minLabel);
    addStatItem("平均", m_avgLabel);
    addStatItem("最大", m_maxLabel);
    addStatItem("标准差", m_stddevLabel);

    statsLayout->addStretch();
    mainLayout->addWidget(statsGroup);
}

// ─── Connections ───────────────────────────────────────────────────────

void PingWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &PingWidget::startPing);
    connect(m_stopBtn, &QPushButton::clicked, this, &PingWidget::stopPing);
    connect(m_exportBtn, &QPushButton::clicked, this, &PingWidget::exportToCSV);
}

// ─── Control ───────────────────────────────────────────────────────────

void PingWidget::startPing()
{
    QString hostText = m_hostEdit->text().trimmed();
    if (hostText.isEmpty()) {
        Logger::instance().warning("Ping", "No host specified");
        return;
    }

    m_hosts.clear();
    QStringList parts = hostText.split(',', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        QString host = part.trimmed();
        if (!host.isEmpty()) {
            m_hosts.append(host);
        }
    }

    if (m_hosts.isEmpty()) {
        Logger::instance().warning("Ping", "No valid hosts");
        return;
    }

    // Reset state
    m_resultTable->setRowCount(0);
    m_results.clear();
    m_chartWidget->clear();
    m_sent = 0;
    m_received = 0;
    m_minTime = 0;
    m_maxTime = 0;
    m_totalTime = 0;
    m_times.clear();
    m_seq = 0;
    m_running = true;
    m_stopping = false;
    m_collectingStats = false;
    m_statisticsBuffer.clear();

    updateStatistics();

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_hostEdit->setEnabled(false);

    m_currentHostIndex = 0;
    nextHost();

    Logger::instance().info("Ping", QString("Started ping to %1 host(s)").arg(m_hosts.size()));
}

void PingWidget::stopPing()
{
    m_stopping = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    Logger::instance().info("Ping", "Ping stopped by user");
}

void PingWidget::nextHost()
{
    if (m_currentHostIndex >= m_hosts.size() || m_stopping) {
        // All hosts done or stopped
        m_running = false;
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_hostEdit->setEnabled(true);
        Logger::instance().info("Ping", "Ping completed");
        return;
    }

    m_currentHost = m_hosts[m_currentHostIndex];
    m_currentHostIndex++;

    startPingForHost(m_currentHost);
}

void PingWidget::startPingForHost(const QString& host)
{
    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    // Reset per-host state
    m_pendingStatisticsHost = host;
    m_pendingStatisticsSent = 0;
    m_pendingStatisticsReceived = 0;
    m_collectingStats = false;
    m_statisticsBuffer.clear();

    m_process = new QProcess(this);

    QStringList args;
    args << "-c" << QString::number(m_countSpin->value());
    args << "-i" << QString::number(m_intervalSpin->value() / 1000.0, 'f', 1);
    args << "-s" << QString::number(m_sizeSpin->value());
    args << "-W" << QString::number(m_timeoutSpin->value());
    args << host;

    connect(m_process, &QProcess::readyReadStandardOutput, this, &PingWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PingWidget::onProcessFinished);

    m_process->start("/sbin/ping", args);
    Logger::instance().info("Ping", QString("Pinging %1 (count=%2, interval=%3ms, size=%4, timeout=%5ms)")
                                      .arg(host)
                                      .arg(m_countSpin->value())
                                      .arg(m_intervalSpin->value())
                                      .arg(m_sizeSpin->value())
                                      .arg(m_timeoutSpin->value()));
}

// ─── Process Callbacks ─────────────────────────────────────────────────

void PingWidget::onProcessReadyRead()
{
    if (!m_process) return;

    while (m_process->canReadLine()) {
        QByteArray data = m_process->readLine();
        QString line = QString::fromUtf8(data).trimmed();
        if (!line.isEmpty()) {
            parseLine(line);
        }
    }
}

void PingWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (m_stopping) {
        m_running = false;
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_hostEdit->setEnabled(true);
        Logger::instance().info("Ping", "Ping stopped");
        return;
    }

    // Compute statistics for the just-finished host
    if (!m_times.isEmpty()) {
        double avg = m_totalTime / m_times.size();

        double sumSq = 0;
        for (double t : m_times) {
            double diff = t - avg;
            sumSq += diff * diff;
        }
        double stddev = std::sqrt(sumSq / m_times.size());

        double loss = 0;
        if (m_pendingStatisticsSent > 0) {
            loss = 100.0 * (m_pendingStatisticsSent - m_pendingStatisticsReceived) / m_pendingStatisticsSent;
        }

        saveToHistory(m_pendingStatisticsHost, avg, loss, m_maxTime, m_minTime);
    }

    // Reset per-host times for next host
    m_times.clear();
    m_minTime = 0;
    m_maxTime = 0;
    m_totalTime = 0;

    nextHost();
}

// ─── Parsing ───────────────────────────────────────────────────────────

void PingWidget::parseLine(const QString& line)
{
    // Check if entering statistics section
    if (line.startsWith("---") && line.contains("ping statistics")) {
        m_collectingStats = true;
        m_statisticsBuffer.clear();
        return;
    }

    if (m_collectingStats) {
        m_statisticsBuffer += line + "\n";
        // The statistics block ends after the rtt line
        if (line.startsWith("round-trip") || line.startsWith("rtt")) {
            m_collectingStats = false;
            parseStatisticsLine(m_statisticsBuffer);
        }
        return;
    }

    // Try to parse as a reply line: "64 bytes from 8.8.8.8: icmp_seq=0 ttl=64 time=12.3 ms"
    if (line.contains("bytes from") && line.contains("icmp_seq") && line.contains("time")) {
        parseReplyLine(line);
        m_pendingStatisticsSent++;
        return;
    }

    // Request timeout
    if (line.contains("Request timeout") || line.contains("Host is down") ||
        line.contains("No route to host") || line.contains("100.0% packet loss")) {
        m_seq = m_pendingStatisticsSent + 1;
        addResultRow(m_currentHost, m_seq, 0, 0, "超时");
        m_pendingStatisticsSent++;
        return;
    }

    // Other error lines
    if (line.contains("unknown host") || line.contains("cannot resolve") ||
        line.contains("Unknown host")) {
        addResultRow(m_currentHost, 0, 0, 0, "解析失败");
        Logger::instance().error("Ping", "Host resolution failed: " + m_currentHost);
        return;
    }
}

void PingWidget::parseReplyLine(const QString& line)
{
    // Pattern: "64 bytes from 8.8.8.8: icmp_seq=0 ttl=64 time=12.3 ms"
    static QRegularExpression re(
        R"(icmp_seq=(\d+)\s+ttl=(\d+)\s+time=([\d.]+)\s*ms)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        int seq = match.captured(1).toInt() + 1; // Convert to 1-based
        int ttl = match.captured(2).toInt();
        double timeMs = match.captured(3).toDouble();

        m_pendingStatisticsReceived++;

        // Track global min/max/total
        if (m_times.isEmpty()) {
            m_minTime = timeMs;
            m_maxTime = timeMs;
        } else {
            if (timeMs < m_minTime) m_minTime = timeMs;
            if (timeMs > m_maxTime) m_maxTime = timeMs;
        }
        m_totalTime += timeMs;
        m_times.append(timeMs);

        // Update global counters
        m_sent++;
        m_received++;

        addResultRow(m_currentHost, seq, ttl, timeMs, "成功");
        m_chartWidget->addPoint(timeMs);
        updateStatistics();
    }
}

void PingWidget::parseStatisticsLine(const QString& block)
{
    // Parse "5 packets transmitted, 5 packets received, 0.0% packet loss"
    static QRegularExpression statsRe(
        R"((\d+)\s+packets?\s+transmitted.*?(\d+)\s+packets?\s+received.*?([\d.]+)%\s+packet\s+loss)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch statsMatch = statsRe.match(block);
    if (statsMatch.hasMatch()) {
        m_pendingStatisticsSent = statsMatch.captured(1).toInt();
        m_pendingStatisticsReceived = statsMatch.captured(2).toInt();
    }

    // Parse "round-trip min/avg/max/stddev = 1.234/1.500/2.000/0.500 ms"
    static QRegularExpression rttRe(
        R"(min/avg/max/stddev\s*=\s*([\d.]+)/([\d.]+)/([\d.]+)/([\d.]+)\s*ms)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch rttMatch = rttRe.match(block);
    if (rttMatch.hasMatch()) {
        double minV = rttMatch.captured(1).toDouble();
        double avgV = rttMatch.captured(2).toDouble();
        double maxV = rttMatch.captured(3).toDouble();
        double stddevV = rttMatch.captured(4).toDouble();

        // Use system stats if our per-packet tracking was incomplete
        if (m_times.isEmpty()) {
            m_minTime = minV;
            m_maxTime = maxV;
            m_totalTime = avgV * m_pendingStatisticsReceived;
        }

        updateStatistics();
    }
}

// ─── Result Display ────────────────────────────────────────────────────

void PingWidget::addResultRow(const QString& host, int seq, int ttl, double timeMs, const QString& status)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    auto* hostItem = new QTableWidgetItem(host);
    auto* seqItem = new QTableWidgetItem(QString::number(seq));
    auto* ttlItem = new QTableWidgetItem(ttl > 0 ? QString::number(ttl) : "-");
    auto* timeItem = new QTableWidgetItem(status == "成功" ? QString::number(timeMs, 'f', 3) : "-");
    auto* statusItem = new QTableWidgetItem(status);

    // Color coding
    QColor statusColor = (status == "成功") ? QColor(0x3F, 0xB9, 0x50) : QColor(0xF8, 0x51, 0x49);
    statusItem->setForeground(statusColor);

    if (status == "成功") {
        timeItem->setForeground(QColor(0x58, 0xA6, 0xFF));
    }

    m_resultTable->setItem(row, 0, hostItem);
    m_resultTable->setItem(row, 1, seqItem);
    m_resultTable->setItem(row, 2, ttlItem);
    m_resultTable->setItem(row, 3, timeItem);
    m_resultTable->setItem(row, 4, statusItem);

    m_resultTable->scrollToBottom();

    // Store result for CSV export
    m_results.append({host, seq, ttl, timeMs, status});
}

void PingWidget::updateStatistics()
{
    m_sentLabel->setText(QString::number(m_sent));

    if (m_sent > 0) {
        m_receivedLabel->setText(QString::number(m_received));
        double lossPercent = 100.0 * (m_sent - m_received) / m_sent;
        m_lostLabel->setText(QString::number(lossPercent, 'f', 1) + "%");

        if (lossPercent > 0) {
            m_lostLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #F85149;");
        } else {
            m_lostLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #3FB950;");
        }
    }

    if (!m_times.isEmpty()) {
        m_minLabel->setText(QString::number(m_minTime, 'f', 2));
        m_maxLabel->setText(QString::number(m_maxTime, 'f', 2));

        double avg = m_totalTime / m_times.size();
        m_avgLabel->setText(QString::number(avg, 'f', 2));

        double sumSq = 0;
        for (double t : m_times) {
            double diff = t - avg;
            sumSq += diff * diff;
        }
        double stddev = std::sqrt(sumSq / m_times.size());
        m_stddevLabel->setText(QString::number(stddev, 'f', 2));
    }
}

// ─── History ───────────────────────────────────────────────────────────

void PingWidget::saveToHistory(const QString& host, double avg, double loss, double max, double min)
{
    auto& db = DatabaseManager::instance();
    if (!db.isOpen()) {
        Logger::instance().warning("Ping", "Database not open, skipping history save");
        return;
    }

    QSqlQuery query(db.database());
    query.prepare(
        "INSERT INTO ping_history (host, time, average, loss, max, min) "
        "VALUES (:host, :time, :average, :loss, :max, :min)"
    );
    query.bindValue(":host", host);
    query.bindValue(":time", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":average", avg);
    query.bindValue(":loss", loss);
    query.bindValue(":max", max);
    query.bindValue(":min", min);

    if (!query.exec()) {
        Logger::instance().error("Ping", "Failed to save history: " + query.lastError().text());
    } else {
        Logger::instance().info("Ping", QString("History saved for %1").arg(host));
    }
}

// ─── Export ────────────────────────────────────────────────────────────

void PingWidget::exportToCSV()
{
    if (m_results.isEmpty()) {
        Logger::instance().warning("Ping", "No results to export");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 Ping 结果",
        QString("ping_results_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().error("Ping", "Failed to open file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    // BOM for Excel compatibility
    stream << "\xEF\xBB\xBF";
    stream << "主机,序号,TTL,延迟(ms),状态\n";

    for (const auto& r : m_results) {
        stream << r.host << ","
               << r.seq << ","
               << (r.ttl > 0 ? QString::number(r.ttl) : "-") << ","
               << (r.status == "成功" ? QString::number(r.timeMs, 'f', 3) : "-") << ","
               << r.status << "\n";
    }

    file.close();
    Logger::instance().info("Ping", "Results exported to: " + filePath);
}