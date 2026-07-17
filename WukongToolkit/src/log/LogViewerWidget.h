#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QTimer>

// ============================================================================
// LogViewerWidget — 应用日志查看器
// ============================================================================
class LogViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogViewerWidget(QWidget* parent = nullptr);
    ~LogViewerWidget() override;

private slots:
    void onLogMessage(const QString& time, const QString& level,
                      const QString& module, const QString& message);
    void onClear();
    void onExport();
    void onPauseToggled(bool paused);
    void onFilterChanged();
    void onAutoScroll();

private:
    void setupUI();
    void setupConnections();
    void appendLog(const QString& time, const QString& level,
                   const QString& module, const QString& message);
    bool matchesFilter(const QString& level, const QString& module, const QString& message);

    // --- 过滤控件 ---
    QComboBox*    m_levelFilter;
    QLineEdit*    m_moduleFilter;
    QLineEdit*    m_textFilter;
    QCheckBox*    m_autoScrollCheck;
    QPushButton*  m_pauseBtn;

    // --- 输出 ---
    QPlainTextEdit* m_logView;
    QPushButton*    m_clearBtn;
    QPushButton*    m_exportBtn;

    // --- 统计 ---
    QLabel*       m_statsLabel;
    int           m_totalLogs;
    int           m_infoCount;
    int           m_warningCount;
    int           m_errorCount;
    int           m_debugCount;

    // --- 状态 ---
    bool          m_paused;
    QStringList   m_pendingLogs; // 暂停期间缓存的日志
    QTimer*       m_autoScrollTimer;
};
