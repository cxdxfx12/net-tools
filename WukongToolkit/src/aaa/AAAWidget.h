#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QList>
#include <QString>

// ============================================================================
// AAA 数据结构
// ============================================================================

struct RadiusServer
{
    QString ip;
    int port;
    int timeout;
    int retry;
    QString role;        // 主 / 备
    QString status;      // Active / Inactive / Unreachable
    QString reachability; // 可达 / 不可达
};

struct TacacsServer
{
    QString ip;
    int port;
    QString keyStatus;   // 已配置 / 未配置 / 已加密
    QString role;        // 主 / 备
    int timeout;
    QString authStatus;  // 认证成功 / 认证失败 / 未测试
};

struct LocalAccount
{
    QString username;
    int privilegeLevel;   // 0-15
    QString lastLogin;
    QString status;       // 活跃 / 锁定 / 未使用
    QString expiryDate;
};

struct LoginRecord
{
    QString user;
    QString sourceIp;
    QString loginTime;
    QString result;       // 成功 / 失败
    QString method;       // SSH / Telnet / Console
};

struct RiskItem
{
    QString riskName;
    QString severity;     // 高 / 中 / 低
    QString device;
    QString detail;
};

struct AAAConfig
{
    QString deviceName;
    QString aaaStatus;     // 已启用 / 未启用
    QString sshStatus;     // 已启用 / 未启用
    QString telnetStatus;  // 已启用 / 未启用
    QString authMethod;    // Local / RADIUS / TACACS+ / 混合
    int healthScore;
    QList<RadiusServer> radiusServers;
    QList<TacacsServer> tacacsServers;
    QList<LocalAccount> localAccounts;
    QList<LoginRecord> loginRecords;
    QList<RiskItem> riskItems;
};

// ============================================================================
// AAAWidget — AAA & Authentication Center (第54章)
// ============================================================================
class AAAWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AAAWidget(QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onExport();
    void onDeviceChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void checkAAAStatus();
    void checkRadius();
    void checkTacacs();
    void checkAccounts();
    void checkLoginStats();
    void calculateHealthScore();

    // --- 设备选择 ---
    QComboBox* m_deviceCombo;

    // --- 认证概览区 ---
    QLabel* m_aaaStatusLabel;
    QLabel* m_sshStatusLabel;
    QLabel* m_telnetStatusLabel;
    QLabel* m_authMethodLabel;
    QLabel* m_healthScoreLabel;

    // --- 表格 ---
    QTableWidget* m_radiusTable;
    QTableWidget* m_tacacsTable;
    QTableWidget* m_accountTable;
    QTableWidget* m_loginStatsTable;
    QTableWidget* m_riskTable;

    // --- 导出按钮 ---
    QPushButton* m_exportBtn;

    // --- 定时器 ---
    QTimer* m_refreshTimer;

    // --- 数据 ---
    QList<AAAConfig> m_configs;
};