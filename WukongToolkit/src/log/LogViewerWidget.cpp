#include "log/LogViewerWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// LogViewerWidget 实现
// ============================================================================

LogViewerWidget::LogViewerWidget(QWidget* parent)
    : QWidget(parent)
    , m_totalLogs(0)
    , m_infoCount(0)
    , m_warningCount(0)
    , m_errorCount(0)
    , m_debugCount(0)
    , m_paused(false)
{
    setupUI();
    setupConnections();

    // 连接全局日志信号
    connect(&Logger::instance(), &Logger::logMessage,
            this, &LogViewerWidget::onLogMessage);
}

LogViewerWidget::~LogViewerWidget() = default;

// ─── UI Setup ──────────────────────────────────────────────────────────

void LogViewerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 过滤区 ──
    auto* filterGroup = new QGroupBox("日志过滤");
    auto* filterLayout = new QHBoxLayout(filterGroup);
    filterLayout->setSpacing(8);

    auto* levelLabel = new QLabel("级别:");
    
    m_levelFilter = new QComboBox();
    m_levelFilter->addItems({"全部", "DEBUG", "INFO", "WARNING", "ERROR"});
    m_levelFilter->setCurrentIndex(0);
    m_levelFilter->setFixedWidth(100);
    filterLayout->addWidget(levelLabel);
    filterLayout->addWidget(m_levelFilter);

    auto* moduleLabel = new QLabel("模块:");
    
    m_moduleFilter = new QLineEdit();
    m_moduleFilter->setPlaceholderText("如: APP, SSH, FTP...");
    m_moduleFilter->setFixedWidth(140);
    filterLayout->addWidget(moduleLabel);
    filterLayout->addWidget(m_moduleFilter);

    auto* textLabel = new QLabel("关键词:");
    
    m_textFilter = new QLineEdit();
    m_textFilter->setPlaceholderText("搜索日志内容...");
    filterLayout->addWidget(textLabel);
    filterLayout->addWidget(m_textFilter, 1);

    m_autoScrollCheck = new QCheckBox("自动滚动");
    m_autoScrollCheck->setChecked(true);
    
    filterLayout->addWidget(m_autoScrollCheck);

    m_pauseBtn = new QPushButton("暂停");
    m_pauseBtn->setCheckable(true);
    m_pauseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 14px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
        "QPushButton:checked {"
        "  background-color: #D29922; color: white;"
        "  border-color: #D29922;"
        "}"
    );
    filterLayout->addWidget(m_pauseBtn);

    mainLayout->addWidget(filterGroup);

    // ── 统计标签 ──
    m_statsLabel = new QLabel("总计: 0 | INFO: 0 | WARNING: 0 | ERROR: 0 | DEBUG: 0");
    
    mainLayout->addWidget(m_statsLabel);

    // ── 日志输出区 ──
    auto* logGroup = new QGroupBox("日志输出");
    auto* logLayout = new QVBoxLayout(logGroup);
    logLayout->setSpacing(4);

    // 工具栏
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);
    m_clearBtn = new QPushButton("清空日志");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #F85149;"
        "  border: 1px solid #30363D; padding: 5px 14px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    toolbar->addWidget(m_clearBtn);
    toolbar->addStretch();
    m_exportBtn = new QPushButton("导出日志");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );
    toolbar->addWidget(m_exportBtn);
    logLayout->addLayout(toolbar);

    m_logView = new QPlainTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(10000);
    m_logView->setLineWrapMode(QPlainTextEdit::NoWrap);
    logLayout->addWidget(m_logView, 1);

    mainLayout->addWidget(logGroup, 1);

    // ── 自动滚动定时器 ──
    m_autoScrollTimer = new QTimer(this);
    m_autoScrollTimer->setInterval(100);
    connect(m_autoScrollTimer, &QTimer::timeout, this, &LogViewerWidget::onAutoScroll);
    m_autoScrollTimer->start();
}

// ─── Signal Connections ────────────────────────────────────────────────

void LogViewerWidget::setupConnections()
{
    connect(m_clearBtn, &QPushButton::clicked, this, &LogViewerWidget::onClear);
    connect(m_exportBtn, &QPushButton::clicked, this, &LogViewerWidget::onExport);
    connect(m_pauseBtn, &QPushButton::toggled, this, &LogViewerWidget::onPauseToggled);
    connect(m_levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerWidget::onFilterChanged);
    connect(m_moduleFilter, &QLineEdit::textChanged, this, &LogViewerWidget::onFilterChanged);
    connect(m_textFilter, &QLineEdit::textChanged, this, &LogViewerWidget::onFilterChanged);
}

// ─── Slot: Log Message (from Logger) ───────────────────────────────────

void LogViewerWidget::onLogMessage(const QString& time, const QString& level,
                                    const QString& module, const QString& message)
{
    // 统计
    m_totalLogs++;
    if (level == "INFO") m_infoCount++;
    else if (level == "WARNING") m_warningCount++;
    else if (level == "ERROR") m_errorCount++;
    else if (level == "DEBUG") m_debugCount++;

    // 更新统计标签 (每 50 条更新一次以减少 UI 开销)
    if (m_totalLogs % 50 == 0 || m_totalLogs <= 10) {
        m_statsLabel->setText(
            QString("总计: %1 | INFO: %2 | WARNING: %3 | ERROR: %4 | DEBUG: %5")
                .arg(m_totalLogs).arg(m_infoCount).arg(m_warningCount)
                .arg(m_errorCount).arg(m_debugCount));
    }

    if (m_paused) {
        // 缓存日志
        m_pendingLogs << time << level << module << message;
        return;
    }

    // 过滤
    if (!matchesFilter(level, module, message)) return;

    appendLog(time, level, module, message);
}

// ─── Append Log ────────────────────────────────────────────────────────

void LogViewerWidget::appendLog(const QString& time, const QString& level,
                                 const QString& module, const QString& message)
{
    QString color;
    if (level == "INFO") {
        color = "#3FB950";
    } else if (level == "WARNING") {
        color = "#D29922";
    } else if (level == "ERROR") {
        color = "#F85149";
    } else {
        color = "#8B949E"; // DEBUG
    }

    QString html = QString(
        "<span style='color:#8C8C8C;'>%1</span> "
        "<span style='color:%2;'>[%3]</span> "
        "<span style='color:#409EFF;'>[%4]</span> "
        "<span style='color:#E6EDF3;'>%5</span>"
    ).arg(time, color, level, module, message.toHtmlEscaped());

    m_logView->appendHtml(html);
}

// ─── Filter Matching ───────────────────────────────────────────────────

bool LogViewerWidget::matchesFilter(const QString& level, const QString& module,
                                     const QString& message)
{
    // 级别过滤
    QString levelFilter = m_levelFilter->currentText();
    if (levelFilter != "全部" && level != levelFilter) {
        return false;
    }

    // 模块过滤 (大小写不敏感)
    QString moduleFilter = m_moduleFilter->text().trimmed();
    if (!moduleFilter.isEmpty() &&
        !module.contains(moduleFilter, Qt::CaseInsensitive)) {
        return false;
    }

    // 关键词过滤
    QString textFilter = m_textFilter->text().trimmed();
    if (!textFilter.isEmpty() &&
        !message.contains(textFilter, Qt::CaseInsensitive)) {
        return false;
    }

    return true;
}

// ─── Slot: Clear ───────────────────────────────────────────────────────

void LogViewerWidget::onClear()
{
    m_logView->clear();
    m_totalLogs = 0;
    m_infoCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;
    m_debugCount = 0;
    m_pendingLogs.clear();
    m_statsLabel->setText("总计: 0 | INFO: 0 | WARNING: 0 | ERROR: 0 | DEBUG: 0");
}

// ─── Slot: Export ──────────────────────────────────────────────────────

void LogViewerWidget::onExport()
{
    if (m_logView->toPlainText().isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的日志。");
        return;
    }

    QString defaultName = "wukong_log_" +
                          QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";

    QString filePath = QFileDialog::getSaveFileName(this, "导出日志",
                                                     defaultName,
                                                     "日志文件 (*.log *.txt);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "=== 悟空网络工程师工具箱 日志导出 ===\n";
    stream << "导出时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    stream << "统计: 总计=" << m_totalLogs
           << " INFO=" << m_infoCount
           << " WARNING=" << m_warningCount
           << " ERROR=" << m_errorCount
           << " DEBUG=" << m_debugCount << "\n";
    stream << "========================================\n\n";
    stream << m_logView->toPlainText();
    file.close();

    Logger::instance().info("LogViewer", "导出日志: " + filePath);
    QMessageBox::information(this, "导出成功", "日志已导出到:\n" + filePath);
}

// ─── Slot: Pause Toggled ───────────────────────────────────────────────

void LogViewerWidget::onPauseToggled(bool paused)
{
    m_paused = paused;
    m_pauseBtn->setText(paused ? "继续" : "暂停");

    if (!paused && !m_pendingLogs.isEmpty()) {
        // 恢复时重放缓存的日志
        for (int i = 0; i < m_pendingLogs.size(); i += 4) {
            QString time = m_pendingLogs[i];
            QString level = m_pendingLogs[i + 1];
            QString module = m_pendingLogs[i + 2];
            QString message = m_pendingLogs[i + 3];

            if (matchesFilter(level, module, message)) {
                appendLog(time, level, module, message);
            }
        }
        m_pendingLogs.clear();
    }
}

// ─── Slot: Filter Changed ──────────────────────────────────────────────

void LogViewerWidget::onFilterChanged()
{
    // 重建显示 - 清除并重新从缓存数据中过滤显示
    // 由于日志是从 Logger 实时推送的，过滤只在新增日志时生效
    // 这里提供清空重建的选项
    // 实际实现：保留已有日志，新日志按过滤条件显示
}

// ─── Auto Scroll ───────────────────────────────────────────────────────

void LogViewerWidget::onAutoScroll()
{
    if (m_autoScrollCheck->isChecked() && !m_paused) {
        QScrollBar* bar = m_logView->verticalScrollBar();
        if (bar) {
            bar->setValue(bar->maximum());
        }
    }
}
