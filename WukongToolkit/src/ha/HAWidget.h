#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QTimer>
#include <QList>
#include <QString>

// ============================================================================
// HA 配置结构
// ============================================================================
struct HAConfig
{
    QString groupName;
    QString role;          // Active / Standby / Master / Backup / Member
    int priority;
    QString status;        // Active / Standby / Healthy / Warning / Failed
    QString peer;
    int heartbeatInterval; // ms
    QString haType;        // ASA-HA / HSRP / VRRP / Stack
};

// ============================================================================
// 接口状态结构
// ============================================================================
struct HAInterface
{
    QString ifName;
    QString haAddr;
    QString physAddr;
    QString status;        // Up / Down
    QString linkState;     // Link Up / Link Down
};

// ============================================================================
// 切换日志结构
// ============================================================================
struct SwitchLog
{
    QString time;
    QString reason;
    QString beforeMaster;
    QString afterMaster;
    int durationMs;
};

// ============================================================================
// HAWidget — 高可用性管理 (第59章)
// ============================================================================
class HAWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HAWidget(QWidget* parent = nullptr);
    ~HAWidget() override;

private slots:
    void onRefresh();
    void onExport();
    void onDeviceChanged(int index);
    void onSwitchTest();
    void onConfigSync();

private:
    void setupUI();
    void setupConnections();
    void updateHAStatus(int haIndex);
    void generateMockHAData();

    // --- 定时器 ---
    QTimer* m_refreshTimer;

    // --- 模拟数据 ---
    QList<HAConfig> m_haConfigs;
    QList<QList<HAInterface>> m_haInterfaces;
    QList<QList<SwitchLog>> m_haSwitchLogs;

    // --- 设备选择 ---
    QComboBox* m_deviceCombo;

    // --- HA 概览状态卡片 ---
    QLabel* m_haStatusLabel;
    QLabel* m_primaryDeviceLabel;
    QLabel* m_standbyDeviceLabel;
    QLabel* m_switchTimeLabel;
    QLabel* m_lastSwitchLabel;

    // --- 标签页 ---
    QTableWidget* m_haInfoTable;
    QTableWidget* m_deviceCompareTable;
    QTableWidget* m_ifStatusTable;
    QTableWidget* m_switchLogTable;

    // --- 操作按钮 ---
    QPushButton* m_switchTestBtn;
    QPushButton* m_configSyncBtn;
    QPushButton* m_exportBtn;
};