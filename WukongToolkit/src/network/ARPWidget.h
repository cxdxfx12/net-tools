#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QUdpSocket>
#include <QProcess>
#include <QStatusBar>

// ============================================================================
// ARPWidget — ARP 与二层诊断工具
// ============================================================================
class ARPWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ARPWidget(QWidget* parent = nullptr);
    ~ARPWidget() override;

    /// 根据 MAC 前缀查找 OUI 厂商
    static QString lookupOUI(const QString& mac);

private slots:
    // ARP 表
    void onRefreshARPTable();
    void onFilterChanged(const QString& text);
    void onExportCSV();
    void onContextMenu(const QPoint& pos);

    // ARP 扫描
    void onScanARP();

    // MAC 厂商查找
    void onLookupVendor();

    // Wake-on-LAN
    void onSendWOL();

    // 免费 ARP 检测
    void onDetectGratuitousARP();

private:
    void setupUI();
    void setupConnections();
    void parseARPOutput(const QString& output);
    void addARPRow(const QString& ip, const QString& mac,
                   const QString& iface, const QString& type);
    void clearARPTable();
    void applyFilter(const QString& text);
    void updateStatusBar();
    static QByteArray buildMagicPacket(const QString& mac);

    // ========== ARP 表区域 ==========
    QTableWidget* m_arpTable;
    QPushButton*  m_refreshBtn;
    QPushButton*  m_exportBtn;
    QLineEdit*    m_filterEdit;

    // ========== ARP 扫描区域 ==========
    QLineEdit*    m_scanRangeEdit;
    QPushButton*  m_scanBtn;

    // ========== MAC 厂商查找区域 ==========
    QLineEdit*    m_macLookupEdit;
    QPushButton*  m_lookupBtn;
    QLabel*       m_vendorResultLabel;

    // ========== Wake-on-LAN 区域 ==========
    QLineEdit*    m_wolMacEdit;
    QPushButton*  m_wolBtn;

    // ========== 免费 ARP 检测区域 ==========
    QPushButton*  m_garpDetectBtn;

    // ========== 状态栏 ==========
    QLabel*       m_statusLabel;

    // ========== 数据 ==========
    struct ARPEntry {
        QString ip;
        QString mac;
        QString iface;
        QString type; // Dynamic / Static
    };
    QList<ARPEntry> m_entries;
    QUdpSocket* m_wolSocket;
};