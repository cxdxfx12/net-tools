#include "traceroute/TracerouteWidget.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QApplication>

TracerouteWidget::TracerouteWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(new QProcess(this))
{
    setupUI();
    setupConnections();
    ensureHopDetailTable();
}

TracerouteWidget::~TracerouteWidget()
{
    if (m_running) {
        onStop();
    }
}

// ── UI Setup ─────────────────────────────────────────────────────────────────

void TracerouteWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // ── Control panel ──
    auto* controlGroup = new QGroupBox("路由追踪");
    auto* controlLayout = new QVBoxLayout(controlGroup);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(6);

    m_targetEdit = new QLineEdit();
    m_targetEdit->setPlaceholderText("输入目标主机或 IP 地址，如 example.com 或 8.8.8.8");
    formLayout->addRow("目标主机:", m_targetEdit);

    m_maxHopsSpin = new QSpinBox();
    m_maxHopsSpin->setRange(1, 255);
    m_maxHopsSpin->setValue(30);
    m_maxHopsSpin->setSuffix(" 跳");
    formLayout->addRow("最大跳数:", m_maxHopsSpin);

    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(1, 60);
    m_timeoutSpin->setValue(5);
    m_timeoutSpin->setSuffix(" 秒");
    formLayout->addRow("超时时间:", m_timeoutSpin);

    m_queriesSpin = new QSpinBox();
    m_queriesSpin->setRange(1, 10);
    m_queriesSpin->setValue(3);
    m_queriesSpin->setSuffix(" 次/跳");
    formLayout->addRow("每跳探测次数:", m_queriesSpin);

    controlLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_startBtn = new QPushButton("开始追踪");
    m_startBtn->setMinimumWidth(100);
    btnLayout->addWidget(m_startBtn);

    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setMinimumWidth(80);
    m_stopBtn->setEnabled(false);
    btnLayout->addWidget(m_stopBtn);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setMinimumWidth(80);
    btnLayout->addWidget(m_exportBtn);

    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setMinimumWidth(60);
    btnLayout->addWidget(m_clearBtn);

    btnLayout->addStretch();
    controlLayout->addLayout(btnLayout);

    mainLayout->addWidget(controlGroup);

    // ── Results table ──
    auto* tableGroup = new QGroupBox("追踪结果");
    auto* tableLayout = new QVBoxLayout(tableGroup);

    m_resultsTable = new QTableWidget(0, 8);
    m_resultsTable->setHorizontalHeaderLabels({
        "跳数", "IP 地址", "主机名", "RTT1(ms)", "RTT2(ms)", "RTT3(ms)", "平均(ms)", "丢包率"
    });
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->verticalHeader()->setVisible(false);

    m_resultsTable->setColumnWidth(0, 50);
    m_resultsTable->setColumnWidth(1, 130);
    m_resultsTable->setColumnWidth(2, 180);
    m_resultsTable->setColumnWidth(3, 80);
    m_resultsTable->setColumnWidth(4, 80);
    m_resultsTable->setColumnWidth(5, 80);
    m_resultsTable->setColumnWidth(6, 80);
    m_resultsTable->setColumnWidth(7, 70);

    tableLayout->addWidget(m_resultsTable);
    mainLayout->addWidget(tableGroup, 1);

    // ── Bottom panel: hop detail + statistics ──
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    auto* detailGroup = new QGroupBox("跳点详情");
    auto* detailLayout = new QVBoxLayout(detailGroup);
    m_hopDetailLabel = new QLabel("选择上方跳点查看详情");
    m_hopDetailLabel->setWordWrap(true);
    m_hopDetailLabel->setMinimumHeight(60);
    m_hopDetailLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailLayout->addWidget(m_hopDetailLabel);
    bottomLayout->addWidget(detailGroup, 2);

    auto* statGroup = new QGroupBox("统计信息");
    auto* statLayout = new QFormLayout(statGroup);
    statLayout->setSpacing(4);

    m_statTotalHops = new QLabel("0");
    statLayout->addRow("总跳数:", m_statTotalHops);

    m_statAvgRtt = new QLabel("N/A");
    statLayout->addRow("平均 RTT:", m_statAvgRtt);

    m_statPacketLoss = new QLabel("0%");
    statLayout->addRow("总丢包率:", m_statPacketLoss);

    bottomLayout->addWidget(statGroup, 1);
    mainLayout->addLayout(bottomLayout);
}

void TracerouteWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &TracerouteWidget::onStart);
    connect(m_stopBtn, &QPushButton::clicked, this, &TracerouteWidget::onStop);
    connect(m_exportBtn, &QPushButton::clicked, this, &TracerouteWidget::onExportCSV);
    connect(m_clearBtn, &QPushButton::clicked, this, &TracerouteWidget::onClear);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TracerouteWidget::onProcessReadyRead);
    connect(m_process, &QProcess::finished, this, &TracerouteWidget::onProcessFinished);

    connect(m_resultsTable, &QTableWidget::currentCellChanged, this, &TracerouteWidget::onTableItemSelected);

    connect(m_targetEdit, &QLineEdit::returnPressed, this, &TracerouteWidget::onStart);
}

// ── Actions ──────────────────────────────────────────────────────────────────

void TracerouteWidget::onStart()
{
    QString target = m_targetEdit->text().trimmed();
    if (target.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入目标主机地址。");
        return;
    }

    onClear();

    m_targetHost = target;
    m_hops.clear();
    m_currentHop = 0;
    m_running = true;
    m_startTime = QDateTime::currentDateTime();

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_targetEdit->setEnabled(false);
    m_maxHopsSpin->setEnabled(false);
    m_timeoutSpin->setEnabled(false);
    m_queriesSpin->setEnabled(false);

    QStringList args;
    args << "-q" << QString::number(m_queriesSpin->value());
    args << "-m" << QString::number(m_maxHopsSpin->value());
    args << "-w" << QString::number(m_timeoutSpin->value());
    args << target;

    Logger::instance().info("Traceroute", "Starting traceroute to " + target);

    m_process->start("traceroute", args);
}

void TracerouteWidget::onStop()
{
    if (m_running) {
        m_process->kill();
        m_running = false;
        Logger::instance().info("Traceroute", "Traceroute stopped by user");
    }

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_targetEdit->setEnabled(true);
    m_maxHopsSpin->setEnabled(true);
    m_timeoutSpin->setEnabled(true);
    m_queriesSpin->setEnabled(true);
}

void TracerouteWidget::onClear()
{
    m_resultsTable->setRowCount(0);
    m_hops.clear();
    m_currentHop = 0;
    m_hopDetailLabel->setText("选择上方跳点查看详情");
    m_statTotalHops->setText("0");
    m_statAvgRtt->setText("N/A");
    m_statPacketLoss->setText("0%");
}

// ── Process handling ─────────────────────────────────────────────────────────

void TracerouteWidget::onProcessReadyRead()
{
    while (m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty()) {
            parseLine(line);
        }
    }
}

void TracerouteWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_running = false;

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_targetEdit->setEnabled(true);
    m_maxHopsSpin->setEnabled(true);
    m_timeoutSpin->setEnabled(true);
    m_queriesSpin->setEnabled(true);

    if (exitStatus == QProcess::NormalExit) {
        updateStatistics();
        saveToHistory();
        Logger::instance().info("Traceroute",
            QString("Traceroute to %1 completed: %2 hops")
                .arg(m_targetHost)
                .arg(m_hops.size()));
    } else {
        if (exitCode != 0) {
            Logger::instance().error("Traceroute",
                QString("Traceroute to %1 failed with exit code %2")
                    .arg(m_targetHost)
                    .arg(exitCode));
        }
    }
}

// ── Parsing ──────────────────────────────────────────────────────────────────

void TracerouteWidget::parseLine(const QString& line)
{
    // Header line: "traceroute to example.com (93.184.216.34), 64 hops max, ..."
    static QRegularExpression headerRe(R"(traceroute to (\S+)\s*\((\S+)\))");
    QRegularExpressionMatch headerMatch = headerRe.match(line);
    if (headerMatch.hasMatch()) {
        m_targetHost = headerMatch.captured(1);
        m_targetIp = headerMatch.captured(2);
        return;
    }

    // Hop line: " 1  router.local (192.168.1.1)  1.234 ms  1.123 ms  0.987 ms"
    // or: " 1  * * *"
    static QRegularExpression hopRe(
        R"(^\s*(\d+)\s+(.+)$)");
    QRegularExpressionMatch hopMatch = hopRe.match(line);
    if (!hopMatch.hasMatch()) {
        return;
    }

    int hopNum = hopMatch.captured(1).toInt();
    QString hopData = hopMatch.captured(2).trimmed();

    TracerouteHop hop;
    hop.hop = hopNum;

    // Check if all timeouts
    if (hopData == "* * *" || hopData == "*") {
        hop.lossPercent = 100.0;
        addHopToTable(hop);
        m_hops.append(hop);
        return;
    }

    // Parse hostname and IP: "router.local (192.168.1.1)  1.234 ms  1.123 ms  0.987 ms"
    static QRegularExpression hostRe(R"(^(\S+)\s*\((\S+)\)\s+(.*)$)");
    QRegularExpressionMatch hostMatch = hostRe.match(hopData);
    if (hostMatch.hasMatch()) {
        hop.hostname = hostMatch.captured(1);
        hop.ip = hostMatch.captured(2);
        hopData = hostMatch.captured(3).trimmed();
    } else {
        // Might be just IP without hostname: "192.168.1.1  1.234 ms  1.123 ms  0.987 ms"
        static QRegularExpression ipOnlyRe(R"(^(\S+)\s+(.*)$)");
        QRegularExpressionMatch ipMatch = ipOnlyRe.match(hopData);
        if (ipMatch.hasMatch()) {
            hop.ip = ipMatch.captured(1);
            hopData = ipMatch.captured(2).trimmed();
        }
    }

    // Parse RTT values: "1.234 ms  1.123 ms  0.987 ms"
    static QRegularExpression rttRe(R"((\d+\.?\d*)\s*ms)");
    QRegularExpressionMatchIterator rttIt = rttRe.globalMatch(hopData);
    QVector<double> rtts;
    while (rttIt.hasNext()) {
        QRegularExpressionMatch m = rttIt.next();
        rtts.append(m.captured(1).toDouble());
    }

    if (rtts.size() >= 1) hop.rtt1 = rtts[0];
    if (rtts.size() >= 2) hop.rtt2 = rtts[1];
    if (rtts.size() >= 3) hop.rtt3 = rtts[2];

    double sum = 0.0;
    int count = 0;
    for (double r : rtts) {
        if (r >= 0) {
            sum += r;
            count++;
        }
    }
    if (count > 0) {
        hop.avgRtt = sum / count;
    }
    hop.lossPercent = (1.0 - static_cast<double>(count) / m_queriesSpin->value()) * 100.0;

    addHopToTable(hop);
    m_hops.append(hop);
}

void TracerouteWidget::addHopToTable(const TracerouteHop& hop)
{
    int row = m_resultsTable->rowCount();
    m_resultsTable->insertRow(row);

    auto setItem = [&](int col, const QString& text) {
        m_resultsTable->setItem(row, col, new QTableWidgetItem(text));
    };

    setItem(0, QString::number(hop.hop));
    setItem(1, hop.ip);
    setItem(2, hop.hostname);

    auto rttText = [](double v) -> QString {
        return (v >= 0) ? QString::number(v, 'f', 2) : "*";
    };
    setItem(3, rttText(hop.rtt1));
    setItem(4, rttText(hop.rtt2));
    setItem(5, rttText(hop.rtt3));
    setItem(6, (hop.avgRtt >= 0) ? QString::number(hop.avgRtt, 'f', 2) : "*");
    setItem(7, QString::number(hop.lossPercent, 'f', 1) + "%");

    m_resultsTable->scrollToBottom();
}

// ── Statistics ───────────────────────────────────────────────────────────────

void TracerouteWidget::updateStatistics()
{
    if (m_hops.isEmpty()) {
        return;
    }

    m_statTotalHops->setText(QString::number(m_hops.size()));

    double totalRtt = 0.0;
    int rttCount = 0;
    int totalQueries = 0;
    int lostQueries = 0;

    for (const auto& hop : m_hops) {
        totalQueries += m_queriesSpin->value();
        if (hop.rtt1 >= 0) { totalRtt += hop.rtt1; rttCount++; } else { lostQueries++; }
        if (hop.rtt2 >= 0) { totalRtt += hop.rtt2; rttCount++; } else { lostQueries++; }
        if (hop.rtt3 >= 0) { totalRtt += hop.rtt3; rttCount++; } else { lostQueries++; }
    }

    if (rttCount > 0) {
        m_statAvgRtt->setText(QString::number(totalRtt / rttCount, 'f', 2) + " ms");
    } else {
        m_statAvgRtt->setText("N/A");
    }

    if (totalQueries > 0) {
        double totalLoss = (static_cast<double>(lostQueries) / totalQueries) * 100.0;
        m_statPacketLoss->setText(QString::number(totalLoss, 'f', 1) + "%");
    }
}

// ── Hop detail ───────────────────────────────────────────────────────────────

void TracerouteWidget::onTableItemSelected(int row, int /*column*/)
{
    if (row < 0 || row >= m_hops.size()) {
        m_hopDetailLabel->setText("选择上方跳点查看详情");
        return;
    }

    const auto& hop = m_hops[row];
    QString detail;
    detail += QString("跳数: %1\n").arg(hop.hop);
    detail += QString("IP 地址: %1\n").arg(hop.ip.isEmpty() ? "超时" : hop.ip);
    detail += QString("主机名: %1\n").arg(hop.hostname.isEmpty() ? "N/A" : hop.hostname);
    detail += QString("平均 RTT: %1 ms\n").arg(hop.avgRtt >= 0 ? QString::number(hop.avgRtt, 'f', 2) : "N/A");
    detail += QString("丢包率: %1%\n").arg(QString::number(hop.lossPercent, 'f', 1));
    detail += QString("AS 号: %1\n").arg(hop.asNumber.isEmpty() ? "N/A" : hop.asNumber);
    detail += QString("位置: %1").arg(hop.location.isEmpty() ? "N/A" : hop.location);

    m_hopDetailLabel->setText(detail);
}

// ── History ──────────────────────────────────────────────────────────────────

void TracerouteWidget::ensureHopDetailTable()
{
    const QSqlDatabase& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return;
    }

    QSqlQuery query(db);
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS traceroute_hop_detail ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "trace_id INTEGER,"
            "hop INTEGER,"
            "ip TEXT,"
            "hostname TEXT,"
            "rtt1 REAL,"
            "rtt2 REAL,"
            "rtt3 REAL,"
            "avg_rtt REAL,"
            "loss REAL,"
            "FOREIGN KEY(trace_id) REFERENCES traceroute_history(id)"
            ")")) {
        Logger::instance().error("Traceroute", "Create hop detail table failed: " + query.lastError().text());
    }
}

void TracerouteWidget::saveToHistory()
{
    const QSqlDatabase& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        Logger::instance().warning("Traceroute", "Database not open, skipping history save");
        return;
    }

    QSqlQuery query(db);

    double avgRtt = 0.0;
    int rttCount = 0;
    for (const auto& hop : m_hops) {
        if (hop.rtt1 >= 0) { avgRtt += hop.rtt1; rttCount++; }
        if (hop.rtt2 >= 0) { avgRtt += hop.rtt2; rttCount++; }
        if (hop.rtt3 >= 0) { avgRtt += hop.rtt3; rttCount++; }
    }
    if (rttCount > 0) {
        avgRtt /= rttCount;
    }

    query.prepare(
        "INSERT INTO traceroute_history (target, start_time, finish_time, hop_count, average_rtt) "
        "VALUES (:target, :start, :finish, :hops, :avg)");
    query.bindValue(":target", m_targetHost);
    query.bindValue(":start", m_startTime.toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":finish", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":hops", m_hops.size());
    query.bindValue(":avg", avgRtt);

    if (!query.exec()) {
        Logger::instance().error("Traceroute", "Save history failed: " + query.lastError().text());
        return;
    }

    int traceId = query.lastInsertId().toInt();

    // Save hop details
    QSqlQuery hopQuery(db);
    for (const auto& hop : m_hops) {
        hopQuery.prepare(
            "INSERT INTO traceroute_hop_detail (trace_id, hop, ip, hostname, rtt1, rtt2, rtt3, avg_rtt, loss) "
            "VALUES (:tid, :hop, :ip, :host, :r1, :r2, :r3, :avg, :loss)");
        hopQuery.bindValue(":tid", traceId);
        hopQuery.bindValue(":hop", hop.hop);
        hopQuery.bindValue(":ip", hop.ip);
        hopQuery.bindValue(":host", hop.hostname);
        hopQuery.bindValue(":r1", hop.rtt1 >= 0 ? hop.rtt1 : QVariant());
        hopQuery.bindValue(":r2", hop.rtt2 >= 0 ? hop.rtt2 : QVariant());
        hopQuery.bindValue(":r3", hop.rtt3 >= 0 ? hop.rtt3 : QVariant());
        hopQuery.bindValue(":avg", hop.avgRtt >= 0 ? hop.avgRtt : QVariant());
        hopQuery.bindValue(":loss", hop.lossPercent);
        hopQuery.exec();
    }

    Logger::instance().info("Traceroute",
        QString("History saved: trace_id=%1, target=%2, hops=%3")
            .arg(traceId)
            .arg(m_targetHost)
            .arg(m_hops.size()));
}

void TracerouteWidget::loadHistory()
{
    // History loading would be triggered by selecting a previous trace.
    // The data is stored and available for future features.
}

// ── Export ───────────────────────────────────────────────────────────────────

void TracerouteWidget::onExportCSV()
{
    if (m_resultsTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "没有可导出的数据。");
        return;
    }

    QString filename = QString("traceroute_%1_%2.csv")
                           .arg(m_targetHost)
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", filename, "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法写入文件: " + filePath);
        Logger::instance().error("Traceroute", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);

    // Header
    QStringList headers;
    for (int col = 0; col < m_resultsTable->columnCount(); ++col) {
        headers << m_resultsTable->horizontalHeaderItem(col)->text();
    }
    stream << headers.join(',') << "\n";

    // Data
    for (int row = 0; row < m_resultsTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_resultsTable->columnCount(); ++col) {
            QTableWidgetItem* item = m_resultsTable->item(row, col);
            QString cell = item ? item->text() : "";
            // Escape CSV: wrap in quotes if contains comma or quote
            if (cell.contains(',') || cell.contains('"')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("Traceroute", "CSV exported to: " + filePath);
    QMessageBox::information(this, "导出成功", "已成功导出到:\n" + filePath);
}