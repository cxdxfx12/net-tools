#include "network/DNSWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QMenu>
#include <QScrollBar>

// ============================================================================
// DNSWidget 实现
// ============================================================================

DNSWidget::DNSWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isBulkQuery(false)
    , m_bulkIndex(0)
    , m_isReverseQuery(false)
{
    setupUI();
    setupConnections();
}

DNSWidget::~DNSWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void DNSWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 顶部: 查询配置 ──
    auto* configGroup = new QGroupBox("DNS 查询");
    auto* configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(8);

    // 正向查询行
    auto* queryRow = new QHBoxLayout();
    queryRow->setSpacing(8);

    auto* domainLabel = new QLabel("域名:");
    domainLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_domainEdit = new QLineEdit();
    m_domainEdit->setPlaceholderText("例如: example.com");
    m_domainEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    queryRow->addWidget(domainLabel);
    queryRow->addWidget(m_domainEdit, 2);

    auto* serverLabel = new QLabel("DNS 服务器:");
    serverLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_serverEdit = new QLineEdit();
    m_serverEdit->setPlaceholderText("默认: 8.8.8.8");
    m_serverEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    queryRow->addWidget(serverLabel);
    queryRow->addWidget(m_serverEdit, 1);

    auto* typeLabel = new QLabel("记录类型:");
    typeLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_recordTypeCombo = new QComboBox();
    m_recordTypeCombo->addItems({"A", "AAAA", "MX", "NS", "CNAME", "TXT", "SOA", "PTR", "SRV"});
    m_recordTypeCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #409EFF;"
        "  border: 1px solid #3C3F41;"
        "}"
    );
    queryRow->addWidget(typeLabel);
    queryRow->addWidget(m_recordTypeCombo, 1);

    m_queryBtn = new QPushButton("查询");
    m_queryBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_queryBtn->setFixedHeight(32);
    queryRow->addWidget(m_queryBtn);

    configLayout->addLayout(queryRow);

    // 反向查询行
    auto* reverseRow = new QHBoxLayout();
    reverseRow->setSpacing(8);

    auto* reverseLabel = new QLabel("反向查询 (IP):");
    reverseLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_reverseEdit = new QLineEdit();
    m_reverseEdit->setPlaceholderText("例如: 8.8.8.8");
    m_reverseEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    reverseRow->addWidget(reverseLabel);
    reverseRow->addWidget(m_reverseEdit, 2);

    m_reverseBtn = new QPushButton("反向查询");
    m_reverseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #E6A23C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #EBB563; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_reverseBtn->setFixedHeight(32);
    reverseRow->addWidget(m_reverseBtn);
    reverseRow->addStretch(2);

    configLayout->addLayout(reverseRow);

    // 批量查询行
    auto* bulkLabel = new QLabel("批量查询 (每行一个域名):");
    bulkLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    configLayout->addWidget(bulkLabel);

    auto* bulkRow = new QHBoxLayout();
    bulkRow->setSpacing(8);

    m_bulkEdit = new QTextEdit();
    m_bulkEdit->setPlaceholderText("example.com\ngoogle.com\ngithub.com");
    m_bulkEdit->setMaximumHeight(80);
    m_bulkEdit->setStyleSheet(
        "QTextEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    bulkRow->addWidget(m_bulkEdit, 1);

    m_bulkBtn = new QPushButton("批量查询");
    m_bulkBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_bulkBtn->setFixedHeight(32);
    m_bulkBtn->setFixedWidth(100);
    bulkRow->addWidget(m_bulkBtn);

    configLayout->addLayout(bulkRow);

    mainLayout->addWidget(configGroup);

    // ── 中间: 结果表 + 历史记录 (左右分栏) ──
    auto* splitter = new QSplitter(Qt::Horizontal);

    // 左侧: 结果区
    auto* resultWidget = new QWidget();
    auto* resultLayout = new QVBoxLayout(resultWidget);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(4);

    // 信息标签
    auto* infoRow = new QHBoxLayout();
    m_serverInfoLabel = new QLabel("服务器: -");
    m_serverInfoLabel->setStyleSheet("font-size: 12px; color: #8C8C8C; padding: 2px 0;");
    m_queryTimeLabel = new QLabel("查询耗时: -");
    m_queryTimeLabel->setStyleSheet("font-size: 12px; color: #8C8C8C; padding: 2px 0;");
    infoRow->addWidget(m_serverInfoLabel);
    infoRow->addWidget(m_queryTimeLabel);
    infoRow->addStretch();

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_exportBtn->setEnabled(false);
    infoRow->addWidget(m_exportBtn);

    resultLayout->addLayout(infoRow);

    // 结果表
    m_resultTable = new QTableWidget(0, 4);
    m_resultTable->setHorizontalHeaderLabels({"名称", "TTL", "类型", "值"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setStyleSheet(
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
    resultLayout->addWidget(m_resultTable, 1);

    // 右键菜单
    connect(m_resultTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem* item = m_resultTable->itemAt(pos);
        if (!item) return;

        int row = item->row();
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* it = m_resultTable->item(row, col);
            cols << (it ? it->text() : "-");
        }

        QMenu menu(this);
        QAction* copyCellAction = menu.addAction("复制单元格");
        QAction* copyRowAction = menu.addAction("复制整行");

        QAction* chosen = menu.exec(m_resultTable->viewport()->mapToGlobal(pos));
        if (chosen == copyCellAction) {
            QApplication::clipboard()->setText(item->text());
            Logger::instance().info("DNS", "已复制单元格: " + item->text());
        } else if (chosen == copyRowAction) {
            QApplication::clipboard()->setText(cols.join('\t'));
            Logger::instance().info("DNS", "已复制整行: " + cols.join(" | "));
        }
    });

    splitter->addWidget(resultWidget);

    // 右侧: 查询历史
    auto* historyWidget = new QWidget();
    auto* historyLayout = new QVBoxLayout(historyWidget);
    historyLayout->setContentsMargins(0, 0, 0, 0);
    historyLayout->setSpacing(4);

    auto* historyTitle = new QLabel("查询历史");
    historyTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #DCDCDC; padding: 2px 0;");
    historyLayout->addWidget(historyTitle);

    m_historyList = new QListWidget();
    m_historyList->setStyleSheet(
        "QListWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item { padding: 5px 8px; border-bottom: 1px solid #3C3F41; }"
        "QListWidget::item:hover { background-color: #2B2D30; }"
        "QListWidget::item:selected { background-color: #409EFF; color: white; }"
    );
    historyLayout->addWidget(m_historyList, 1);

    m_clearHistoryBtn = new QPushButton("清除历史");
    m_clearHistoryBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #F78989; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    historyLayout->addWidget(m_clearHistoryBtn);

    splitter->addWidget(historyWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({600, 200});

    mainLayout->addWidget(splitter, 1);
}

// ─── Connections ───────────────────────────────────────────────────────

void DNSWidget::setupConnections()
{
    connect(m_queryBtn, &QPushButton::clicked, this, &DNSWidget::onQuery);
    connect(m_reverseBtn, &QPushButton::clicked, this, &DNSWidget::onReverseQuery);
    connect(m_bulkBtn, &QPushButton::clicked, this, &DNSWidget::onBulkQuery);
    connect(m_exportBtn, &QPushButton::clicked, this, &DNSWidget::onExportResults);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &DNSWidget::onClearHistory);
    connect(m_historyList, &QListWidget::itemClicked, this, &DNSWidget::onHistoryItemClicked);
    connect(m_domainEdit, &QLineEdit::returnPressed, this, &DNSWidget::onQuery);
    connect(m_reverseEdit, &QLineEdit::returnPressed, this, &DNSWidget::onReverseQuery);
}

// ─── 构建 dig 参数 ────────────────────────────────────────────────────

QStringList DNSWidget::buildDigArgs(const QString& domain, const QString& server, const QString& recordType)
{
    QStringList args;
    if (!server.isEmpty()) {
        args << "@" + server;
    }
    args << domain;
    if (recordType != "ANY") {
        args << recordType;
    }
    args << "+short" << "+nocmd" << "+noquestion" << "+noadditional" << "+noauthority";
    return args;
}

// ─── 运行 dig ─────────────────────────────────────────────────────────

void DNSWidget::runDig(const QStringList& args)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DNSWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DNSWidget::onProcessFinished);

    m_timer.start();
    m_process->start("dig", args);

    Logger::instance().info("DNS", QString("执行 dig %1").arg(args.join(' ')));
}

// ─── 查询控制 ─────────────────────────────────────────────────────────

void DNSWidget::onQuery()
{
    QString domain = m_domainEdit->text().trimmed();
    if (domain.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入要查询的域名。");
        return;
    }

    m_isBulkQuery = false;
    m_isReverseQuery = false;
    m_currentQueryDomain = domain;
    m_currentRecordType = m_recordTypeCombo->currentText();
    m_currentServer = m_serverEdit->text().trimmed().isEmpty() ? "8.8.8.8" : m_serverEdit->text().trimmed();

    clearResults();
    updateInfoLabels(m_currentServer, -1);

    QStringList args = buildDigArgs(domain, m_currentServer, m_currentRecordType);
    runDig(args);

    addToHistory(domain, m_currentServer, m_currentRecordType);
}

void DNSWidget::onReverseQuery()
{
    QString ip = m_reverseEdit->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入要反向查询的 IP 地址。");
        return;
    }

    m_isBulkQuery = false;
    m_isReverseQuery = true;
    m_currentQueryDomain = ip;
    m_currentRecordType = "PTR";
    m_currentServer = m_serverEdit->text().trimmed().isEmpty() ? "8.8.8.8" : m_serverEdit->text().trimmed();

    clearResults();
    updateInfoLabels(m_currentServer, -1);

    // 使用 dig -x 进行反向查询
    QStringList args;
    if (!m_currentServer.isEmpty()) {
        args << "@" + m_currentServer;
    }
    args << "-x" << ip;
    args << "+short" << "+nocmd" << "+noquestion" << "+noadditional" << "+noauthority";

    runDig(args);

    addToHistory("PTR:" + ip, m_currentServer, "PTR");
}

void DNSWidget::onBulkQuery()
{
    QString text = m_bulkEdit->toPlainText().trimmed();
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

    m_isBulkQuery = true;
    m_isReverseQuery = false;
    m_bulkDomains = cleaned;
    m_bulkIndex = 0;
    m_currentRecordType = m_recordTypeCombo->currentText();
    m_currentServer = m_serverEdit->text().trimmed().isEmpty() ? "8.8.8.8" : m_serverEdit->text().trimmed();

    clearResults();
    updateInfoLabels(m_currentServer, -1);

    // 禁用按钮
    m_queryBtn->setEnabled(false);
    m_reverseBtn->setEnabled(false);
    m_bulkBtn->setEnabled(false);

    Logger::instance().info("DNS", QString("开始批量查询 %1 个域名").arg(cleaned.size()));

    // 启动第一个查询
    m_currentQueryDomain = m_bulkDomains[m_bulkIndex];
    m_bulkIndex++;

    QStringList args = buildDigArgs(m_currentQueryDomain, m_currentServer, m_currentRecordType);
    runDig(args);

    addToHistory(m_currentQueryDomain, m_currentServer, m_currentRecordType);
}

// ─── 解析 dig 输出 ────────────────────────────────────────────────────

void DNSWidget::parseDigOutput(const QString& output)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        // 无结果
        if (m_isReverseQuery) {
            addResultRow(m_currentQueryDomain, 0, "PTR", "未找到记录");
        } else {
            addResultRow(m_currentQueryDomain, 0, m_currentRecordType, "未找到记录");
        }
        return;
    }

    bool hasRecords = false;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        if (m_isReverseQuery) {
            // 反向查询结果: dig -x +short 直接返回域名
            addResultRow(m_currentQueryDomain, 0, "PTR", trimmed);
            hasRecords = true;
        } else {
            // dig +short 输出格式可能是:
            // "value"                          — 简单值 (A/AAAA/CNAME/NS)
            // "preference value"               — MX
            // "mname rname serial refresh retry expire minimum" — SOA
            // "priority weight port target"    — SRV
            // '"text"' 或 text                 — TXT

            if (m_currentRecordType == "SOA") {
                // SOA: mname rname serial refresh retry expire minimum
                QStringList parts = trimmed.split(QRegularExpression("\\s+"));
                if (parts.size() >= 7) {
                    QString mname = parts[0];
                    addResultRow(m_currentQueryDomain, 0, "SOA",
                                 QString("主NS: %1, 管理员: %2, 序列号: %3, 刷新: %4, 重试: %5, 过期: %6, 最小TTL: %7")
                                     .arg(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6]));
                    hasRecords = true;
                }
            } else if (m_currentRecordType == "MX") {
                // MX: preference mail exchanger
                QStringList parts = trimmed.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    addResultRow(m_currentQueryDomain, 0, "MX",
                                 QString("优先级: %1, 邮件服务器: %2").arg(parts[0], parts[1]));
                    hasRecords = true;
                }
            } else if (m_currentRecordType == "SRV") {
                // SRV: priority weight port target
                QStringList parts = trimmed.split(QRegularExpression("\\s+"));
                if (parts.size() >= 4) {
                    addResultRow(m_currentQueryDomain, 0, "SRV",
                                 QString("优先级: %1, 权重: %2, 端口: %3, 目标: %4")
                                     .arg(parts[0], parts[1], parts[2], parts[3]));
                    hasRecords = true;
                }
            } else {
                // A, AAAA, CNAME, NS, TXT — 简单值
                addResultRow(m_currentQueryDomain, 0, m_currentRecordType, trimmed);
                hasRecords = true;
            }
        }
    }

    if (!hasRecords) {
        addResultRow(m_currentQueryDomain, 0, m_currentRecordType, "未找到记录");
    }
}

// ─── Process 回调 ─────────────────────────────────────────────────────

void DNSWidget::onProcessReadyRead()
{
    // 读取全部输出，在 finished 时统一解析
    // 这里不做部分解析，因为 dig 输出通常较小
}

void DNSWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    qint64 elapsed = m_timer.elapsed();

    if (m_process) {
        // 读取所有输出
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        QString errOutput = QString::fromUtf8(m_process->readAllStandardError());

        if (!errOutput.isEmpty() && exitCode != 0) {
            Logger::instance().error("DNS", "dig 错误: " + errOutput.trimmed());
            addResultRow(m_currentQueryDomain, 0, m_currentRecordType, "查询错误: " + errOutput.trimmed());
        } else {
            parseDigOutput(output);
        }

        updateInfoLabels(m_currentServer, elapsed);

        m_process->deleteLater();
        m_process = nullptr;
    }

    // 批量查询: 继续下一个
    if (m_isBulkQuery && m_bulkIndex < m_bulkDomains.size()) {
        m_currentQueryDomain = m_bulkDomains[m_bulkIndex];
        m_bulkIndex++;

        QStringList args = buildDigArgs(m_currentQueryDomain, m_currentServer, m_currentRecordType);
        runDig(args);

        addToHistory(m_currentQueryDomain, m_currentServer, m_currentRecordType);
    } else if (m_isBulkQuery) {
        // 批量查询完成
        m_isBulkQuery = false;
        m_queryBtn->setEnabled(true);
        m_reverseBtn->setEnabled(true);
        m_bulkBtn->setEnabled(true);
        m_exportBtn->setEnabled(m_resultTable->rowCount() > 0);

        Logger::instance().info("DNS", QString("批量查询完成，共 %1 个域名").arg(m_bulkDomains.size()));
    }
}

// ─── 结果表格 ─────────────────────────────────────────────────────────

void DNSWidget::addResultRow(const QString& name, int ttl, const QString& type, const QString& value)
{
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    auto* nameItem = new QTableWidgetItem(name);
    m_resultTable->setItem(row, 0, nameItem);

    auto* ttlItem = new QTableWidgetItem(ttl > 0 ? QString::number(ttl) : "-");
    ttlItem->setTextAlignment(Qt::AlignCenter);
    m_resultTable->setItem(row, 1, ttlItem);

    auto* typeItem = new QTableWidgetItem(type);
    typeItem->setTextAlignment(Qt::AlignCenter);
    // 根据记录类型着色
    QColor typeColor;
    if (type == "A") typeColor = QColor(0x40, 0x9E, 0xFF);
    else if (type == "AAAA") typeColor = QColor(0x67, 0xC2, 0x3A);
    else if (type == "MX") typeColor = QColor(0xE6, 0xA2, 0x3C);
    else if (type == "NS") typeColor = QColor(0x90, 0x90, 0x90);
    else if (type == "CNAME") typeColor = QColor(0x9B, 0x59, 0xB6);
    else if (type == "TXT") typeColor = QColor(0xF5, 0x6C, 0x6C);
    else if (type == "SOA") typeColor = QColor(0x00, 0xB4, 0xD8);
    else if (type == "PTR") typeColor = QColor(0x2C, 0xCC, 0x70);
    else if (type == "SRV") typeColor = QColor(0xFF, 0x85, 0x1B);
    else typeColor = QColor(0xDC, 0xDC, 0xDC);
    typeItem->setForeground(typeColor);
    m_resultTable->setItem(row, 2, typeItem);

    auto* valueItem = new QTableWidgetItem(value);
    m_resultTable->setItem(row, 3, valueItem);

    m_resultTable->scrollToBottom();

    // 如果不是批量查询，启用导出按钮
    if (!m_isBulkQuery) {
        m_exportBtn->setEnabled(true);
    }
}

void DNSWidget::clearResults()
{
    m_resultTable->setRowCount(0);
    m_exportBtn->setEnabled(false);
}

void DNSWidget::updateInfoLabels(const QString& server, qint64 elapsedMs)
{
    m_serverInfoLabel->setText(QString("服务器: %1").arg(server));
    if (elapsedMs >= 0) {
        m_queryTimeLabel->setText(QString("查询耗时: %1 ms").arg(elapsedMs));
    } else {
        m_queryTimeLabel->setText("查询耗时: 查询中...");
    }
}

// ─── 查询历史 ─────────────────────────────────────────────────────────

void DNSWidget::addToHistory(const QString& queryStr, const QString& server, const QString& recordType)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString entry = QString("[%1] %2 (%3) @%4")
                        .arg(timeStr, queryStr, recordType, server);

    // 插入到列表顶部
    m_historyList->insertItem(0, entry);

    // 限制历史记录数量
    while (m_historyList->count() > 100) {
        delete m_historyList->takeItem(m_historyList->count() - 1);
    }
}

void DNSWidget::onHistoryItemClicked(QListWidgetItem* item)
{
    if (!item) return;

    QString text = item->text();
    // 格式: "[HH:mm:ss] domain (TYPE) @server"
    static QRegularExpression re(R"(^\[[\d:]+\]\s+(.+?)\s+\((\w+)\)\s+@(.+)$)");
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        QString domain = match.captured(1);
        QString recordType = match.captured(2);
        QString server = match.captured(3);

        // 如果是反向查询
        if (domain.startsWith("PTR:")) {
            m_reverseEdit->setText(domain.mid(4));
            m_serverEdit->setText(server);
            onReverseQuery();
        } else {
            m_domainEdit->setText(domain);
            m_serverEdit->setText(server);

            // 设置记录类型
            int idx = m_recordTypeCombo->findText(recordType);
            if (idx >= 0) {
                m_recordTypeCombo->setCurrentIndex(idx);
            }

            onQuery();
        }
    }
}

void DNSWidget::onClearHistory()
{
    m_historyList->clear();
    Logger::instance().info("DNS", "查询历史已清除");
}

// ─── 导出 ─────────────────────────────────────────────────────────────

void DNSWidget::onExportResults()
{
    exportToCSV();
}

void DNSWidget::exportToCSV()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 DNS 结果",
        QString("dns_results_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
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
    out << "名称,TTL,类型,值\n";

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
    Logger::instance().info("DNS",
                            QString("结果已导出到: %1 (%2 条记录)").arg(filePath).arg(m_resultTable->rowCount()));
    QMessageBox::information(this, "导出成功",
                             QString("已导出 %1 条记录到:\n%2").arg(m_resultTable->rowCount()).arg(filePath));
}