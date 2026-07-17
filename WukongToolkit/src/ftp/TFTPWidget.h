#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QComboBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QUdpSocket>
#include <QFile>
#include <QDateTime>
#include <QVector>

// ============================================================================
// TransferRecord — 传输历史记录
// ============================================================================
struct TransferRecord
{
    QDateTime timestamp;
    QString direction;   // "上传" / "下载"
    QString serverAddr;
    quint16 port;
    QString localFile;
    QString remoteFile;
    qint64 fileSize;
    bool success;
    QString errorMsg;
};

// ============================================================================
// TFTPServer — 基于 QUdpSocket 的简易 TFTP 服务器
// ============================================================================
class TFTPServer : public QObject
{
    Q_OBJECT

public:
    explicit TFTPServer(QObject* parent = nullptr);
    ~TFTPServer() override;

    bool start(quint16 port, const QString& rootDir);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    QString rootDir() const;

signals:
    void clientConnected(const QString& clientAddr);
    void transferStarted(const QString& clientAddr, const QString& filename, const QString& mode);
    void transferProgress(const QString& clientAddr, int blockNum, qint64 bytesTransferred);
    void transferCompleted(const QString& clientAddr, const QString& filename, qint64 totalBytes);
    void transferError(const QString& clientAddr, const QString& error);
    void serverLog(const QString& message);

private slots:
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    void handleRRQ(const QHostAddress& sender, quint16 senderPort, const QString& filename, const QString& mode);
    void handleWRQ(const QHostAddress& sender, quint16 senderPort, const QString& filename, const QString& mode);
    void sendData(const QHostAddress& addr, quint16 port, quint16 blockNum, const QByteArray& data);
    void sendAck(const QHostAddress& addr, quint16 port, quint16 blockNum);
    void sendError(const QHostAddress& addr, quint16 port, quint16 errorCode, const QString& errorMsg);
    QByteArray readBlock(QFile& file, quint16 blockNum);

    QUdpSocket* m_socket;
    QString m_rootDir;
    bool m_running;
    quint16 m_port;

    struct ClientTransfer {
        QHostAddress address;
        quint16 port;
        quint16 lastBlock;
        QString filename;
        QFile* file;
        bool isWrite; // true = WRQ (client sending), false = RRQ (client requesting)
    };
    QMap<QString, ClientTransfer> m_transfers;

    QString clientKey(const QHostAddress& addr, quint16 port) const;
};

// ============================================================================
// TFTPWidget — TFTP 服务器/客户端工具
// ============================================================================
class TFTPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TFTPWidget(QWidget* parent = nullptr);
    ~TFTPWidget() override;

private slots:
    // Mode switching
    void onModeChanged(int index);

    // Server
    void onServerBrowseRootDir();
    void onServerStart();
    void onServerStop();
    void onServerLog(const QString& message);
    void onServerTransferStarted(const QString& clientAddr, const QString& filename, const QString& mode);
    void onServerTransferCompleted(const QString& clientAddr, const QString& filename, qint64 totalBytes);

    // Client
    void onClientBrowseLocalFile();
    void onClientUpload();
    void onClientDownload();
    void onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClientProcessError(QProcess::ProcessError error);

    // History
    void onExportHistory();
    void onClearHistory();

private:
    void setupUI();
    void setupConnections();

    QWidget* createServerPage();
    QWidget* createClientPage();

    void addHistoryRecord(const TransferRecord& record);
    void saveToDatabase(const TransferRecord& record);

    // --- Mode selector ---
    QComboBox* m_modeCombo;
    QStackedWidget* m_pageStack;

    // --- Server page ---
    QSpinBox* m_serverPortSpin;
    QLineEdit* m_serverRootDirEdit;
    QPushButton* m_serverBrowseBtn;
    QPushButton* m_serverStartBtn;
    QPushButton* m_serverStopBtn;
    QLabel* m_serverStatusLabel;
    QTextEdit* m_serverLogEdit;
    TFTPServer* m_tftpServer;

    // --- Client page ---
    QLineEdit* m_clientServerAddrEdit;
    QSpinBox* m_clientPortSpin;
    QLineEdit* m_clientLocalFileEdit;
    QPushButton* m_clientBrowseBtn;
    QLineEdit* m_clientRemoteFileEdit;
    QPushButton* m_clientUploadBtn;
    QPushButton* m_clientDownloadBtn;
    QProgressBar* m_clientProgressBar;
    QLabel* m_clientStatusLabel;
    QProcess* m_clientProcess;
    QString m_clientCurrentLocalFile;
    QString m_clientCurrentRemoteFile;
    QString m_clientCurrentServerAddr;
    quint16 m_clientCurrentPort;
    bool m_clientIsUpload;

    // --- Transfer history ---
    QTableWidget* m_historyTable;
    QPushButton* m_exportBtn;
    QPushButton* m_clearBtn;
};