#include "dns/DnsToolkitWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QPlainTextEdit>
#include <QTimer>
#include <QSplitter>
#include <QLabel>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFile>
#include <QColor>
#include <QApplication>

// ============================================================================
// DnsToolkitWidget 实现
// ============================================================================

DnsToolkitWidget::DnsToolkitWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isBatchQuery(false)
    , m_batchIndex(0)
    , m_isCompareQuery(false)
    , m_compareIndex(0)
{
    setupUI();
    setupConnections();
}

DnsToolkitWidget::~DnsToolkitWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void DnsToolkitWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 左侧: 输入区 ──
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 查询配置
    auto* queryGroup = new QGroupBox("DNS 查询");
    auto* queryLayout = new QVBoxLayout(queryGroup);
    queryLayout->setSpacing(6);

    // 域名
    auto* domainLabel = new QLabel("域名:");
    
    m_domainEdit = new QLineEdit();
    m_domainEdit->setPlaceholderText("例如: example.com");
    m_domainEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    queryLayout->addWidget(domainLabel);
    queryLayout->addWidget(m_domainEdit);

    // 记录类型
    auto* typeLabel = new QLabel("记录类型:");
    
    m_recordTypeCombo = new QComboBox();
    m_recordTypeCombo->addItems({"A", "AAAA", "CNAME", "MX", "TXT", "NS", "PTR", "SOA", "SRV", "CAA", "ANY"});
    m_recordTypeCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    queryLayout->addWidget(typeLabel);
    queryLayout->addWidget(m_recordTypeCombo);

    // DNS 服务器
    auto* serverLabel = new QLabel("DNS 服务器:");
    
    m_dnsServerCombo = new QComboBox();
    m_dnsServerCombo->addItems({"8.8.8.8", "114.114.114.114", "223.5.5.5", "自定义"});
    m_dnsServerCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    queryLayout->addWidget(serverLabel);
    queryLayout->addWidget(m_dnsServerCombo);

    // 自定义服务器
    auto* customLabel = new QLabel("自定义服务器:");
    
    m_customServerEdit = new QLineEdit();
    m_customServerEdit->setPlaceholderText("例如: 1.1.1.1");
    m_customServerEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    m_customServerEdit->setEnabled(false);
    queryLayout->addWidget(customLabel);
    queryLayout->addWidget(m_customServerEdit);

    // 查询按钮
    m_queryBtn = new QPushButton("查询");
    m_queryBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_queryBtn->setFixedHeight(32);
    queryLayout->addWidget(m_queryBtn);

    leftLayout->addWidget(queryGroup);

    // 批量查询
    auto* batchGroup = new QGroupBox("批量查询");
    auto* batchLayout = new QVBoxLayout(batchGroup);
    batchLayout->setSpacing(6);

    auto* batchLabel = new QLabel("每行输入一个域名:");
    
    m_batchEdit = new QPlainTextEdit();
    m_batchEdit->setPlaceholderText("example.com\ngoogle.com\ngithub.com");
    m_batchEdit->setMaximumHeight(100);
    m_batchEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    batchLayout->addWidget(batchLabel);
    batchLayout->addWidget(m_batchEdit);

    m_batchBtn = new QPushButton("批量查询");
    m_batchBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_batchBtn->setFixedHeight(32);
    batchLayout->addWidget(m_batchBtn);

    leftLayout->addWidget(batchGroup);

    // 多 DNS 对比
    auto* compareGroup = new QGroupBox("多 DNS 对比");
    auto* compareLayout = new QVBoxLayout(compareGroup);
    compareLayout->setSpacing(6);

    auto* compareLabel = new QLabel("同时向多个 DNS 服务器查询同一域名，对比结果");
    
    compareLabel->setWordWrap(true);
    m_compareBtn = new QPushButton("多 DNS 对比");
    m_compareBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #D29922; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #DBAB4A; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_compareBtn->setFixedHeight(32);
    compareLayout->addWidget(compareLabel);
    compareLayout->addWidget(m_compareBtn);

    leftLayout->addWidget(compareGroup);
    leftLayout->addStretch();

    // ── 中间: 结果区 ──
    auto* centerWidget = new QWidget();
    auto* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(4);

    auto* resultTitle = new QLabel("查询结果");
    resultTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #E6EDF3; padding: 2px 0;");

    auto* resultHeaderRow = new QHBoxLayout();
    resultHeaderRow->addWidget(resultTitle);
    resultHeaderRow->addStretch();

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    resultHeaderRow->addWidget(m_exportBtn);

    centerLayout->addLayout(resultHeaderRow);

    // 结果表
    m_resultTable = new QTableWidget(0, 6);
    m_resultTable->setHorizontalHeaderLabels({"域名", "类型", "结果", "TTL", "服务器", "响应时间(ms)"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->verticalHeader()->setVisible(false);
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
    centerLayout->addWidget(m_resultTable, 1);

    // 响应详情
    auto* detailLabel = new QLabel("响应详情");
    detailLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #E6EDF3; padding: 2px 0;");
    m_detailEdit = new QPlainTextEdit();
    m_detailEdit->setReadOnly(true);
    m_detailEdit->setMaximumHeight(150);
    m_detailEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px;"
        "  border-radius: 3px; font-size: 12px; font-family: 'Menlo', 'Consolas', monospace;"
        "}"
    );
    centerLayout->addWidget(detailLabel);
    centerLayout->addWidget(m_detailEdit);

    // ── 右侧: 历史区 ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    auto* historyTitle = new QLabel("查询历史");
    historyTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #E6EDF3; padding: 2px 0;");
    rightLayout->addWidget(historyTitle);

    m_historyTable = new QTableWidget(0, 4);
    m_historyTable->setHorizontalHeaderLabels({"时间", "域名", "类型", "状态"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setStyleSheet(
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
    rightLayout->addWidget(m_historyTable, 1);

    // 组合布局
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addWidget(centerWidget, 3);
    mainLayout->addWidget(rightWidget, 1);
}

// ─── Connections ───────────────────────────────────────────────────────

void DnsToolkitWidget::setupConnections()
{
    connect(m_queryBtn, &QPushButton::clicked, this, &DnsToolkitWidget::onQuery);
    connect(m_batchBtn, &QPushButton::clicked, this, &DnsToolkitWidget::onBatchQuery);
    connect(m_compareBtn, &QPushButton::clicked, this, &DnsToolkitWidget::onCompareDns);
    connect(m_exportBtn, &QPushButton::clicked, this, &DnsToolkitWidget::onExport);
    connect(m_dnsServerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DnsToolkitWidget::onServerChanged);
    connect(m_domainEdit, &QLineEdit::returnPressed, this, &DnsToolkitWidget::onQuery);
}

// ─── 服务器切换 ────────────────────────────────────────────────────────

void DnsToolkitWidget::onServerChanged(int index)
{
    m_customServerEdit->setEnabled(index == 3);
}

// ─── 获取当前 DNS 服务器 ──────────────────────────────────────────────

static QString currentServer(QComboBox* combo, QLineEdit* customEdit)
{
    if (combo->currentIndex() == 3) {
        QString custom = customEdit->text().trimmed();
        return custom.isEmpty() ? "8.8.8.8" : custom;
    }
    return combo->currentText();
}

// ─── 单次查询 ─────────────────────────────────────────────────────────

void DnsToolkitWidget::onQuery()
{
    QString domain = m_domainEdit->text().trimmed();
    if (domain.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入要查询的域名。");
        return;
    }

    m_isBatchQuery = false;
    m_isCompareQuery = false;
    m_currentDomain = domain;
    m_currentType = m_recordTypeCombo->currentText();
    m_currentServer = currentServer(m_dnsServerCombo, m_customServerEdit);

    m_resultTable->setRowCount(0);
    m_detailEdit->clear();

    setQueryButtonsEnabled(false);
    runDigQuery(domain, m_currentType, m_currentServer);
}

// ─── 批量查询 ─────────────────────────────────────────────────────────

void DnsToolkitWidget::onBatchQuery()
{
    QString text = m_batchEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请在批量查询区域输入域名，每行一个。");
        return;
    }

    QStringList domains = text.split('\n', Qt::SkipEmptyParts);
    QStringList cleaned;
    for (const QString& d : domains) {
        QString domain = d.trimmed();
        if (!domain.isEmpty()) {
            cleaned.append(domain);
        }
    }

    if (cleaned.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "未找到有效的域名。");
        return;
    }

    m_isBatchQuery = true;
    m_isCompareQuery = false;
    m_batchDomains = cleaned;
    m_batchIndex = 0;
    m_currentType = m_recordTypeCombo->currentText();
    m_currentServer = currentServer(m_dnsServerCombo, m_customServerEdit);

    m_resultTable->setRowCount(0);
    m_detailEdit->clear();
    setQueryButtonsEnabled(false);

    Logger::instance().info("DNS-Toolkit", QString("开始批量查询 %1 个域名").arg(cleaned.size()));

    m_currentDomain = m_batchDomains[m_batchIndex];
    m_batchIndex++;
    runDigQuery(m_currentDomain, m_currentType, m_currentServer);
}

// ─── 多 DNS 对比 ──────────────────────────────────────────────────────

void DnsToolkitWidget::onCompareDns()
{
    QString domain = m_domainEdit->text().trimmed();
    if (domain.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入要查询的域名。");
        return;
    }

    m_isBatchQuery = false;
    m_isCompareQuery = true;
    m_compareDomain = domain;
    m_compareType = m_recordTypeCombo->currentText();
    m_compareServers = {"8.8.8.8", "114.114.114.114", "223.5.5.5"};
    m_compareIndex = 0;

    m_resultTable->setRowCount(0);
    m_detailEdit->clear();
    setQueryButtonsEnabled(false);

    Logger::instance().info("DNS-Toolkit", QString("开始多 DNS 对比: %1").arg(domain));

    m_currentDomain = domain;
    m_currentType = m_compareType;
    m_currentServer = m_compareServers[m_compareIndex];
    m_compareIndex++;
    runDigQuery(domain, m_compareType, m_currentServer);
}

// ─── 运行 dig 查询 ────────────────────────────────────────────────────

void DnsToolkitWidget::runDigQuery(const QString& domain, const QString& type, const QString& server)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, domain, type, server](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)

        qint64 elapsed = m_timer.isValid() ? m_timer.elapsed() : -1;

        QString output;
        QString errOutput;
        if (m_process) {
            output = QString::fromUtf8(m_process->readAllStandardOutput());
            errOutput = QString::fromUtf8(m_process->readAllStandardError());
            m_process->deleteLater();
            m_process = nullptr;
        }

        if (!errOutput.isEmpty() && exitCode != 0) {
            Logger::instance().error("DNS-Toolkit", "dig 错误: " + errOutput.trimmed());
            addResultRow(domain, type, "查询错误: " + errOutput.trimmed(), 0, server, elapsed >= 0 ? elapsed : 0);
            addHistoryEntry(domain, type, "失败");
        } else {
            parseDigResult(domain, type, server, output, elapsed);
            addHistoryEntry(domain, type, "成功");
        }

        m_detailEdit->appendPlainText(QString("--- 服务器: %1 (%2 ms) ---\n%3\n")
                                          .arg(server)
                                          .arg(elapsed >= 0 ? QString::number(elapsed) : "-")
                                          .arg(output.isEmpty() ? "(无结果)" : output.trimmed()));

        // 继续批量查询
        if (m_isBatchQuery && m_batchIndex < m_batchDomains.size()) {
            m_currentDomain = m_batchDomains[m_batchIndex];
            m_batchIndex++;
            runDigQuery(m_currentDomain, m_currentType, m_currentServer);
            return;
        }

        // 继续对比查询
        if (m_isCompareQuery && m_compareIndex < m_compareServers.size()) {
            m_currentDomain = m_compareDomain;
            m_currentType = m_compareType;
            m_currentServer = m_compareServers[m_compareIndex];
            m_compareIndex++;
            runDigQuery(m_compareDomain, m_compareType, m_currentServer);
            return;
        }

        // 所有查询完成
        if (m_isBatchQuery) {
            Logger::instance().info("DNS-Toolkit", QString("批量查询完成，共 %1 个域名").arg(m_batchDomains.size()));
            m_isBatchQuery = false;
        }
        if (m_isCompareQuery) {
            Logger::instance().info("DNS-Toolkit", "多 DNS 对比完成");
            m_isCompareQuery = false;
        }

        setQueryButtonsEnabled(true);
    });

    QStringList args;
    args << "@" + server << domain;
    if (type != "ANY") {
        args << type;
    }
    args << "+short" << "+nocmd" << "+noquestion" << "+noadditional" << "+noauthority";

    m_timer.start();
    m_process->start("dig", args);

    Logger::instance().info("DNS-Toolkit", QString("执行 dig %1").arg(args.join(' ')));
}

// ─── 解析 dig 结果 ────────────────────────────────────────────────────

void DnsToolkitWidget::parseDigResult(const QString& domain, const QString& type, const QString& server,
                                       const QString& output, qint64 elapsedMs)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        addResultRow(domain, type, "未找到记录", 0, server, elapsedMs);
        return;
    }

    bool hasRecords = false;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        if (type == "SOA") {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"));
            if (parts.size() >= 7) {
                addResultRow(domain, "SOA",
                             QString("主NS: %1, 管理员: %2, 序列号: %3, 刷新: %4, 重试: %5, 过期: %6, 最小TTL: %7")
                                 .arg(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6]),
                             0, server, elapsedMs);
                hasRecords = true;
            }
        } else if (type == "MX") {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"));
            if (parts.size() >= 2) {
                addResultRow(domain, "MX",
                             QString("优先级: %1, 邮件服务器: %2").arg(parts[0], parts[1]),
                             0, server, elapsedMs);
                hasRecords = true;
            }
        } else if (type == "SRV") {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"));
            if (parts.size() >= 4) {
                addResultRow(domain, "SRV",
                             QString("优先级: %1, 权重: %2, 端口: %3, 目标: %4")
                                 .arg(parts[0], parts[1], parts[2], parts[3]),
                             0, server, elapsedMs);
                hasRecords = true;
            }
        } else {
            addResultRow(domain, type, trimmed, 0, server, elapsedMs);
            hasRecords = true;
        }
    }

    if (!hasRecords) {
        addResultRow(domain, type, "未找到记录", 0, server, elapsedMs);
    }
}

// ─── 结果表格 ─────────────────────────────────────────────────────────

void DnsToolkitWidget::addResultRow(const QString& domain, const QString& type, const QString& result,
                                     int ttl, const QString& server, double responseTimeMs)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0, new QTableWidgetItem(domain));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(type));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(result));

    auto* ttlItem = new QTableWidgetItem(ttl > 0 ? QString::number(ttl) : "-");
    ttlItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 3, ttlItem);

    m_resultTable->setItem(row, 4, new QTableWidgetItem(server));

    auto* timeItem = new QTableWidgetItem(responseTimeMs >= 0 ? QString::number(responseTimeMs, 'f', 1) : "-");
    timeItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 5, timeItem);

    m_resultTable->scrollToBottom();
}

// ─── 历史记录 ─────────────────────────────────────────────────────────

void DnsToolkitWidget::addHistoryEntry(const QString& domain, const QString& type, const QString& status)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(0);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_historyTable->setItem(0, 0, new QTableWidgetItem(timeStr));
    m_historyTable->setItem(0, 1, new QTableWidgetItem(domain));
    m_historyTable->setItem(0, 2, new QTableWidgetItem(type));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "成功") {
        statusItem->setForeground(QColor(0x3F, 0xB9, 0x50));
    } else {
        statusItem->setForeground(QColor(0xF8, 0x51, 0x49));
    }
    statusItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(0, 3, statusItem);

    // 限制历史记录数量
    while (m_historyTable->rowCount() > 200) {
        m_historyTable->removeRow(m_historyTable->rowCount() - 1);
    }
}

// ─── 核心解析函数 (同步执行) ──────────────────────────────────────────

QString DnsToolkitWidget::resolveDNS(const QString& domain, const QString& type, const QString& server)
{
    QProcess proc;
    QStringList args;
    args << "@" + server << domain;
    if (type != "ANY") {
        args << type;
    }
    args << "+short" << "+nocmd" << "+noquestion" << "+noadditional" << "+noauthority";

    proc.start("dig", args);
    if (!proc.waitForFinished(5000)) {
        Logger::instance().error("DNS-Toolkit", "dig 超时: " + domain);
        return QString();
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    Logger::instance().info("DNS-Toolkit", QString("resolveDNS: %1 (%2) @%3 -> %4")
                                                .arg(domain, type, server, output.left(100)));
    return output;
}

// ─── 导出 CSV ─────────────────────────────────────────────────────────

void DnsToolkitWidget::onExport()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 DNS 结果",
        QString("dns_toolkit_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV 文件 (*.csv)"
    );

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
    out << "域名,类型,结果,TTL,服务器,响应时间(ms)\n";

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* item = m_resultTable->item(row, col);
            QString val = item ? item->text() : "-";
            if (val.contains(',') || val.contains('\n') || val.contains('"')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            cols << val;
        }
        out << cols.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("DNS-Toolkit",
                            QString("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(m_resultTable->rowCount()));
    QMessageBox::information(this, "导出成功",
                             QString("已导出 %1 条记录到:\n%2").arg(m_resultTable->rowCount()).arg(filePath));
}

// ─── 按钮状态控制 ─────────────────────────────────────────────────────

void DnsToolkitWidget::setQueryButtonsEnabled(bool enabled)
{
    m_queryBtn->setEnabled(enabled);
    m_batchBtn->setEnabled(enabled);
    m_compareBtn->setEnabled(enabled);
    m_exportBtn->setEnabled(enabled && m_resultTable->rowCount() > 0);
}