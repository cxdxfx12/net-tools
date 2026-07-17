#pragma once

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QList>
#include <QString>

// ============================================================================
// 资产信息结构
// ============================================================================

struct AssetInfo
{
    QString deviceName;
    QString ip;
    QString type;           // 交换机 / 路由器 / 防火墙 / 服务器 / 无线 / 其他
    QString vendor;         // 华为 / 思科 / H3C / 锐捷 / 其他
    QString model;
    QString serialNumber;
    QString purchaseDate;
    QString warrantyExpiry; // 维保到期日
    QString status;         // 在线 / 离线 / 已报废 / 待部署
};

struct LicenseInfo
{
    QString licenseName;
    QString type;           // 企业版 / 高级版 / 基础版 / 订阅 / 永久
    QString device;
    QString expiryDate;
    int remainingDays;
    QString status;         // 有效 / 即将到期 / 已过期
};

// ============================================================================
// AssetWidget — 资产 & 许可证管理 (第58章)
// ============================================================================
class AssetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AssetWidget(QWidget* parent = nullptr);

private slots:
    void onAddAsset();
    void onEditAsset();
    void onDeleteAsset();
    void onImportCSV();
    void onExport();
    void onSearch();

private:
    void setupUI();
    void setupConnections();
    void updateOverview();
    void checkWarrantyExpiry();
    void loadDemoAssets();

    // --- 资产总览区 ---
    QLabel* m_totalAssetsLabel;
    QLabel* m_expiringLabel;
    QLabel* m_newDevicesLabel;
    QLabel* m_maintenanceLabel;

    // --- 资产列表 ---
    QTableWidget* m_assetTable;

    // --- 许可证表格 ---
    QTableWidget* m_licenseTable;

    // --- 维保到期提醒 ---
    QLabel* m_warrantyAlertLabel;

    // --- 搜索 ---
    QLineEdit* m_searchEdit;
    QPushButton* m_searchBtn;

    // --- 操作按钮 ---
    QPushButton* m_addBtn;
    QPushButton* m_editBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_importCsvBtn;
    QPushButton* m_exportBtn;

    // --- 数据 ---
    QList<AssetInfo> m_assets;
    QList<LicenseInfo> m_licenses;
};