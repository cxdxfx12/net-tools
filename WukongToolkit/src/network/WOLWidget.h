#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QUdpSocket>
#include <QCheckBox>
#include <QList>

struct WOLDeviceInfo
{
    QString id;
    QString name;
    QString mac;
    QString ip;
    QString lastWoken;
    int status = 0; // 0=unknown, 1=success, 2=failed
    QString createTime;
    QString updateTime;
};

struct WOLHistoryItem
{
    int id = 0;
    QString name;
    QString mac;
    QString targetIp;
    QString broadcastAddr;
    int port = 9;
    QString sendTime;
    int status = 0; // 0=发送中, 1=成功, 2=失败
    QString errorMsg;
};

class WOLWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WOLWidget(QWidget* parent = nullptr);
    ~WOLWidget() override;

private slots:
    void onSendWOL();
    void onAddDevice();
    void onEditDevice();
    void onDeleteDevice();
    void onDeviceTableDoubleClicked(int row, int column);
    void onDeviceTableSelectionChanged();
    void onClearHistory();
    void onExportHistory();
    void onHistoryTableDoubleClicked(int row, int column);

private:
    void setupUI();
    void setupConnections();
    void createTables();
    void loadDevices();
    void loadHistory();
    void refreshDeviceTable();
    void refreshHistoryTable();
    void sendMagicPacket(const QString& mac, const QString& broadcastAddr, int port,
                         const QString& deviceName = QString());
    QByteArray buildMagicPacket(const QString& mac) const;
    QStringList parseMacAddress(const QString& mac) const;
    QString normalizeMac(const QString& mac) const;
    QStringList getSubnetBroadcasts(const QString& baseBroadcast) const;
    void showDeviceDialog(WOLDeviceInfo& info, bool isEdit);
    void updateDeviceStatus(const QString& deviceId, int status);

    QString generateId();
    QString currentTimestamp();
    bool insertDevice(const WOLDeviceInfo& info);
    bool updateDevice(const WOLDeviceInfo& info);
    bool deleteDevice(const QString& deviceId);
    QList<WOLDeviceInfo> queryAllDevices();
    WOLDeviceInfo queryDeviceById(const QString& deviceId);
    WOLDeviceInfo queryDeviceByMac(const QString& mac);
    bool insertHistory(const WOLHistoryItem& item);
    QList<WOLHistoryItem> queryAllHistory();
    bool clearHistory();

    // UI - Send section
    QLineEdit*      m_macEdit;
    QLineEdit*      m_broadcastEdit;
    QSpinBox*       m_portSpin;
    QPushButton*    m_sendBtn;
    QLabel*         m_statusLabel;
    QCheckBox*      m_multiSubnetCheck;

    // UI - Device list section
    QTableWidget*   m_deviceTable;
    QPushButton*    m_addDeviceBtn;
    QPushButton*    m_editDeviceBtn;
    QPushButton*    m_deleteDeviceBtn;

    // UI - History section
    QTableWidget*   m_historyTable;
    QPushButton*    m_clearHistoryBtn;
    QPushButton*    m_exportHistoryBtn;

    // Network
    QUdpSocket*     m_socket;

    // Data
    QList<WOLDeviceInfo> m_devices;
    QList<WOLHistoryItem> m_history;

    // Table columns
    static constexpr int DEV_COL_NAME = 0;
    static constexpr int DEV_COL_MAC = 1;
    static constexpr int DEV_COL_IP = 2;
    static constexpr int DEV_COL_LAST_WOKEN = 3;
    static constexpr int DEV_COL_STATUS = 4;

    static constexpr int HIST_COL_NAME = 0;
    static constexpr int HIST_COL_MAC = 1;
    static constexpr int HIST_COL_TARGET_IP = 2;
    static constexpr int HIST_COL_BROADCAST = 3;
    static constexpr int HIST_COL_PORT = 4;
    static constexpr int HIST_COL_TIME = 5;
    static constexpr int HIST_COL_STATUS = 6;
};