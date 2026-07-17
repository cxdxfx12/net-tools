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
#include <QProcess>
#include <QMutex>
#include <QAtomicInt>
#include <QMenu>
#include <QStringList>

// ============================================================================
// IPScanWorker — QRunnable 执行单个 IP 的 Ping/ARP 扫描
// ============================================================================
class IPScanWorker : public QRunnable
{
public:
    IPScanWorker(const QString& ip, int timeoutMs, QObject* receiver,
                 QAtomicInt* scanningFlag, QAtomicInt* progressCounter);

    void run() override;

private:
    QString pingIP();
    QString resolveMAC();
    QString resolveHostname();

    QString m_ip;
    int m_timeoutMs;
    QObject* m_receiver;
    QAtomicInt* m_scanning;
    QAtomicInt* m_progress;
};

// ============================================================================
// IPScannerWidget — 局域网 IP 扫描器
// ============================================================================
class IPScannerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IPScannerWidget(QWidget* parent = nullptr);
    ~IPScannerWidget() override;

signals:
    void deviceFound(QString ip, QString mac, QString hostname);
    void scanFinished(int totalFound);

public slots:
    void onResultReceived(QString ip, QString mac, QString hostname,
                          QString vendor, int responseTime, QString status);

private slots:
    void onStartScan();
    void onStopScan();
    void onExportCSV();
    void onContextMenu(const QPoint& pos);

public:
    static QString lookupOUI(const QString& mac);

private:
    void setupUI();
    void setupConnections();
    void parseIPRange(const QString& input, QStringList& outIps);
    void addResultRow(const QString& ip, const QString& mac, const QString& hostname,
                      const QString& vendor, int responseTime, const QString& status);
    void clearResults();
    static QString parseArpTable(const QString& ip);

    // --- UI 组件 ---
    QLineEdit*   m_ipRangeEdit;
    QComboBox*   m_scanMethodCombo;
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
    int          m_foundCount = 0;
};