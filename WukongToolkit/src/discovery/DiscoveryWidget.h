#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>
#include <QTimer>
#include <QStringList>
#include <QElapsedTimer>

// ============================================================================
// DiscoveryWidget — 设备自动发现模块 (第44章)
// ============================================================================
class DiscoveryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiscoveryWidget(QWidget* parent = nullptr);
    ~DiscoveryWidget() override;

    struct DiscoveryResult {
        QString ip;
        QString hostname;
        QString mac;
        QString vendor;
        QString deviceType;
        QString os;
        QString method;
    };

private slots:
    void onStartScan();
    void onStopScan();
    void onPauseResume();
    void onAddToDeviceCenter();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void scanIP(const QString& ip);
    void icmpPing(const QString& ip);
    void arpScan();
    void tcpPortScan(const QString& ip);
    void snmpDiscover(const QString& ip);
    void addDiscoveryResult(const QString& ip, const QString& hostname, const QString& mac,
                            const QString& vendor, const QString& deviceType, const QString& os,
                            const QString& method);
    void addScanHistory(const QString& target, const QString& method, int found, double elapsed);
    QString lookupVendor(const QString& mac);
    QString detectDeviceType(const QString& hostname, const QString& mac);
    QStringList parseIPRange(const QString& input);

    // --- UI 组件 ---
    QLineEdit*    m_targetEdit;
    QLineEdit*    m_ipRangeEdit;
    QComboBox*    m_methodCombo;
    QSpinBox*     m_threadCountSpin;
    QSpinBox*     m_timeoutSpin;
    QPushButton*  m_startBtn;
    QPushButton*  m_stopBtn;
    QPushButton*  m_pauseBtn;
    QProgressBar* m_progressBar;
    QLabel*       m_statusLabel;
    QTableWidget* m_resultTable;
    QCheckBox*    m_autoCreateCheck;
    QPushButton*  m_addDeviceBtn;
    QTableWidget* m_historyTable;
    QPushButton*  m_exportBtn;

    // --- 扫描状态 ---
    QTimer*       m_scanTimer;
    QStringList   m_pendingIPs;
    int           m_currentIndex = 0;
    bool          m_isRunning = false;
    bool          m_isPaused = false;
    int           m_foundCount = 0;
    int           m_scannedCount = 0;
    QElapsedTimer m_elapsed;
};