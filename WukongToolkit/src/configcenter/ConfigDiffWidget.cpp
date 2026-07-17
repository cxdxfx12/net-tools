#include "configcenter/ConfigDiffWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QRegularExpression>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// ConfigDiffWidget 实现
// ============================================================================

ConfigDiffWidget::ConfigDiffWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

ConfigDiffWidget::~ConfigDiffWidget() = default;

// ─── UI Setup ──────────────────────────────────────────────────────────

void ConfigDiffWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 顶部工具栏 ──
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    m_compareBtn = new QPushButton("对比差异");
    m_compareBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );

    m_swapBtn = new QPushButton("交换左右");
    m_swapBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 8px 16px;"
        "  border-radius: 4px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );

    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #F85149;"
        "  border: 1px solid #30363D; padding: 8px 16px;"
        "  border-radius: 4px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );

    m_exportBtn = new QPushButton("导出差异");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
    );

    toolbar->addWidget(m_compareBtn);
    toolbar->addWidget(m_swapBtn);
    toolbar->addWidget(m_clearBtn);
    toolbar->addStretch();
    toolbar->addWidget(m_exportBtn);

    mainLayout->addLayout(toolbar);

    // ── 统计信息 ──
    m_statsLabel = new QLabel(QString::fromUtf8("就绪 - 加载两个配置文件后点击「对比差异」"));
    m_statsLabel->setStyleSheet(
        "font-size: 13px; color: #8B949E; padding: 4px 12px;"
        "background-color: #161B22; border: 1px solid #30363D; border-radius: 3px;"
    );
    mainLayout->addWidget(m_statsLabel);

    // ── 对比区 (左右分屏) ──
    auto* diffLayout = new QHBoxLayout();
    diffLayout->setSpacing(4);

    // 左侧
    auto* leftPanel = new QWidget();
    auto* leftPanelLayout = new QVBoxLayout(leftPanel);
    leftPanelLayout->setContentsMargins(0, 0, 0, 0);
    leftPanelLayout->setSpacing(4);

    auto* leftHeader = new QHBoxLayout();
    m_leftLabel = new QLabel("配置 A (原始)");
    m_leftLabel->setStyleSheet("font-size: 13px; color: #E6EDF3; font-weight: bold;");
    leftHeader->addWidget(m_leftLabel);
    leftHeader->addStretch();
    m_loadLeftBtn = new QPushButton("加载文件...");
    m_loadLeftBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #58A6FF;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    leftHeader->addWidget(m_loadLeftBtn);
    leftPanelLayout->addLayout(leftHeader);

    m_leftEdit = new QPlainTextEdit();
    m_leftEdit->setPlaceholderText("粘贴或加载配置内容...");
    m_leftEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    leftPanelLayout->addWidget(m_leftEdit, 1);

    // 右侧
    auto* rightPanel = new QWidget();
    auto* rightPanelLayout = new QVBoxLayout(rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(4);

    auto* rightHeader = new QHBoxLayout();
    m_rightLabel = new QLabel("配置 B (对比)");
    m_rightLabel->setStyleSheet("font-size: 13px; color: #E6EDF3; font-weight: bold;");
    rightHeader->addWidget(m_rightLabel);
    rightHeader->addStretch();
    m_loadRightBtn = new QPushButton("加载文件...");
    m_loadRightBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #58A6FF;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    rightHeader->addWidget(m_loadRightBtn);
    rightPanelLayout->addLayout(rightHeader);

    m_rightEdit = new QPlainTextEdit();
    m_rightEdit->setPlaceholderText("粘贴或加载配置内容...");
    m_rightEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    rightPanelLayout->addWidget(m_rightEdit, 1);

    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStyleSheet("QSplitter::handle { background-color: #30363D; width: 3px; }");

    diffLayout->addWidget(splitter);
    mainLayout->addLayout(diffLayout, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void ConfigDiffWidget::setupConnections()
{
    connect(m_compareBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onCompare);
    connect(m_swapBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onSwap);
    connect(m_clearBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onClear);
    connect(m_exportBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onExportDiff);
    connect(m_loadLeftBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onLoadLeft);
    connect(m_loadRightBtn, &QPushButton::clicked, this, &ConfigDiffWidget::onLoadRight);
}

// ─── Slot: Load Left ───────────────────────────────────────────────────

void ConfigDiffWidget::onLoadLeft()
{
    QString filePath = QFileDialog::getOpenFileName(this, "加载配置 A",
                                                     QString(),
                                                     "配置文件 (*.cfg *.conf *.txt *.json *.xml);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "加载失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    m_leftEdit->setPlainText(stream.readAll());
    file.close();

    QFileInfo fi(filePath);
    m_leftLabel->setText("配置 A: " + fi.fileName());
    Logger::instance().info("ConfigDiff", "加载配置 A: " + filePath);
}

// ─── Slot: Load Right ──────────────────────────────────────────────────

void ConfigDiffWidget::onLoadRight()
{
    QString filePath = QFileDialog::getOpenFileName(this, "加载配置 B",
                                                     QString(),
                                                     "配置文件 (*.cfg *.conf *.txt *.json *.xml);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "加载失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    m_rightEdit->setPlainText(stream.readAll());
    file.close();

    QFileInfo fi(filePath);
    m_rightLabel->setText("配置 B: " + fi.fileName());
    Logger::instance().info("ConfigDiff", "加载配置 B: " + filePath);
}

// ─── Slot: Compare ─────────────────────────────────────────────────────

void ConfigDiffWidget::onCompare()
{
    QString left = m_leftEdit->toPlainText();
    QString right = m_rightEdit->toPlainText();

    if (left.isEmpty() && right.isEmpty()) {
        QMessageBox::information(this, "对比", "请先加载或输入配置内容。");
        return;
    }

    highlightDifferences();

    // 生成统一 diff 统计
    QStringList leftLines = left.split('\n');
    QStringList rightLines = right.split('\n');

    int added = 0, removed = 0, unchanged = 0;

    // 简单的逐行对比
    int maxLen = qMax(leftLines.size(), rightLines.size());
    for (int i = 0; i < maxLen; ++i) {
        if (i >= leftLines.size()) {
            added++;
        } else if (i >= rightLines.size()) {
            removed++;
        } else if (leftLines[i] != rightLines[i]) {
            removed++;
            added++;
        } else {
            unchanged++;
        }
    }

    m_statsLabel->setText(
        QString("对比结果: +%1 行新增  -%2 行删除  =%3 行未变 | 共 %4 行差异")
            .arg(added).arg(removed).arg(unchanged).arg(added + removed));
    m_statsLabel->setStyleSheet(
        "font-size: 13px; color: #E6EDF3; padding: 4px 12px;"
        "background-color: #161B22; border: 1px solid #30363D; border-radius: 3px;"
    );

    Logger::instance().info("ConfigDiff",
                            QString("对比完成: +%1 -%2 =%3").arg(added).arg(removed).arg(unchanged));
}

// ─── Highlight Differences ─────────────────────────────────────────────

void ConfigDiffWidget::highlightDifferences()
{
    QString left = m_leftEdit->toPlainText();
    QString right = m_rightEdit->toPlainText();

    // 逐行对比，用特殊前缀标记差异行
    QStringList leftLines = left.split('\n');
    QStringList rightLines = right.split('\n');

    QString leftResult, rightResult;
    int maxLen = qMax(leftLines.size(), rightLines.size());

    for (int i = 0; i < maxLen; ++i) {
        QString l = (i < leftLines.size()) ? leftLines[i] : "";
        QString r = (i < rightLines.size()) ? rightLines[i] : "";

        if (l != r) {
            if (i < leftLines.size())
                leftResult += "[-] " + l + "\n";
            if (i < rightLines.size())
                rightResult += "[+] " + r + "\n";
        } else {
            leftResult += "    " + l + "\n";
            rightResult += "    " + r + "\n";
        }
    }

    m_leftEdit->setPlainText(leftResult);
    m_rightEdit->setPlainText(rightResult);
}

// ─── Slot: Swap ────────────────────────────────────────────────────────

void ConfigDiffWidget::onSwap()
{
    QString leftText = m_leftEdit->toPlainText();
    QString rightText = m_rightEdit->toPlainText();
    QString leftLabel = m_leftLabel->text();
    QString rightLabel = m_rightLabel->text();

    m_leftEdit->setPlainText(rightText);
    m_rightEdit->setPlainText(leftText);
    m_leftLabel->setText(rightLabel);
    m_rightLabel->setText(leftLabel);

    Logger::instance().info("ConfigDiff", "交换左右配置");
}

// ─── Slot: Clear ───────────────────────────────────────────────────────

void ConfigDiffWidget::onClear()
{
    m_leftEdit->clear();
    m_rightEdit->clear();
    m_leftLabel->setText("配置 A (原始)");
    m_rightLabel->setText("配置 B (对比)");
    m_statsLabel->setText(QString::fromUtf8("就绪 - 加载两个配置文件后点击「对比差异」"));
    m_statsLabel->setStyleSheet(
        "font-size: 13px; color: #8B949E; padding: 4px 12px;"
        "background-color: #161B22; border: 1px solid #30363D; border-radius: 3px;"
    );
}

// ─── Slot: Export Diff ─────────────────────────────────────────────────

void ConfigDiffWidget::onExportDiff()
{
    QString left = m_leftEdit->toPlainText();
    QString right = m_rightEdit->toPlainText();

    if (left.isEmpty() && right.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的内容。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出差异",
                                                     "config_diff.txt",
                                                     "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "=== 配置差异对比 ===\n";
    stream << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    stream << "=== 配置 A ===\n" << left << "\n\n";
    stream << "=== 配置 B ===\n" << right << "\n\n";
    stream << "=== 差异 (Unified Diff) ===\n";
    stream << generateUnifiedDiff(left, right);

    file.close();

    Logger::instance().info("ConfigDiff", "导出差异: " + filePath);
    QMessageBox::information(this, "导出成功", "差异已导出到:\n" + filePath);
}

// ─── Generate Unified Diff ─────────────────────────────────────────────

QString ConfigDiffWidget::generateUnifiedDiff(const QString& left, const QString& right)
{
    QStringList leftLines = left.split('\n');
    QStringList rightLines = right.split('\n');
    QString result;

    int maxLen = qMax(leftLines.size(), rightLines.size());
    for (int i = 0; i < maxLen; ++i) {
        QString l = (i < leftLines.size()) ? leftLines[i] : "";
        QString r = (i < rightLines.size()) ? rightLines[i] : "";

        if (l != r) {
            if (i < leftLines.size())
                result += QString("L%1: -%2\n").arg(i + 1).arg(l);
            if (i < rightLines.size())
                result += QString("R%1: +%2\n").arg(i + 1).arg(r);
        }
    }

    return result;
}
