#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QThreadPool>
#include <QRunnable>
#include <QTcpSocket>
#include <QMutex>
#include <QAtomicInt>
#include <QMenu>
#include <QStringList>
#include <QElapsedTimer>

// ============================================================================
// PortScanWorker — QRunnable 执行单个端口的 TCP Connect 扫描 + Banner 读取
// ============================================================================
class PortScanWorker : public QRunnable
{
public:
    PortScanWorker(const QString& host, int port, int timeoutMs, QObject* receiver,
                   QAtomicInt* scanningFlag, QAtomicInt* progressCounter);

    void run() override;

private:
    QString readBanner(QTcpSocket* socket, int timeoutMs);

    QString m_host;
    int m_port;
    int m_timeoutMs;
    QObject* m_receiver;
    QAtomicInt* m_scanning;
    QAtomicInt* m_progress;
};

// ============================================================================
// PortScannerWidget — TCP 端口扫描器
// ============================================================================
class PortScannerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PortScannerWidget(QWidget* parent = nullptr);
    ~PortScannerWidget() override;

    /// 根据端口号返回内置服务名
    static QString lookupService(int port);

signals:
    void scanFinished(int openPorts);
    void portFound(int port, QString service, qint64 responseTime);

public slots:
    void onResultReceived(int port, QString service, qint64 responseTime,
                          QString state, QString banner);

private slots:
    void onStartScan();
    void onStopScan();
    void onExportCSV();
    void onContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupConnections();
    void parsePortRange(const QString& input, QList<int>& outPorts);
    void addResultRow(int port, const QString& service, const QString& state,
                      qint64 responseTime, const QString& banner);
    void clearResults();

    // --- UI 组件 ---
    QLineEdit*   m_hostEdit;
    QLineEdit*   m_portRangeEdit;
    QComboBox*   m_scanTypeCombo;
    QSpinBox*    m_threadCountSpin;
    QSpinBox*    m_timeoutSpin;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_exportBtn;
    QTableWidget* m_resultTable;
    QProgressBar* m_progressBar;

    // --- 扫描状态 ---
    QThreadPool* m_threadPool;
    QAtomicInt   m_scanning;
    QAtomicInt   m_scannedCount;
    int          m_totalCount = 0;
    QMutex       m_resultMutex;
    int          m_openCount = 0;
};