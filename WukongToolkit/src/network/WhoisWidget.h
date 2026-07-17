#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QProcess>
#include <QListWidget>

// ============================================================================
// WhoisWidget — 独立 WHOIS 域名/IP 查询工具
// ============================================================================
class WhoisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WhoisWidget(QWidget* parent = nullptr);
    ~WhoisWidget() override;

private slots:
    void onQuery();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExport();
    void onClearHistory();
    void onHistoryItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupConnections();
    void addToHistory(const QString& target);

    // --- UI 组件 ---
    QLineEdit*      m_inputEdit;
    QPushButton*    m_queryBtn;
    QPlainTextEdit* m_resultText;
    QPushButton*    m_exportBtn;

    // 历史记录
    QListWidget*    m_historyList;
    QPushButton*    m_clearHistoryBtn;

    // --- 查询状态 ---
    QProcess*       m_process;
    QStringList     m_historyItems;
    static constexpr int kMaxHistory = 50;
};
