#pragma once

#include <QWidget>
#include <QTreeView>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileSystemModel>
#include <QVariantMap>

class SFTPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SFTPWidget(QWidget* parent = nullptr);
    ~SFTPWidget() override;

    void setSessionId(const QString& sessionId);
    QString sessionId() const;

signals:
    void sftpCommandRequested(QString command, QVariantMap params);
    void uploadRequested(QString localPath, QString remotePath);
    void downloadRequested(QString remotePath, QString localPath);

private slots:
    // Local pane
    void onLocalTreeClicked(const QModelIndex& index);
    void onLocalTreeDoubleClicked(const QModelIndex& index);
    void onLocalPathChanged();
    void onLocalGoUp();
    void onLocalGoHome();
    void onLocalRefresh();

    // Remote pane
    void onRemoteItemClicked(QTreeWidgetItem* item, int column);
    void onRemoteItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onRemotePathChanged();
    void onRemoteGoUp();
    void onRemoteGoHome();
    void onRemoteRefresh();

    // Transfer
    void onUploadClicked();
    void onDownloadClicked();
    void onCancelTransferClicked();

    // Remote operations
    void onRemoteDelete();
    void onRemoteRename();
    void onRemoteMkdir();

    // Remote directory listing callback
    void onRemoteDirectoryListed(QString path, QVariantList entries);

private:
    void setupUI();
    void setupConnections();
    void setupLocalPane(QWidget* parent);
    void setupRemotePane(QWidget* parent);
    void setupTransferPanel(QVBoxLayout* mainLayout);
    void setupProgressPanel(QVBoxLayout* mainLayout);

    void requestRemoteDirectory(const QString& path);
    void populateRemoteTree(const QVariantList& entries);
    void setRemotePath(const QString& path);

    void addTransferEntry(const QString& localPath, const QString& remotePath,
                          const QString& direction);
    void updateTransferProgress(int row, qint64 transferred, qint64 total);

    // Local pane
    QTreeView* m_localTree;
    QFileSystemModel* m_localModel;
    QLineEdit* m_localPathEdit;
    QPushButton* m_localUpBtn;
    QPushButton* m_localHomeBtn;
    QPushButton* m_localRefreshBtn;

    // Remote pane
    QTreeWidget* m_remoteTree;
    QLineEdit* m_remotePathEdit;
    QPushButton* m_remoteUpBtn;
    QPushButton* m_remoteHomeBtn;
    QPushButton* m_remoteRefreshBtn;

    // Remote operations
    QPushButton* m_remoteDeleteBtn;
    QPushButton* m_remoteRenameBtn;
    QPushButton* m_remoteMkdirBtn;

    // Transfer buttons
    QPushButton* m_uploadBtn;
    QPushButton* m_downloadBtn;
    QPushButton* m_cancelBtn;

    // Progress
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;

    // Transfer queue
    QTableWidget* m_transferTable;

    // State
    QString m_sessionId;
    QString m_remoteCurrentPath;
    QString m_remoteHomePath;
    bool m_remoteConnected;
};