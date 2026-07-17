#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QList>
#include <QMap>

enum class DeviceVendor {
    Unknown = 0,
    Cisco,
    Huawei,
    H3C,
    Juniper,
    Arista,
    Ruijie,
    Other
};

struct DeviceInfo {
    QString id;
    QString name;
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString protocol = "SSH";
    QString groupId;
    QString groupName;
    int vendor = 0;
    QString model;
    QString version;
    QString remark;
    bool favorite = false;
    QString createTime;
    QString updateTime;
};

struct DeviceGroupInfo {
    QString id;
    QString parentId;
    QString name;
    int sort = 0;
};

class DeviceCenter : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceCenter(QWidget* parent = nullptr);
    ~DeviceCenter() override;

    void refresh();

signals:
    void deviceDoubleClicked(const QString& host, int port,
                             const QString& username, const QString& password);
    void deviceSelected(const QString& deviceId);

private slots:
    void onSearchTextChanged(const QString& text);
    void onAddDevice();
    void onEditDevice();
    void onDeleteDevice();
    void onImportCSV();
    void onExportCSV();
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    void onTreeContextMenu(const QPoint& pos);
    void onAddGroup();
    void onDeleteGroup();
    void onToggleFavorite();

private:
    void setupUI();
    void setupConnections();
    void loadDevices();
    void loadGroups();
    void populateTree();
    void applyFilter();

    // Device CRUD
    bool insertDevice(const DeviceInfo& info);
    bool updateDevice(const DeviceInfo& info);
    bool deleteDevice(const QString& deviceId);
    QList<DeviceInfo> queryAllDevices();
    DeviceInfo queryDeviceById(const QString& deviceId);

    // Group CRUD
    bool insertGroup(const DeviceGroupInfo& info);
    bool deleteGroup(const QString& groupId);
    QList<DeviceGroupInfo> queryAllGroups();
    QString groupNameById(const QString& groupId);
    QString groupIdByName(const QString& name);

    // Helper
    QString generateId();
    QString currentTimestamp();
    void showDeviceDialog(DeviceInfo& info, bool isEdit);
    QString vendorToString(int vendor) const;
    int stringToVendor(const QString& str) const;
    QTreeWidgetItem* findGroupItem(const QString& groupId);
    QTreeWidgetItem* findDeviceItem(const QString& deviceId);

    // UI
    QLineEdit* m_searchEdit;
    QTreeWidget* m_deviceTree;
    QPushButton* m_addBtn;
    QPushButton* m_editBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_importBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_addGroupBtn;
    QPushButton* m_deleteGroupBtn;

    // Data
    QList<DeviceInfo> m_devices;
    QList<DeviceGroupInfo> m_groups;

    // Constants
    static constexpr int TREE_COL_NAME = 0;
    static constexpr int TREE_COL_HOST = 1;
    static constexpr int TREE_COL_VENDOR = 2;
    static constexpr int TREE_COL_GROUP = 3;
};