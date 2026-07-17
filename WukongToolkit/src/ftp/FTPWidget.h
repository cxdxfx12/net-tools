#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QProcess>
#include <QListWidget>
#include <QLabel>

// ============================================================================
// FTPWidget — FTP 文件传输工具 (使用系统 curl/命令行 ftp)
// ============================================================================
class FTPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FTPWidget(QWidget* parent = nullptr);
    ~FTPWidget() override;

private slots:
    void onConnect();
    void onDisconnect();
    void onUpload();
    void onDownload();
    void onBrowseLocal();
    void onListDir();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommandReturn();
    void onClearOutput();

private:
    void setupUI();
    void setupConnections();
    void appendOutput(const QString& text, const QString& color = "#E6EDF3");
    void executeFtpCommand(const QStringList& args);
    void executeInteractive(const QStringList& args);

    // --- 连接信息 ---
    QLineEdit*    m_hostEdit;
    QSpinBox*     m_portSpin;
    QLineEdit*    m_userEdit;
    QLineEdit*    m_passEdit;
    QPushButton*  m_connectBtn;
    QPushButton*  m_disconnectBtn;

    // --- 远程浏览 ---
    QPushButton*  m_listBtn;
    QLineEdit*    m_remotePathEdit;
    QTreeWidget*  m_remoteTree;

    // --- 本地浏览 ---
    QLineEdit*    m_localPathEdit;
    QPushButton*  m_browseBtn;

    // --- 操作按钮 ---
    QPushButton*  m_uploadBtn;
    QPushButton*  m_downloadBtn;

    // --- 输出区 ---
    QPlainTextEdit* m_outputText;
    QPushButton*    m_clearOutputBtn;

    // --- 命令输入 ---
    QLineEdit*    m_commandEdit;

    // --- 状态 ---
    QLabel*       m_statusLabel;
    QProcess*     m_process;
    bool          m_connected;
};
