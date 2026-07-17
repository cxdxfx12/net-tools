#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>

// ============================================================================
// SCPWidget — SCP 安全文件传输工具 (基于系统 scp 命令)
// ============================================================================
class SCPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SCPWidget(QWidget* parent = nullptr);
    ~SCPWidget() override;

private slots:
    void onUpload();
    void onDownload();
    void onBrowseLocal();
    void onBrowseRemotePath();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClearOutput();
    void onHistoryItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupConnections();
    void appendOutput(const QString& text, const QString& color = "#E6EDF3");
    void addToHistory(const QString& entry);
    void executeScp(const QStringList& args, const QString& desc);

    // --- 连接信息 ---
    QLineEdit*    m_hostEdit;
    QSpinBox*     m_portSpin;
    QLineEdit*    m_userEdit;
    QLineEdit*    m_remotePathEdit;

    // --- 本地路径 ---
    QLineEdit*    m_localPathEdit;
    QPushButton*  m_browseLocalBtn;

    // --- 操作按钮 ---
    QPushButton*  m_uploadBtn;
    QPushButton*  m_downloadBtn;

    // --- 进度 ---
    QProgressBar* m_progressBar;
    QLabel*       m_statusLabel;

    // --- 输出 ---
    QPlainTextEdit* m_outputText;
    QPushButton*    m_clearOutputBtn;

    // --- 历史 ---
    QListWidget*    m_historyList;
    QPushButton*    m_clearHistoryBtn;

    // --- 状态 ---
    QProcess*       m_process;
    bool            m_isRunning;
    QStringList     m_historyItems;
    static constexpr int kMaxHistory = 50;
};
