#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QProcess>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QNetworkInterface>

// ============================================================================
// DHCP 发现响应结构
// ============================================================================
struct DHCPOffer
{
    QString serverIP;
    QString offeredIP;
    QString subnetMask;
    QString gateway;
    QString dns;
    QString leaseTime;
    QString interfaceName;
    QDateTime time;
};

// ============================================================================
// DHCP 服务器监视记录
// ============================================================================
struct DHCPServerRecord
{
    QDateTime time;
    QString clientMAC;
    QString requestType;   // DISCOVER / REQUEST / RELEASE
    QString serverIP;
    QString offeredIP;
    QString interfaceName;
};

// ============================================================================
// DHCP 租约信息
// ============================================================================
struct DHCPLease
{
    QString interfaceName;
    QString ipAddress;
    QString subnetMask;
    QString router;
    QString dns;
    QString leaseStart;
    QString leaseEnd;
    QString serverIP;
};

// ============================================================================
// DHCPWidget — DHCP 工具集 (Chapter 29)
// ============================================================================
class DHCPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DHCPWidget(QWidget* parent = nullptr);
    ~DHCPWidget() override;

private slots:
    // ── Tab 1: DHCP Discover ──
    void onDiscover();
    void onDiscoverProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // ── Tab 2: DHCP Server Monitor ──
    void onServerStartStop();
    void onServerClear();
    void onServerUdpReadyRead();
    void onServerExportCSV();

    // ── Tab 3: Lease Viewer ──
    void onRefreshLease();
    void onLeaseExportCSV();

    // ── Export ──
    void onDiscoverExportCSV();

private:
    void setupUI();
    void setupConnections();
    void populateInterfaces();

    // ── Tab 1: DHCP Discover 辅助 ──
    void addDiscoverRow(const DHCPOffer& offer);
    void clearDiscoverTable();
    void parseDiscoverOutput(const QString& output, const QString& iface);

    // ── Tab 2: DHCP Server Monitor 辅助 ──
    void addServerRow(const DHCPServerRecord& record);
    void parseDHCPPacket(const QByteArray& data, const QString& sourceIP);
    void updateServerStatus(bool running);

    // ── Tab 3: Lease Viewer 辅助 ──
    void addLeaseRow(const DHCPLease& lease);
    void clearLeaseTable();
    void parseLeaseOutput(const QString& output);

    // ── 通用导出 ──
    void exportTableToCSV(QTableWidget* table, const QString& title, const QStringList& headers);

    // ========== Tab 1: DHCP Discover ==========
    QComboBox*     m_discoverInterfaceCombo;
    QPushButton*   m_discoverBtn;
    QPushButton*   m_discoverExportBtn;
    QTableWidget*  m_discoverTable;
    QLabel*        m_discoverStatusLabel;
    QProcess*      m_discoverProcess;

    // ========== Tab 2: DHCP Server Monitor ==========
    QPushButton*   m_serverStartStopBtn;
    QPushButton*   m_serverClearBtn;
    QPushButton*   m_serverExportBtn;
    QTableWidget*  m_serverTable;
    QLabel*        m_serverStatusLabel;
    QLabel*        m_serverCountLabel;
    QUdpSocket*    m_serverSocket;
    bool           m_serverRunning;

    // ========== Tab 3: Lease Viewer ==========
    QComboBox*     m_leaseInterfaceCombo;
    QPushButton*   m_leaseRefreshBtn;
    QPushButton*   m_leaseExportBtn;
    QTableWidget*  m_leaseTable;
    QLabel*        m_leaseStatusLabel;
    QProcess*      m_leaseProcess;
};