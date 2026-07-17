#include "network/WhoisWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

// ============================================================================
// ============================================================================
// WhoisWidget 实现
// ============================================================================

WhoisWidget::WhoisWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
{
    setupUI();
    setupConnections();
}

WhoisWidget::~WhoisWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void WhoisWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 左侧: 查询区 ──
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 输入区
    auto* inputGroup = new QGroupBox("WHOIS 查询");
    auto* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(8);

    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("输入域名或 IP 地址，如 example.com");
    inputRow->addWidget(m_inputEdit, 1);

    m_queryBtn = new QPushButton("查询");
    m_queryBtn->setFixedHeight(32);
    inputRow->addWidget(m_queryBtn);

    inputLayout->addLayout(inputRow);

    // 导出按钮
    auto* exportRow = new QHBoxLayout();
    exportRow->addStretch();
    m_exportBtn = new QPushButton("导出结果");
    m_exportBtn->setEnabled(false);
    exportRow->addWidget(m_exportBtn);
    inputLayout->addLayout(exportRow);

    leftLayout->addWidget(inputGroup);

    // 结果区
    auto* resultGroup = new QGroupBox("查询结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);
    resultLayout->setSpacing(4);

    m_resultText = new QPlainTextEdit();
    m_resultText->setReadOnly(true);
    m_resultText->setPlaceholderText("WHOIS 查询结果将显示在此处...");
    resultLayout->addWidget(m_resultText, 1);

    leftLayout->addWidget(resultGroup, 1);

    // ── 右侧: 历史记录 ──
    auto* historyGroup = new QGroupBox("查询历史");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    m_historyList = new QListWidget();
    historyLayout->addWidget(m_historyList, 1);

    m_clearHistoryBtn = new QPushButton("清空历史");
    historyLayout->addWidget(m_clearHistoryBtn);

    // ── Splitter ──
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(historyGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setStyleSheet("QSplitter::handle { background-color: #30363D; width: 2px; }");

    mainLayout->addWidget(splitter);
}

// ─── Signal Connections ────────────────────────────────────────────────

void WhoisWidget::setupConnections()
{
    connect(m_queryBtn, &QPushButton::clicked, this, &WhoisWidget::onQuery);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &WhoisWidget::onQuery);
    connect(m_exportBtn, &QPushButton::clicked, this, &WhoisWidget::onExport);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &WhoisWidget::onClearHistory);
    connect(m_historyList, &QListWidget::itemClicked, this, &WhoisWidget::onHistoryItemClicked);
}

// ─── Slot: Query ───────────────────────────────────────────────────────

void WhoisWidget::onQuery()
{
    QString target = m_inputEdit->text().trimmed();
    if (target.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入域名或 IP 地址。");
        return;
    }

    m_resultText->clear();
    m_resultText->setPlaceholderText("查询中...");
    m_queryBtn->setEnabled(false);
    m_queryBtn->setText("查询中...");
    m_exportBtn->setEnabled(false);

    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WhoisWidget::onProcessFinished);

    m_process->start("whois", QStringList() << target);

    addToHistory(target);
    Logger::instance().info("WHOIS", "查询 WHOIS: " + target);
}

// ─── Slot: Process Finished ────────────────────────────────────────────

void WhoisWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    m_queryBtn->setEnabled(true);
    m_queryBtn->setText("查询");

    if (!m_process) return;

    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_process->readAllStandardError());

    m_process->deleteLater();
    m_process = nullptr;

    if (output.isEmpty() && !errOutput.isEmpty()) {
        m_resultText->setPlainText("错误: " + errOutput.trimmed());
        Logger::instance().error("WHOIS", "查询失败: " + errOutput.trimmed());
        return;
    }

    if (output.trimmed().isEmpty()) {
        m_resultText->setPlainText("未返回结果。");
        return;
    }

    m_resultText->setPlainText(output);
    m_exportBtn->setEnabled(true);

    Logger::instance().info("WHOIS",
                            QString("WHOIS 查询完成，%1 字符").arg(output.size()));
}

// ─── Slot: Export ──────────────────────────────────────────────────────

void WhoisWidget::onExport()
{
    if (m_resultText->toPlainText().isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 WHOIS 结果",
                                                     "whois_result.txt",
                                                     "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << m_resultText->toPlainText();
    file.close();

    Logger::instance().info("WHOIS", "导出结果: " + filePath);
    QMessageBox::information(this, "导出成功", "结果已导出到:\n" + filePath);
}

// ─── History Management ────────────────────────────────────────────────

void WhoisWidget::addToHistory(const QString& target)
{
    // 去重：如果已存在则移到顶部
    m_historyItems.removeAll(target);

    m_historyItems.prepend(target);
    if (m_historyItems.size() > kMaxHistory) {
        m_historyItems.removeLast();
    }

    // 更新 UI
    m_historyList->clear();
    for (const QString& item : m_historyItems) {
        m_historyList->addItem(item);
    }
}

void WhoisWidget::onClearHistory()
{
    m_historyItems.clear();
    m_historyList->clear();
    Logger::instance().info("WHOIS", "清空查询历史");
}

void WhoisWidget::onHistoryItemClicked(QListWidgetItem* item)
{
    if (item) {
        m_inputEdit->setText(item->text());
        onQuery();
    }
}
