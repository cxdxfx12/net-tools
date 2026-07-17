#pragma once

#include <QWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QList>

// ============================================================================
// VPN 隧道结构
// ============================================================================
struct VPNTunnel
{
    QString tunnelName;
    QString localAddr;
    QString remoteAddr;
    QString encAlgo;
    QString authAlgo;
    QString keyLifetime;
    QString status;       // 在线/离线/故障
    QString onlineDuration;
    QString type;         // IPSec VPN/SSL VPN/L2TP/GRE/DMVPN
    bool    isSiteToSite;
};

// ============================================================================
// IKE SA 结构
// ============================================================================
struct IKESA
{
    QString peerAddr;
    QString encAlgo;
    QString hashAlgo;
    QString dhGroup;
    QString lifetime;
    QString status;       // 已建立/协商中/已过期
    QString tunnelName;   // 关联隧道
};

// ============================================================================
// IPSec SA 结构
// ============================================================================
struct IPSecSA
{
    QString localSubnet;
    QString remoteSubnet;
    QString protocol;     // ESP/AH
    QString encAlgo;
    QString authAlgo;
    QString spi;
    QString status;       // 活跃/已过期
    QString tunnelName;   // 关联隧道
};

// ============================================================================
// VPN 在线用户结构
// ============================================================================
struct VPNUser
{
    QString username;
    QString sourceIP;
    QString associatedTunnel;
    QString loginTime;
    QString duration;
    QString traffic;
};

// ============================================================================
// VPNWidget — VPN 管理中心 (第56章)
// ============================================================================
class VPNWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VPNWidget(QWidget* parent = nullptr);
    ~VPNWidget() override;

private slots:
    void onRefresh();
    void onExport();
    void onDeviceChanged(int index);
    void onTestConnection();
    void updateTunnelStatus();

private:
    void setupUI();
    void setupConnections();
    void generateMockTunnels();
    void calculateHealthScore();

    // --- 顶部控件 ---
    QComboBox*    m_deviceCombo;
    QComboBox*    m_vpnTypeCombo;

    // --- VPN 概览卡片 ---
    QLabel*       m_totalTunnelsLabel;
    QLabel*       m_onlineTunnelsLabel;
    QLabel*       m_offlineTunnelsLabel;
    QLabel*       m_faultTunnelsLabel;

    // --- 标签页 ---
    QTableWidget* m_tunnelTable;
    QTableWidget* m_ikeTable;
    QTableWidget* m_ipsecTable;
    QTableWidget* m_userTable;

    // --- 底部按钮 ---
    QPushButton*  m_testBtn;
    QPushButton*  m_exportBtn;

    // --- 数据 ---
    QTimer*              m_refreshTimer;
    QList<VPNTunnel>     m_tunnels;
    QList<IKESA>         m_ikeSAs;
    QList<IPSecSA>       m_ipsecSAs;
    QList<VPNUser>       m_users;
};