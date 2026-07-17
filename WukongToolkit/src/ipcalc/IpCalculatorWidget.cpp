#include "ipcalc/IpCalculatorWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <QHostAddress>
#include <QSplitter>
#include <QDateTime>

// ═══════════════════════════════════════════════════════════════════════════
// Helper: convert quint32 to dotted-decimal string
// ═══════════════════════════════════════════════════════════════════════════

static QString ipFromUint32(quint32 addr)
{
    return QString("%1.%2.%3.%4")
        .arg((addr >> 24) & 0xFF)
        .arg((addr >> 16) & 0xFF)
        .arg((addr >> 8) & 0xFF)
        .arg(addr & 0xFF);
}

static quint32 ipToUint32(const QString& ip)
{
    QHostAddress addr(ip);
    return addr.toIPv4Address();
}

static quint32 maskFromPrefix(int prefix)
{
    if (prefix == 0) return 0;
    return 0xFFFFFFFF << (32 - prefix);
}

static QString binaryString(quint32 addr)
{
    QString bin;
    for (int i = 31; i >= 0; --i) {
        bin += ((addr >> i) & 1) ? '1' : '0';
        if (i % 8 == 0 && i > 0) bin += '.';
    }
    return bin;
}

// ═══════════════════════════════════════════════════════════════════════════
// IpCalculatorWidget
// ═══════════════════════════════════════════════════════════════════════════

IpCalculatorWidget::IpCalculatorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void IpCalculatorWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Top: left input + right result ──
    auto* topSplitter = new QSplitter(Qt::Horizontal);

    // ── Left: input area ──
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // Single IP calculation
    auto* singleGroup = new QGroupBox("单IP计算");
    auto* singleLayout = new QVBoxLayout(singleGroup);
    singleLayout->setSpacing(8);

    auto* ipForm = new QFormLayout();
    ipForm->setSpacing(6);

    m_ipEdit = new QLineEdit();
    m_ipEdit->setPlaceholderText("例如: 192.168.1.0");
    m_ipEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    ipForm->addRow("IP 地址:", m_ipEdit);

    m_prefixSpin = new QSpinBox();
    m_prefixSpin->setRange(0, 32);
    m_prefixSpin->setValue(24);
    m_prefixSpin->setSuffix(" /");
    m_prefixSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    ipForm->addRow("前缀长度:", m_prefixSpin);

    singleLayout->addLayout(ipForm);

    m_calcBtn = new QPushButton("计算");
    m_calcBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
    );
    singleLayout->addWidget(m_calcBtn);

    leftLayout->addWidget(singleGroup);

    // Batch calculation
    auto* batchGroup = new QGroupBox("批量计算 (CIDR格式)");
    auto* batchLayout = new QVBoxLayout(batchGroup);
    batchLayout->setSpacing(8);

    m_batchEdit = new QPlainTextEdit();
    m_batchEdit->setPlaceholderText("每行一个CIDR, 例如:\n192.168.1.0/24\n10.0.0.0/8\n172.16.0.0/16");
    m_batchEdit->setMaximumHeight(120);
    m_batchEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    batchLayout->addWidget(m_batchEdit);

    m_batchBtn = new QPushButton("批量计算");
    m_batchBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #E6A23C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #EBB563; }"
    );
    batchLayout->addWidget(m_batchBtn);

    leftLayout->addWidget(batchGroup);
    leftLayout->addStretch();

    topSplitter->addWidget(leftWidget);

    // ── Right: result area ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* resultGroup = new QGroupBox("计算结果");
    auto* resultForm = new QFormLayout(resultGroup);
    resultForm->setSpacing(6);

    auto makeReadOnlyEdit = [this]() -> QLineEdit* {
        auto* edit = new QLineEdit();
        edit->setReadOnly(true);
        edit->setStyleSheet(
            "QLineEdit {"
            "  background: #1E1F22; color: #409EFF;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px; font-weight: bold;"
            "}"
        );
        return edit;
    };

    m_networkEdit = makeReadOnlyEdit();
    resultForm->addRow("网络地址:", m_networkEdit);

    m_broadcastEdit = makeReadOnlyEdit();
    resultForm->addRow("广播地址:", m_broadcastEdit);

    m_rangeEdit = makeReadOnlyEdit();
    resultForm->addRow("可用范围:", m_rangeEdit);

    m_hostCountEdit = makeReadOnlyEdit();
    resultForm->addRow("主机数:", m_hostCountEdit);

    m_maskEdit = makeReadOnlyEdit();
    resultForm->addRow("子网掩码:", m_maskEdit);

    m_wildcardEdit = makeReadOnlyEdit();
    resultForm->addRow("Wildcard Mask:", m_wildcardEdit);

    rightLayout->addWidget(resultGroup);

    // Binary view
    auto* binaryGroup = new QGroupBox("二进制视图");
    auto* binaryLayout = new QVBoxLayout(binaryGroup);
    binaryLayout->setContentsMargins(4, 4, 4, 4);

    m_binaryEdit = new QPlainTextEdit();
    m_binaryEdit->setReadOnly(true);
    m_binaryEdit->setMaximumHeight(140);
    m_binaryEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #1E1F22; color: #DCDCDC;"
        "  border: none; font-size: 12px; font-family: monospace;"
        "}"
    );
    binaryLayout->addWidget(m_binaryEdit);

    rightLayout->addWidget(binaryGroup);
    rightLayout->addStretch();

    topSplitter->addWidget(rightWidget);
    topSplitter->setStretchFactor(0, 1);
    topSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(topSplitter, 1);

    // ── Bottom: history table ──
    auto* historyGroup = new QGroupBox("历史记录");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(6);

    m_historyTable = new QTableWidget(0, 9);
    m_historyTable->setHorizontalHeaderLabels({
        "IP/CIDR", "网络地址", "广播地址", "可用范围",
        "主机数", "子网掩码", "Wildcard", "前缀", "二进制"
    });
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setStyleSheet(
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
    );
    historyLayout->addWidget(m_historyTable);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    historyLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(historyGroup);
}

// ─── Connections ───────────────────────────────────────────────────────

void IpCalculatorWidget::setupConnections()
{
    connect(m_calcBtn, &QPushButton::clicked, this, &IpCalculatorWidget::onCalculate);
    connect(m_batchBtn, &QPushButton::clicked, this, &IpCalculatorWidget::onBatchCalculate);
    connect(m_exportBtn, &QPushButton::clicked, this, &IpCalculatorWidget::onExport);
    // Allow Enter key in IP edit to trigger calculation
    connect(m_ipEdit, &QLineEdit::returnPressed, this, &IpCalculatorWidget::onCalculate);
}

// ─── IP Calculation ────────────────────────────────────────────────────

void IpCalculatorWidget::onCalculate()
{
    QString ipText = m_ipEdit->text().trimmed();
    if (ipText.isEmpty()) {
        Logger::instance().warning("IPCalc", "IP address is empty");
        return;
    }

    // Validate IP format
    static QRegularExpression ipRe(
        R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)"
    );
    QRegularExpressionMatch match = ipRe.match(ipText);
    if (!match.hasMatch()) {
        Logger::instance().warning("IPCalc", "Invalid IP format: " + ipText);
        QMessageBox::warning(this, "输入错误", "请输入有效的 IPv4 地址。");
        return;
    }

    for (int i = 1; i <= 4; ++i) {
        int octet = match.captured(i).toInt();
        if (octet < 0 || octet > 255) {
            Logger::instance().warning("IPCalc", "Invalid IP octet: " + ipText);
            QMessageBox::warning(this, "输入错误", "IP 地址每个字段必须在 0-255 之间。");
            return;
        }
    }

    int prefix = m_prefixSpin->value();
    quint32 ip = ipToUint32(ipText);
    quint32 mask = maskFromPrefix(prefix);
    quint32 network = ip & mask;
    quint32 broadcast = network | (~mask);
    quint32 wildcard = ~mask;

    // Subnet mask string
    QString maskStr = ipFromUint32(mask);
    QString wildcardStr = ipFromUint32(wildcard);
    QString networkStr = ipFromUint32(network);
    QString broadcastStr = ipFromUint32(broadcast);

    // Host count and range
    quint32 hostCount = 0;
    QString firstHost = "-";
    QString lastHost = "-";
    QString rangeStr = "-";

    if (prefix == 32) {
        hostCount = 1;
        firstHost = ipText;
        lastHost = ipText;
        rangeStr = ipText;
    } else if (prefix == 31) {
        hostCount = 2;
        firstHost = ipFromUint32(network);
        lastHost = ipFromUint32(broadcast);
        rangeStr = QString("%1 - %2").arg(firstHost, lastHost);
    } else {
        hostCount = (1u << (32 - prefix)) - 2;
        firstHost = ipFromUint32(network + 1);
        lastHost = ipFromUint32(broadcast - 1);
        rangeStr = QString("%1 - %2").arg(firstHost, lastHost);
    }

    // Binary representation
    QString binary = QString("IP:       %1\n").arg(binaryString(ip));
    binary += QString("掩码:     %1\n").arg(binaryString(mask));
    binary += QString("网络:     %1\n").arg(binaryString(network));
    binary += QString("广播:     %1\n").arg(binaryString(broadcast));
    binary += QString("Wildcard: %1").arg(binaryString(wildcard));

    // Update result fields
    m_networkEdit->setText(networkStr);
    m_broadcastEdit->setText(broadcastStr);
    m_rangeEdit->setText(rangeStr);
    m_hostCountEdit->setText(QString::number(hostCount));
    m_maskEdit->setText(maskStr);
    m_wildcardEdit->setText(wildcardStr);
    m_binaryEdit->setPlainText(binary);

    // Add to history
    addHistoryEntry(ipText, prefix);

    Logger::instance().info("IPCalc", QString("Calculated %1/%2 -> network=%3 broadcast=%4 hosts=%5")
        .arg(ipText).arg(prefix).arg(networkStr, broadcastStr).arg(hostCount));
}

// ─── Batch Calculation ─────────────────────────────────────────────────

void IpCalculatorWidget::onBatchCalculate()
{
    QString text = m_batchEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        Logger::instance().warning("IPCalc", "Batch input is empty");
        return;
    }

    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    int successCount = 0;
    int failCount = 0;

    // CIDR regex: optional IP with optional /prefix
    static QRegularExpression cidrRe(
        R"(^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})\s*/\s*(\d{1,2})?\s*$)"
    );

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QRegularExpressionMatch match = cidrRe.match(trimmed);
        if (!match.hasMatch()) {
            failCount++;
            Logger::instance().warning("IPCalc", "Invalid CIDR line: " + trimmed);
            continue;
        }

        QString ip = match.captured(1);
        int prefix = match.captured(2).isEmpty() ? 32 : match.captured(2).toInt();

        // Validate octets
        QStringList octets = ip.split('.');
        bool valid = true;
        for (const QString& oct : octets) {
            int val = oct.toInt();
            if (val < 0 || val > 255) {
                valid = false;
                break;
            }
        }
        if (!valid || prefix < 0 || prefix > 32) {
            failCount++;
            Logger::instance().warning("IPCalc", "Invalid CIDR: " + trimmed);
            continue;
        }

        addHistoryEntry(ip, prefix);
        successCount++;
    }

    Logger::instance().info("IPCalc",
        QString("Batch calculation: %1 success, %2 failed").arg(successCount).arg(failCount));

    if (failCount > 0) {
        QMessageBox::information(this, "批量计算完成",
            QString("成功: %1 条\n失败: %2 条").arg(successCount).arg(failCount));
    }
}

// ─── Add History Entry ─────────────────────────────────────────────────

void IpCalculatorWidget::addHistoryEntry(const QString& ip, int prefix)
{
    quint32 ipInt = ipToUint32(ip);
    quint32 mask = maskFromPrefix(prefix);
    quint32 network = ipInt & mask;
    quint32 broadcast = network | (~mask);
    quint32 wildcard = ~mask;

    QString networkStr = ipFromUint32(network);
    QString broadcastStr = ipFromUint32(broadcast);
    QString maskStr = ipFromUint32(mask);
    QString wildcardStr = ipFromUint32(wildcard);
    QString cidr = QString("%1/%2").arg(ip, QString::number(prefix));

    // Host count and range
    quint32 hostCount = 0;
    QString rangeStr = "-";
    if (prefix == 32) {
        hostCount = 1;
        rangeStr = ip;
    } else if (prefix == 31) {
        hostCount = 2;
        rangeStr = QString("%1 - %2").arg(ipFromUint32(network), ipFromUint32(broadcast));
    } else {
        hostCount = (1u << (32 - prefix)) - 2;
        rangeStr = QString("%1 - %2").arg(ipFromUint32(network + 1), ipFromUint32(broadcast - 1));
    }

    QString binary = binaryString(ipInt) + "\n" + binaryString(mask);

    // Store in data
    IPv4Info info;
    info.ip = ip;
    info.network = networkStr;
    info.broadcast = broadcastStr;
    info.subnetMask = maskStr;
    info.wildcard = wildcardStr;
    info.prefix = prefix;
    info.hostCount = hostCount;
    info.firstHost = (prefix == 32) ? ip : ipFromUint32(network + 1);
    info.lastHost = (prefix == 32) ? ip : ipFromUint32(broadcast - 1);
    info.binary = binary;
    m_history.append(info);

    // Add to table
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    m_historyTable->setItem(row, 0, new QTableWidgetItem(cidr));
    m_historyTable->setItem(row, 1, new QTableWidgetItem(networkStr));
    m_historyTable->setItem(row, 2, new QTableWidgetItem(broadcastStr));
    m_historyTable->setItem(row, 3, new QTableWidgetItem(rangeStr));
    m_historyTable->setItem(row, 4, new QTableWidgetItem(QString::number(hostCount)));
    m_historyTable->setItem(row, 5, new QTableWidgetItem(maskStr));
    m_historyTable->setItem(row, 6, new QTableWidgetItem(wildcardStr));
    m_historyTable->setItem(row, 7, new QTableWidgetItem(QString("/%1").arg(prefix)));
    m_historyTable->setItem(row, 8, new QTableWidgetItem(binary));

    m_historyTable->scrollToBottom();
}

// ─── Export CSV ─────────────────────────────────────────────────────────

void IpCalculatorWidget::onExport()
{
    if (m_history.isEmpty()) {
        Logger::instance().warning("IPCalc", "No history to export");
        QMessageBox::information(this, "导出", "没有可导出的历史记录。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 IP 计算结果",
        QString("ipcalc_results_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().error("IPCalc", "Failed to open file: " + filePath);
        QMessageBox::critical(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    // BOM for Excel compatibility
    stream << "\xEF\xBB\xBF";
    stream << "IP/CIDR,网络地址,广播地址,可用范围,主机数,子网掩码,Wildcard,前缀\n";

    for (const auto& info : m_history) {
        stream << QString("%1/%2").arg(info.ip).arg(info.prefix) << ","
               << info.network << ","
               << info.broadcast << ","
               << QString("%1 - %2").arg(info.firstHost, info.lastHost) << ","
               << info.hostCount << ","
               << info.subnetMask << ","
               << info.wildcard << ","
               << QString("/%1").arg(info.prefix) << "\n";
    }

    file.close();
    Logger::instance().info("IPCalc", "Results exported to: " + filePath);
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(m_history.size()).arg(filePath));
}