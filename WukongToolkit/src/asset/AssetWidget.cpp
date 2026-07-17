#include "asset/AssetWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTabWidget>
#include <QFrame>
#include <QDateTime>
#include <QRegularExpression>
#include <QApplication>

// ============================================================================
// 构造
// ============================================================================
AssetWidget::AssetWidget(QWidget* parent)
    : QWidget(parent)
    , m_totalAssetsLabel(nullptr)
    , m_expiringLabel(nullptr)
    , m_newDevicesLabel(nullptr)
    , m_maintenanceLabel(nullptr)
    , m_assetTable(nullptr)
    , m_licenseTable(nullptr)
    , m_warrantyAlertLabel(nullptr)
    , m_searchEdit(nullptr)
    , m_searchBtn(nullptr)
    , m_addBtn(nullptr)
    , m_editBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_importCsvBtn(nullptr)
    , m_exportBtn(nullptr)
{
    setupUI();
    setupConnections();
    loadDemoAssets();
    updateOverview();
    checkWarrantyExpiry();

    Logger::instance().info(QStringLiteral("Asset & License"),
        QStringLiteral("资产 & 许可证管理初始化完成，%1 个资产，%2 个许可证")
            .arg(m_assets.size()).arg(m_licenses.size()));
}

// ============================================================================
// UI 构建
// ============================================================================
void AssetWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 通用样式 lambda ────────────────────────────────────────────────

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
            QStringLiteral(
                "QTableWidget {"
                "  background-color: #0D1117; color: #E6EDF3;"
                "  border: 1px solid #30363D; font-size: 12px;"
                "  gridline-color: #21262D;"
                "}"
                "QTableWidget::item { padding: 3px 6px; }"
                "QTableWidget::item:selected { background-color: #30363D; }"
                "QHeaderView::section {"
                "  background-color: #161B22; color: #8B949E;"
                "  border: none; border-bottom: 2px solid #30363D;"
                "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
                "}"
            )
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
    };

    auto styleGroupBox = [](QGroupBox* group) {
        group->setStyleSheet(
            QStringLiteral(
                "QGroupBox {"
                "  color: #58A6FF; font-size: 13px; font-weight: bold;"
                "  border: 1px solid #30363D; border-radius: 4px; margin-top: 8px;"
                "  padding-top: 16px;"
                "}"
                "QGroupBox::title {"
                "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
                "}"
            )
        );
    };

    // ── 顶部: 资产总览 4 个彩色卡片 ────────────────────────────────────
    auto* overviewGroup = new QGroupBox(QStringLiteral("资产总览"));
    styleGroupBox(overviewGroup);
    auto* overviewLayout = new QHBoxLayout(overviewGroup);
    overviewLayout->setSpacing(12);

    auto makeCard = [](const QString& title, const QString& color) -> QLabel* {
        auto* label = new QLabel(QStringLiteral("%1\n--").arg(title));
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumHeight(70);
        label->setStyleSheet(
            QString(QStringLiteral(
                "font-size: 14px; font-weight: bold; color: %1;"
                "background-color: #161B22; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid %1;"
            )).arg(color)
        );
        return label;
    };

    m_totalAssetsLabel = makeCard(QStringLiteral("设备总数"),    QStringLiteral("#58A6FF"));
    m_expiringLabel    = makeCard(QStringLiteral("即将到期"),    QStringLiteral("#D29922"));
    m_newDevicesLabel  = makeCard(QStringLiteral("新建资产"),    QStringLiteral("#3FB950"));
    m_maintenanceLabel = makeCard(QStringLiteral("维保中"),      QStringLiteral("#8B949E"));

    overviewLayout->addWidget(m_totalAssetsLabel);
    overviewLayout->addWidget(m_expiringLabel);
    overviewLayout->addWidget(m_newDevicesLabel);
    overviewLayout->addWidget(m_maintenanceLabel);

    mainLayout->addWidget(overviewGroup);

    // ── 维保到期提醒 (红色高亮) ─────────────────────────────────────────
    m_warrantyAlertLabel = new QLabel();
    m_warrantyAlertLabel->setAlignment(Qt::AlignCenter);
    m_warrantyAlertLabel->setMinimumHeight(36);
    m_warrantyAlertLabel->setVisible(false);
    m_warrantyAlertLabel->setStyleSheet(
        QStringLiteral(
            "font-size: 13px; font-weight: bold; color: #F85149;"
            "background-color: #2D1B1B; border-radius: 4px;"
            "padding: 6px 12px; border: 1px solid #F56C6C;"
        )
    );
    mainLayout->addWidget(m_warrantyAlertLabel);

    // ── 中部标签页: 资产列表 / 许可证管理 ──────────────────────────────
    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        QStringLiteral(
            "QTabWidget::pane {"
            "  border: 1px solid #30363D; background-color: #0D1117;"
            "  border-top: 2px solid #409EFF;"
            "}"
            "QTabBar::tab {"
            "  background-color: #161B22; color: #8B949E;"
            "  padding: 8px 20px; font-size: 13px; font-weight: bold;"
            "  border: 1px solid #30363D; border-bottom: none;"
            "  border-top-left-radius: 4px; border-top-right-radius: 4px;"
            "  margin-right: 2px;"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: #0D1117; color: #58A6FF;"
            "  border-bottom: 2px solid #409EFF;"
            "}"
            "QTabBar::tab:hover {"
            "  background-color: #30363D; color: #E6EDF3;"
            "}"
        )
    );

    // 资产列表表格
    m_assetTable = new QTableWidget();
    m_assetTable->setColumnCount(9);
    m_assetTable->setHorizontalHeaderLabels({
        QStringLiteral("设备名称"),
        QStringLiteral("IP 地址"),
        QStringLiteral("设备类型"),
        QStringLiteral("厂商"),
        QStringLiteral("型号"),
        QStringLiteral("序列号"),
        QStringLiteral("购买日期"),
        QStringLiteral("维保到期"),
        QStringLiteral("状态")
    });
    m_assetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_assetTable);
    tabWidget->addTab(m_assetTable, QStringLiteral("资产列表"));

    // 许可证表格
    m_licenseTable = new QTableWidget();
    m_licenseTable->setColumnCount(6);
    m_licenseTable->setHorizontalHeaderLabels({
        QStringLiteral("许可证名称"),
        QStringLiteral("类型"),
        QStringLiteral("适用设备"),
        QStringLiteral("有效期至"),
        QStringLiteral("剩余天数"),
        QStringLiteral("状态")
    });
    m_licenseTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    styleTable(m_licenseTable);
    tabWidget->addTab(m_licenseTable, QStringLiteral("许可证管理"));

    mainLayout->addWidget(tabWidget, 1);

    // ── 底部: 搜索 + 操作按钮 ──────────────────────────────────────────
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(8);

    // 搜索框
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索设备名称、IP 或序列号..."));
    m_searchEdit->setMinimumWidth(250);
    m_searchEdit->setStyleSheet(
        QStringLiteral(
            "QLineEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 12px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QLineEdit:focus { border-color: #58A6FF; }"
        )
    );
    bottomLayout->addWidget(m_searchEdit);

    m_searchBtn = new QPushButton(QStringLiteral("搜索"));
    m_searchBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #58A6FF; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #79C0FF; }"
        )
    );
    bottomLayout->addWidget(m_searchBtn);

    bottomLayout->addSpacing(20);

    // 分隔线
    auto* separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    bottomLayout->addWidget(separator);
    bottomLayout->addSpacing(8);

    m_addBtn = new QPushButton(QStringLiteral("添加资产"));
    m_addBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #3FB950; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #56D364; }"
        )
    );
    bottomLayout->addWidget(m_addBtn);

    m_editBtn = new QPushButton(QStringLiteral("编辑资产"));
    m_editBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #D29922; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #DBAB4A; }"
        )
    );
    bottomLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(QStringLiteral("删除资产"));
    m_deleteBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #F85149; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #F89898; }"
        )
    );
    bottomLayout->addWidget(m_deleteBtn);

    bottomLayout->addStretch();

    m_importCsvBtn = new QPushButton(QStringLiteral("导入 CSV"));
    m_importCsvBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #4A4D52; color: #E6EDF3;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #5A5D62; }"
        )
    );
    bottomLayout->addWidget(m_importCsvBtn);

    m_exportBtn = new QPushButton(QStringLiteral("导出 CSV"));
    m_exportBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton { background-color: #4A4D52; color: #E6EDF3;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #5A5D62; }"
        )
    );
    bottomLayout->addWidget(m_exportBtn);

    mainLayout->addLayout(bottomLayout);
}

// ============================================================================
// 信号槽连接
// ============================================================================
void AssetWidget::setupConnections()
{
    connect(m_addBtn,      &QPushButton::clicked, this, &AssetWidget::onAddAsset);
    connect(m_editBtn,     &QPushButton::clicked, this, &AssetWidget::onEditAsset);
    connect(m_deleteBtn,   &QPushButton::clicked, this, &AssetWidget::onDeleteAsset);
    connect(m_importCsvBtn,&QPushButton::clicked, this, &AssetWidget::onImportCSV);
    connect(m_exportBtn,   &QPushButton::clicked, this, &AssetWidget::onExport);
    connect(m_searchBtn,   &QPushButton::clicked, this, &AssetWidget::onSearch);
    connect(m_searchEdit,  &QLineEdit::returnPressed, this, &AssetWidget::onSearch);
}

// ============================================================================
// 预置模拟数据: 15 个设备 + 10 个许可证
// ============================================================================
void AssetWidget::loadDemoAssets()
{
    m_assets.clear();
    m_licenses.clear();

    QDate today = QDate::currentDate();

    // ── 15 个设备 ───────────────────────────────────────────────────────

    // 1: Core-SW-01 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Core-SW-01");
        a.ip             = QStringLiteral("192.168.1.1");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("S12700E-8");
        a.serialNumber   = QStringLiteral("HW202301001");
        a.purchaseDate   = QStringLiteral("2023-03-15");
        a.warrantyExpiry = today.addDays(365).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 2: Core-SW-02 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Core-SW-02");
        a.ip             = QStringLiteral("192.168.1.2");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("S12700E-8");
        a.serialNumber   = QStringLiteral("HW202301002");
        a.purchaseDate   = QStringLiteral("2023-03-15");
        a.warrantyExpiry = today.addDays(365).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 3: Agg-SW-01 (即将到期 - 15天)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Agg-SW-01");
        a.ip             = QStringLiteral("192.168.1.10");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("思科");
        a.model          = QStringLiteral("C9500-24Y4C");
        a.serialNumber   = QStringLiteral("CISCO202401001");
        a.purchaseDate   = QStringLiteral("2024-01-20");
        a.warrantyExpiry = today.addDays(15).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 4: Agg-SW-02 (即将到期 - 7天)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Agg-SW-02");
        a.ip             = QStringLiteral("192.168.1.11");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("思科");
        a.model          = QStringLiteral("C9500-24Y4C");
        a.serialNumber   = QStringLiteral("CISCO202401002");
        a.purchaseDate   = QStringLiteral("2024-01-20");
        a.warrantyExpiry = today.addDays(7).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 5: Access-SW-01 (已过期 - 30天前)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Access-SW-01");
        a.ip             = QStringLiteral("192.168.1.20");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("H3C");
        a.model          = QStringLiteral("S5560X-54C-EI");
        a.serialNumber   = QStringLiteral("H3C202205001");
        a.purchaseDate   = QStringLiteral("2022-05-10");
        a.warrantyExpiry = today.addDays(-30).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 6: Access-SW-02 (已过期 - 60天前)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Access-SW-02");
        a.ip             = QStringLiteral("192.168.1.21");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("H3C");
        a.model          = QStringLiteral("S5560X-54C-EI");
        a.serialNumber   = QStringLiteral("H3C202205002");
        a.purchaseDate   = QStringLiteral("2022-05-10");
        a.warrantyExpiry = today.addDays(-60).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("离线");
        m_assets.append(a);
    }

    // 7: Router-GW-01 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Router-GW-01");
        a.ip             = QStringLiteral("192.168.1.254");
        a.type           = QStringLiteral("路由器");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("NetEngine 8000 M8");
        a.serialNumber   = QStringLiteral("HW202406001");
        a.purchaseDate   = QStringLiteral("2024-06-01");
        a.warrantyExpiry = today.addDays(680).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 8: Router-GW-02 (即将到期 - 5天)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Router-GW-02");
        a.ip             = QStringLiteral("192.168.1.253");
        a.type           = QStringLiteral("路由器");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("NetEngine 8000 M8");
        a.serialNumber   = QStringLiteral("HW202406002");
        a.purchaseDate   = QStringLiteral("2024-06-01");
        a.warrantyExpiry = today.addDays(5).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 9: Firewall-01 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Firewall-01");
        a.ip             = QStringLiteral("192.168.1.250");
        a.type           = QStringLiteral("防火墙");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("USG6650E");
        a.serialNumber   = QStringLiteral("HW202409001");
        a.purchaseDate   = QStringLiteral("2024-09-15");
        a.warrantyExpiry = today.addDays(790).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 10: Firewall-02 (即将到期 - 20天)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Firewall-02");
        a.ip             = QStringLiteral("192.168.1.249");
        a.type           = QStringLiteral("防火墙");
        a.vendor         = QStringLiteral("锐捷");
        a.model          = QStringLiteral("RG-WALL 1600-X8600");
        a.serialNumber   = QStringLiteral("RJ202311001");
        a.purchaseDate   = QStringLiteral("2023-11-20");
        a.warrantyExpiry = today.addDays(20).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 11: Server-DC-01 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Server-DC-01");
        a.ip             = QStringLiteral("10.0.0.10");
        a.type           = QStringLiteral("服务器");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("FusionServer 2288H V7");
        a.serialNumber   = QStringLiteral("HW202501001");
        a.purchaseDate   = QStringLiteral("2025-01-10");
        a.warrantyExpiry = today.addDays(910).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 12: Server-DC-02 (待部署)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Server-DC-02");
        a.ip             = QStringLiteral("10.0.0.11");
        a.type           = QStringLiteral("服务器");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("FusionServer 2288H V7");
        a.serialNumber   = QStringLiteral("HW202507001");
        a.purchaseDate   = QStringLiteral("2025-07-01");
        a.warrantyExpiry = today.addDays(1080).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("待部署");
        m_assets.append(a);
    }

    // 13: Wireless-AC-01 (维保中，在线)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Wireless-AC-01");
        a.ip             = QStringLiteral("192.168.2.1");
        a.type           = QStringLiteral("无线");
        a.vendor         = QStringLiteral("华为");
        a.model          = QStringLiteral("AC6805");
        a.serialNumber   = QStringLiteral("HW202408001");
        a.purchaseDate   = QStringLiteral("2024-08-05");
        a.warrantyExpiry = today.addDays(750).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // 14: Old-SW-01 (已报废)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Old-SW-01");
        a.ip             = QStringLiteral("");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("思科");
        a.model          = QStringLiteral("C2960X-48TS-L");
        a.serialNumber   = QStringLiteral("CISCO201903001");
        a.purchaseDate   = QStringLiteral("2019-03-10");
        a.warrantyExpiry = today.addDays(-800).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("已报废");
        m_assets.append(a);
    }

    // 15: Access-SW-03 (即将到期 - 3天)
    {
        AssetInfo a;
        a.deviceName     = QStringLiteral("Access-SW-03");
        a.ip             = QStringLiteral("192.168.1.22");
        a.type           = QStringLiteral("交换机");
        a.vendor         = QStringLiteral("H3C");
        a.model          = QStringLiteral("S5130S-52P-EI");
        a.serialNumber   = QStringLiteral("H3C202307001");
        a.purchaseDate   = QStringLiteral("2023-07-20");
        a.warrantyExpiry = today.addDays(3).toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = QStringLiteral("在线");
        m_assets.append(a);
    }

    // ── 10 个许可证 ─────────────────────────────────────────────────────

    // 1
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("华为 iMaster NCE 企业版");
        l.type           = QStringLiteral("企业版");
        l.device         = QStringLiteral("Core-SW-01 / Core-SW-02");
        l.expiryDate     = today.addDays(365).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 365;
        l.status         = QStringLiteral("有效");
        m_licenses.append(l);
    }

    // 2
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("Cisco DNA Advantage");
        l.type           = QStringLiteral("订阅");
        l.device         = QStringLiteral("Agg-SW-01 / Agg-SW-02");
        l.expiryDate     = today.addDays(15).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 15;
        l.status         = QStringLiteral("即将到期");
        m_licenses.append(l);
    }

    // 3
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("华为 USG 威胁防护订阅");
        l.type           = QStringLiteral("订阅");
        l.device         = QStringLiteral("Firewall-01");
        l.expiryDate     = today.addDays(790).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 790;
        l.status         = QStringLiteral("有效");
        m_licenses.append(l);
    }

    // 4
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("锐捷 RG-WALL 安全套装");
        l.type           = QStringLiteral("高级版");
        l.device         = QStringLiteral("Firewall-02");
        l.expiryDate     = today.addDays(20).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 20;
        l.status         = QStringLiteral("即将到期");
        m_licenses.append(l);
    }

    // 5
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("H3C iMC 智能管理平台");
        l.type           = QStringLiteral("永久");
        l.device         = QStringLiteral("Access-SW-01 / Access-SW-02 / Access-SW-03");
        l.expiryDate     = QStringLiteral("永久有效");
        l.remainingDays  = -1;
        l.status         = QStringLiteral("有效");
        m_licenses.append(l);
    }

    // 6
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("华为 NetEngine 基础许可");
        l.type           = QStringLiteral("基础版");
        l.device         = QStringLiteral("Router-GW-01 / Router-GW-02");
        l.expiryDate     = today.addDays(-5).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = -5;
        l.status         = QStringLiteral("已过期");
        m_licenses.append(l);
    }

    // 7
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("思科 IOS 软件服务");
        l.type           = QStringLiteral("订阅");
        l.device         = QStringLiteral("Old-SW-01");
        l.expiryDate     = today.addDays(-800).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = -800;
        l.status         = QStringLiteral("已过期");
        m_licenses.append(l);
    }

    // 8
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("华为 AC 无线接入许可");
        l.type           = QStringLiteral("企业版");
        l.device         = QStringLiteral("Wireless-AC-01");
        l.expiryDate     = today.addDays(750).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 750;
        l.status         = QStringLiteral("有效");
        m_licenses.append(l);
    }

    // 9
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("华为 FusionServer 管理许可");
        l.type           = QStringLiteral("企业版");
        l.device         = QStringLiteral("Server-DC-01");
        l.expiryDate     = today.addDays(910).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 910;
        l.status         = QStringLiteral("有效");
        m_licenses.append(l);
    }

    // 10
    {
        LicenseInfo l;
        l.licenseName    = QStringLiteral("H3C S5130 增强特性包");
        l.type           = QStringLiteral("高级版");
        l.device         = QStringLiteral("Access-SW-03");
        l.expiryDate     = today.addDays(3).toString(QStringLiteral("yyyy-MM-dd"));
        l.remainingDays  = 3;
        l.status         = QStringLiteral("即将到期");
        m_licenses.append(l);
    }

    // 填充资产表格
    for (const auto& a : m_assets) {
        int row = m_assetTable->rowCount();
        m_assetTable->insertRow(row);
        m_assetTable->setItem(row, 0, new QTableWidgetItem(a.deviceName));
        m_assetTable->setItem(row, 1, new QTableWidgetItem(a.ip));

        auto* typeItem = new QTableWidgetItem(a.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 2, typeItem);

        auto* vendorItem = new QTableWidgetItem(a.vendor);
        vendorItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 3, vendorItem);

        m_assetTable->setItem(row, 4, new QTableWidgetItem(a.model));
        m_assetTable->setItem(row, 5, new QTableWidgetItem(a.serialNumber));
        m_assetTable->setItem(row, 6, new QTableWidgetItem(a.purchaseDate));

        // 维保到期日期高亮
        auto* warrantyItem = new QTableWidgetItem(a.warrantyExpiry);
        warrantyItem->setTextAlignment(Qt::AlignCenter);
        QDate expiryDate = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
        if (expiryDate.isValid()) {
            qint64 daysToExpiry = today.daysTo(expiryDate);
            if (daysToExpiry < 0) {
                warrantyItem->setForeground(QColor(QStringLiteral("#F85149")));
            } else if (daysToExpiry <= 30) {
                warrantyItem->setForeground(QColor(QStringLiteral("#D29922")));
            } else {
                warrantyItem->setForeground(QColor(QStringLiteral("#3FB950")));
            }
        }
        m_assetTable->setItem(row, 7, warrantyItem);

        auto* statusItem = new QTableWidgetItem(a.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (a.status == QStringLiteral("在线")) {
            statusItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else if (a.status == QStringLiteral("离线")) {
            statusItem->setForeground(QColor(QStringLiteral("#F85149")));
        } else if (a.status == QStringLiteral("待部署")) {
            statusItem->setForeground(QColor(QStringLiteral("#58A6FF")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#8B949E")));
        }
        m_assetTable->setItem(row, 8, statusItem);
    }

    // 填充许可证表格
    for (const auto& l : m_licenses) {
        int row = m_licenseTable->rowCount();
        m_licenseTable->insertRow(row);
        m_licenseTable->setItem(row, 0, new QTableWidgetItem(l.licenseName));

        auto* typeItem = new QTableWidgetItem(l.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        m_licenseTable->setItem(row, 1, typeItem);

        m_licenseTable->setItem(row, 2, new QTableWidgetItem(l.device));

        auto* expiryItem = new QTableWidgetItem(l.expiryDate);
        expiryItem->setTextAlignment(Qt::AlignCenter);
        m_licenseTable->setItem(row, 3, expiryItem);

        auto* daysItem = new QTableWidgetItem();
        daysItem->setTextAlignment(Qt::AlignCenter);
        if (l.remainingDays < 0) {
            daysItem->setText(QStringLiteral("永久"));
            daysItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else {
            daysItem->setText(QString::number(l.remainingDays));
            if (l.remainingDays <= 30) {
                daysItem->setForeground(QColor(QStringLiteral("#D29922")));
            } else {
                daysItem->setForeground(QColor(QStringLiteral("#3FB950")));
            }
        }
        m_licenseTable->setItem(row, 4, daysItem);

        auto* statusItem = new QTableWidgetItem(l.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (l.status == QStringLiteral("有效")) {
            statusItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else if (l.status == QStringLiteral("即将到期")) {
            statusItem->setForeground(QColor(QStringLiteral("#D29922")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#F85149")));
        }
        m_licenseTable->setItem(row, 5, statusItem);
    }
}

// ============================================================================
// 更新资产总览
// ============================================================================
void AssetWidget::updateOverview()
{
    QDate today = QDate::currentDate();

    int totalCount = m_assets.size();
    int newCount = 0;
    int maintenanceCount = 0;
    int expiringCount = 0;

    for (const auto& a : m_assets) {
        // 新建设备：购买日期在最近 90 天内
        if (!a.purchaseDate.isEmpty()) {
            QDate purchase = QDate::fromString(a.purchaseDate, QStringLiteral("yyyy-MM-dd"));
            if (purchase.isValid() && purchase.daysTo(today) <= 90 && purchase <= today) {
                newCount++;
            }
        }

        // 维保中：维保到期日期 > 今天
        if (!a.warrantyExpiry.isEmpty()) {
            QDate expiry = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
            if (expiry.isValid() && expiry >= today) {
                maintenanceCount++;
            }
        }

        // 即将到期：维保到期日期在 30 天内
        if (!a.warrantyExpiry.isEmpty()) {
            QDate expiry = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
            if (expiry.isValid()) {
                qint64 days = today.daysTo(expiry);
                if (days >= 0 && days <= 30) {
                    expiringCount++;
                }
            }
        }
    }

    m_totalAssetsLabel->setText(QStringLiteral("设备总数\n%1").arg(totalCount));
    m_expiringLabel->setText(QStringLiteral("即将到期\n%1").arg(expiringCount));
    m_newDevicesLabel->setText(QStringLiteral("新建资产\n%1").arg(newCount));
    m_maintenanceLabel->setText(QStringLiteral("维保中\n%1").arg(maintenanceCount));

    // 即将到期卡片颜色
    if (expiringCount > 0) {
        m_expiringLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #F85149;"
                "background-color: #161B22; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #F56C6C;"
            )
        );
    } else {
        m_expiringLabel->setStyleSheet(
            QStringLiteral(
                "font-size: 14px; font-weight: bold; color: #D29922;"
                "background-color: #161B22; border-radius: 8px;"
                "padding: 10px 12px; border: 2px solid #E6A23C;"
            )
        );
    }
}

// ============================================================================
// 维保到期检测: 30 天内到期设备，红色提醒
// ============================================================================
void AssetWidget::checkWarrantyExpiry()
{
    QDate today = QDate::currentDate();
    QStringList expiringDevices;
    QStringList expiredDevices;

    for (const auto& a : m_assets) {
        if (a.warrantyExpiry.isEmpty()) continue;
        QDate expiry = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
        if (!expiry.isValid()) continue;

        qint64 days = today.daysTo(expiry);
        if (days < 0) {
            expiredDevices.append(QStringLiteral("%1 (已过期 %2 天)").arg(a.deviceName).arg(-days));
        } else if (days <= 30) {
            expiringDevices.append(QStringLiteral("%1 (剩余 %2 天)").arg(a.deviceName).arg(days));
        }
    }

    if (expiringDevices.isEmpty() && expiredDevices.isEmpty()) {
        m_warrantyAlertLabel->setVisible(false);
        return;
    }

    QString alertText;
    if (!expiringDevices.isEmpty()) {
        alertText += QStringLiteral("⚠ 维保到期提醒: ") + expiringDevices.join(QStringLiteral(" | "));
    }
    if (!expiredDevices.isEmpty()) {
        if (!alertText.isEmpty()) alertText += QStringLiteral("\n");
        alertText += QStringLiteral("‼ 已过期设备: ") + expiredDevices.join(QStringLiteral(" | "));
    }

    m_warrantyAlertLabel->setText(alertText);
    m_warrantyAlertLabel->setVisible(true);

    Logger::instance().warning(QStringLiteral("Asset & License"),
        QString(QStringLiteral("维保到期告警: %1 个即将到期, %2 个已过期"))
            .arg(expiringDevices.size()).arg(expiredDevices.size()));
}

// ============================================================================
// 添加资产
// ============================================================================
void AssetWidget::onAddAsset()
{
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("添加资产"));
    dialog->setMinimumWidth(420);
    dialog->setStyleSheet(
        QStringLiteral(
            "QDialog { background-color: #0D1117; color: #E6EDF3; }"
            "QLabel { color: #8B949E; font-size: 13px; }"
            "QLineEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QLineEdit:focus { border-color: #58A6FF; }"
            "QComboBox {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #161B22; color: #E6EDF3;"
            "  selection-background-color: #30363D;"
            "}"
            "QDateEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        )
    );

    auto* layout = new QFormLayout(dialog);
    layout->setSpacing(10);

    auto* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(QStringLiteral("如: Core-SW-03"));
    layout->addRow(QStringLiteral("设备名称:"), nameEdit);

    auto* ipEdit = new QLineEdit();
    ipEdit->setPlaceholderText(QStringLiteral("如: 192.168.1.100"));
    layout->addRow(QStringLiteral("IP 地址:"), ipEdit);

    auto* typeCombo = new QComboBox();
    typeCombo->addItems({QStringLiteral("交换机"), QStringLiteral("路由器"), QStringLiteral("防火墙"),
                         QStringLiteral("服务器"), QStringLiteral("无线"), QStringLiteral("其他")});
    layout->addRow(QStringLiteral("设备类型:"), typeCombo);

    auto* vendorCombo = new QComboBox();
    vendorCombo->addItems({QStringLiteral("华为"), QStringLiteral("思科"), QStringLiteral("H3C"),
                           QStringLiteral("锐捷"), QStringLiteral("其他")});
    layout->addRow(QStringLiteral("厂商:"), vendorCombo);

    auto* modelEdit = new QLineEdit();
    modelEdit->setPlaceholderText(QStringLiteral("如: S12700E-8"));
    layout->addRow(QStringLiteral("型号:"), modelEdit);

    auto* serialEdit = new QLineEdit();
    serialEdit->setPlaceholderText(QStringLiteral("如: HW202501003"));
    layout->addRow(QStringLiteral("序列号:"), serialEdit);

    auto* purchaseDateEdit = new QDateEdit(QDate::currentDate());
    purchaseDateEdit->setCalendarPopup(true);
    purchaseDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    layout->addRow(QStringLiteral("购买日期:"), purchaseDateEdit);

    auto* warrantyDateEdit = new QDateEdit(QDate::currentDate().addYears(1));
    warrantyDateEdit->setCalendarPopup(true);
    warrantyDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    layout->addRow(QStringLiteral("维保到期:"), warrantyDateEdit);

    auto* statusCombo = new QComboBox();
    statusCombo->addItems({QStringLiteral("在线"), QStringLiteral("离线"),
                           QStringLiteral("待部署"), QStringLiteral("已报废")});
    layout->addRow(QStringLiteral("状态:"), statusCombo);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttonBox->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  background-color: #58A6FF; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #79C0FF; }"
        )
    );
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    if (dialog->exec() == QDialog::Accepted) {
        AssetInfo a;
        a.deviceName     = nameEdit->text().trimmed();
        a.ip             = ipEdit->text().trimmed();
        a.type           = typeCombo->currentText();
        a.vendor         = vendorCombo->currentText();
        a.model          = modelEdit->text().trimmed();
        a.serialNumber   = serialEdit->text().trimmed();
        a.purchaseDate   = purchaseDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"));
        a.warrantyExpiry = warrantyDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = statusCombo->currentText();

        if (a.deviceName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("设备名称不能为空。"));
            delete dialog;
            return;
        }

        m_assets.append(a);

        // 刷新资产表格
        int row = m_assetTable->rowCount();
        m_assetTable->insertRow(row);
        m_assetTable->setItem(row, 0, new QTableWidgetItem(a.deviceName));
        m_assetTable->setItem(row, 1, new QTableWidgetItem(a.ip));
        auto* typeItem = new QTableWidgetItem(a.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 2, typeItem);
        auto* vendorItem = new QTableWidgetItem(a.vendor);
        vendorItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 3, vendorItem);
        m_assetTable->setItem(row, 4, new QTableWidgetItem(a.model));
        m_assetTable->setItem(row, 5, new QTableWidgetItem(a.serialNumber));
        m_assetTable->setItem(row, 6, new QTableWidgetItem(a.purchaseDate));

        auto* warrantyItem = new QTableWidgetItem(a.warrantyExpiry);
        warrantyItem->setTextAlignment(Qt::AlignCenter);
        QDate expiryDate = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
        if (expiryDate.isValid()) {
            qint64 daysToExpiry = QDate::currentDate().daysTo(expiryDate);
            if (daysToExpiry < 0) {
                warrantyItem->setForeground(QColor(QStringLiteral("#F85149")));
            } else if (daysToExpiry <= 30) {
                warrantyItem->setForeground(QColor(QStringLiteral("#D29922")));
            } else {
                warrantyItem->setForeground(QColor(QStringLiteral("#3FB950")));
            }
        }
        m_assetTable->setItem(row, 7, warrantyItem);

        auto* statusItem = new QTableWidgetItem(a.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (a.status == QStringLiteral("在线")) {
            statusItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else if (a.status == QStringLiteral("离线")) {
            statusItem->setForeground(QColor(QStringLiteral("#F85149")));
        } else if (a.status == QStringLiteral("待部署")) {
            statusItem->setForeground(QColor(QStringLiteral("#58A6FF")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#8B949E")));
        }
        m_assetTable->setItem(row, 8, statusItem);

        updateOverview();
        checkWarrantyExpiry();

        Logger::instance().info(QStringLiteral("Asset & License"),
            QString(QStringLiteral("资产已添加: %1 (%2)")).arg(a.deviceName, a.ip));
    }
    delete dialog;
}

// ============================================================================
// 编辑资产
// ============================================================================
void AssetWidget::onEditAsset()
{
    int currentRow = m_assetTable->currentRow();
    if (currentRow < 0 || currentRow >= m_assets.size()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要编辑的资产。"));
        return;
    }

    AssetInfo& a = m_assets[currentRow];

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("编辑资产"));
    dialog->setMinimumWidth(420);
    dialog->setStyleSheet(
        QStringLiteral(
            "QDialog { background-color: #0D1117; color: #E6EDF3; }"
            "QLabel { color: #8B949E; font-size: 13px; }"
            "QLineEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QLineEdit:focus { border-color: #58A6FF; }"
            "QComboBox {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #161B22; color: #E6EDF3;"
            "  selection-background-color: #30363D;"
            "}"
            "QDateEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 10px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        )
    );

    auto* layout = new QFormLayout(dialog);
    layout->setSpacing(10);

    auto* nameEdit = new QLineEdit(a.deviceName);
    layout->addRow(QStringLiteral("设备名称:"), nameEdit);

    auto* ipEdit = new QLineEdit(a.ip);
    layout->addRow(QStringLiteral("IP 地址:"), ipEdit);

    auto* typeCombo = new QComboBox();
    typeCombo->addItems({QStringLiteral("交换机"), QStringLiteral("路由器"), QStringLiteral("防火墙"),
                         QStringLiteral("服务器"), QStringLiteral("无线"), QStringLiteral("其他")});
    typeCombo->setCurrentText(a.type);
    layout->addRow(QStringLiteral("设备类型:"), typeCombo);

    auto* vendorCombo = new QComboBox();
    vendorCombo->addItems({QStringLiteral("华为"), QStringLiteral("思科"), QStringLiteral("H3C"),
                           QStringLiteral("锐捷"), QStringLiteral("其他")});
    vendorCombo->setCurrentText(a.vendor);
    layout->addRow(QStringLiteral("厂商:"), vendorCombo);

    auto* modelEdit = new QLineEdit(a.model);
    layout->addRow(QStringLiteral("型号:"), modelEdit);

    auto* serialEdit = new QLineEdit(a.serialNumber);
    layout->addRow(QStringLiteral("序列号:"), serialEdit);

    auto* purchaseDateEdit = new QDateEdit(QDate::fromString(a.purchaseDate, QStringLiteral("yyyy-MM-dd")));
    purchaseDateEdit->setCalendarPopup(true);
    purchaseDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    layout->addRow(QStringLiteral("购买日期:"), purchaseDateEdit);

    auto* warrantyDateEdit = new QDateEdit(QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd")));
    warrantyDateEdit->setCalendarPopup(true);
    warrantyDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    layout->addRow(QStringLiteral("维保到期:"), warrantyDateEdit);

    auto* statusCombo = new QComboBox();
    statusCombo->addItems({QStringLiteral("在线"), QStringLiteral("离线"),
                           QStringLiteral("待部署"), QStringLiteral("已报废")});
    statusCombo->setCurrentText(a.status);
    layout->addRow(QStringLiteral("状态:"), statusCombo);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttonBox->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  background-color: #58A6FF; color: white;"
            "  border: none; padding: 8px 20px; border-radius: 4px;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #79C0FF; }"
        )
    );
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    if (dialog->exec() == QDialog::Accepted) {
        QString newName = nameEdit->text().trimmed();
        if (newName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("设备名称不能为空。"));
            delete dialog;
            return;
        }

        a.deviceName     = newName;
        a.ip             = ipEdit->text().trimmed();
        a.type           = typeCombo->currentText();
        a.vendor         = vendorCombo->currentText();
        a.model          = modelEdit->text().trimmed();
        a.serialNumber   = serialEdit->text().trimmed();
        a.purchaseDate   = purchaseDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"));
        a.warrantyExpiry = warrantyDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"));
        a.status         = statusCombo->currentText();

        // 更新表格行
        m_assetTable->item(currentRow, 0)->setText(a.deviceName);
        m_assetTable->item(currentRow, 1)->setText(a.ip);
        m_assetTable->item(currentRow, 2)->setText(a.type);
        m_assetTable->item(currentRow, 3)->setText(a.vendor);
        m_assetTable->item(currentRow, 4)->setText(a.model);
        m_assetTable->item(currentRow, 5)->setText(a.serialNumber);
        m_assetTable->item(currentRow, 6)->setText(a.purchaseDate);

        auto* warrantyItem = m_assetTable->item(currentRow, 7);
        warrantyItem->setText(a.warrantyExpiry);
        QDate expiryDate = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
        if (expiryDate.isValid()) {
            qint64 daysToExpiry = QDate::currentDate().daysTo(expiryDate);
            if (daysToExpiry < 0) {
                warrantyItem->setForeground(QColor(QStringLiteral("#F85149")));
            } else if (daysToExpiry <= 30) {
                warrantyItem->setForeground(QColor(QStringLiteral("#D29922")));
            } else {
                warrantyItem->setForeground(QColor(QStringLiteral("#3FB950")));
            }
        }

        auto* statusItem = m_assetTable->item(currentRow, 8);
        statusItem->setText(a.status);
        if (a.status == QStringLiteral("在线")) {
            statusItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else if (a.status == QStringLiteral("离线")) {
            statusItem->setForeground(QColor(QStringLiteral("#F85149")));
        } else if (a.status == QStringLiteral("待部署")) {
            statusItem->setForeground(QColor(QStringLiteral("#58A6FF")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#8B949E")));
        }

        updateOverview();
        checkWarrantyExpiry();

        Logger::instance().info(QStringLiteral("Asset & License"),
            QString(QStringLiteral("资产已更新: %1")).arg(a.deviceName));
    }
    delete dialog;
}

// ============================================================================
// 删除资产
// ============================================================================
void AssetWidget::onDeleteAsset()
{
    int currentRow = m_assetTable->currentRow();
    if (currentRow < 0 || currentRow >= m_assets.size()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的资产。"));
        return;
    }

    const AssetInfo& a = m_assets.at(currentRow);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QString(QStringLiteral("确定要删除资产 \"%1\" 吗？\n此操作不可撤销。")).arg(a.deviceName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString name = a.deviceName;
        m_assets.removeAt(currentRow);
        m_assetTable->removeRow(currentRow);

        updateOverview();
        checkWarrantyExpiry();

        Logger::instance().info(QStringLiteral("Asset & License"),
            QString(QStringLiteral("资产已删除: %1")).arg(name));
    }
}

// ============================================================================
// 导入 CSV
// ============================================================================
void AssetWidget::onImportCSV()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("导入 CSV 资产文件"),
        QString(), QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导入失败"),
            QStringLiteral("无法打开文件: ") + filePath);
        return;
    }

    QTextStream in(&file);
    // 跳过 BOM
    if (!in.atEnd()) {
        QString firstLine = in.readLine();
        if (firstLine.startsWith('\xEF') || firstLine.startsWith('\xFE')) {
            // skip BOM line as header
        }
    }

    int imported = 0;
    int skipped = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // 解析 CSV（简单逗号分隔，支持引号内逗号）
        QStringList fields;
        bool inQuote = false;
        QString field;
        for (int i = 0; i < line.size(); ++i) {
            QChar c = line.at(i);
            if (c == QLatin1Char('"')) {
                inQuote = !inQuote;
            } else if (c == QLatin1Char(',') && !inQuote) {
                fields.append(field.trimmed());
                field.clear();
            } else {
                field += c;
            }
        }
        fields.append(field.trimmed());

        // 期望至少 9 列
        if (fields.size() < 9) {
            skipped++;
            continue;
        }

        AssetInfo a;
        a.deviceName     = fields[0];
        a.ip             = fields[1];
        a.type           = fields[2];
        a.vendor         = fields[3];
        a.model          = fields[4];
        a.serialNumber   = fields[5];
        a.purchaseDate   = fields[6];
        a.warrantyExpiry = fields[7];
        a.status         = fields[8];

        if (a.deviceName.isEmpty()) {
            skipped++;
            continue;
        }

        m_assets.append(a);

        // 添加到表格
        int row = m_assetTable->rowCount();
        m_assetTable->insertRow(row);
        m_assetTable->setItem(row, 0, new QTableWidgetItem(a.deviceName));
        m_assetTable->setItem(row, 1, new QTableWidgetItem(a.ip));
        auto* typeItem = new QTableWidgetItem(a.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 2, typeItem);
        auto* vendorItem = new QTableWidgetItem(a.vendor);
        vendorItem->setTextAlignment(Qt::AlignCenter);
        m_assetTable->setItem(row, 3, vendorItem);
        m_assetTable->setItem(row, 4, new QTableWidgetItem(a.model));
        m_assetTable->setItem(row, 5, new QTableWidgetItem(a.serialNumber));
        m_assetTable->setItem(row, 6, new QTableWidgetItem(a.purchaseDate));

        auto* warrantyItem = new QTableWidgetItem(a.warrantyExpiry);
        warrantyItem->setTextAlignment(Qt::AlignCenter);
        QDate expiryDate = QDate::fromString(a.warrantyExpiry, QStringLiteral("yyyy-MM-dd"));
        if (expiryDate.isValid()) {
            qint64 daysToExpiry = QDate::currentDate().daysTo(expiryDate);
            if (daysToExpiry < 0) {
                warrantyItem->setForeground(QColor(QStringLiteral("#F85149")));
            } else if (daysToExpiry <= 30) {
                warrantyItem->setForeground(QColor(QStringLiteral("#D29922")));
            } else {
                warrantyItem->setForeground(QColor(QStringLiteral("#3FB950")));
            }
        }
        m_assetTable->setItem(row, 7, warrantyItem);

        auto* statusItem = new QTableWidgetItem(a.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (a.status == QStringLiteral("在线")) {
            statusItem->setForeground(QColor(QStringLiteral("#3FB950")));
        } else if (a.status == QStringLiteral("离线")) {
            statusItem->setForeground(QColor(QStringLiteral("#F85149")));
        } else if (a.status == QStringLiteral("待部署")) {
            statusItem->setForeground(QColor(QStringLiteral("#58A6FF")));
        } else {
            statusItem->setForeground(QColor(QStringLiteral("#8B949E")));
        }
        m_assetTable->setItem(row, 8, statusItem);

        imported++;
    }

    file.close();

    updateOverview();
    checkWarrantyExpiry();

    Logger::instance().info(QStringLiteral("Asset & License"),
        QString(QStringLiteral("CSV 导入完成: %1 条成功, %2 条跳过")).arg(imported).arg(skipped));
    QMessageBox::information(this, QStringLiteral("导入完成"),
        QString(QStringLiteral("成功导入 %1 条资产\n跳过 %2 条无效行")).arg(imported).arg(skipped));
}

// ============================================================================
// 导出 CSV
// ============================================================================
void AssetWidget::onExport()
{
    if (m_assets.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("没有可导出的资产数据。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出资产列表"),
        QString(QStringLiteral("asset_list_%1.csv"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法写入文件: ") + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF"); // BOM for Excel UTF-8
    QTextStream out(&file);

    auto csvEscape = [](const QString& val) -> QString {
        QString v = val;
        if (v.contains(',') || v.contains('\n') || v.contains('"')) {
            v.replace("\"", "\"\"");
            v = "\"" + v + "\"";
        }
        return v;
    };

    // 资产列表
    out << QStringLiteral("=== 资产列表 ===\n");
    out << QStringLiteral("设备名称,IP 地址,设备类型,厂商,型号,序列号,购买日期,维保到期,状态\n");
    for (const auto& a : m_assets) {
        out << csvEscape(a.deviceName)     << ","
            << csvEscape(a.ip)             << ","
            << csvEscape(a.type)           << ","
            << csvEscape(a.vendor)         << ","
            << csvEscape(a.model)          << ","
            << csvEscape(a.serialNumber)   << ","
            << csvEscape(a.purchaseDate)   << ","
            << csvEscape(a.warrantyExpiry) << ","
            << csvEscape(a.status)         << "\n";
    }

    // 许可证列表
    out << QStringLiteral("\n=== 许可证列表 ===\n");
    out << QStringLiteral("许可证名称,类型,适用设备,有效期至,剩余天数,状态\n");
    for (const auto& l : m_licenses) {
        out << csvEscape(l.licenseName) << ","
            << csvEscape(l.type)        << ","
            << csvEscape(l.device)      << ","
            << csvEscape(l.expiryDate)  << ","
            << (l.remainingDays < 0 ? QStringLiteral("永久") : QString::number(l.remainingDays)) << ","
            << csvEscape(l.status)      << "\n";
    }

    file.close();
    Logger::instance().info(QStringLiteral("Asset & License"),
        QString(QStringLiteral("资产列表已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("资产列表已保存到:\n%1")).arg(filePath));
}

// ============================================================================
// 搜索
// ============================================================================
void AssetWidget::onSearch()
{
    QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        // 显示所有行
        for (int row = 0; row < m_assetTable->rowCount(); ++row) {
            m_assetTable->setRowHidden(row, false);
        }
        return;
    }

    for (int row = 0; row < m_assetTable->rowCount(); ++row) {
        bool match = false;
        // 在设备名称、IP、序列号中搜索
        for (int col : {0, 1, 5}) {
            QTableWidgetItem* item = m_assetTable->item(row, col);
            if (item && item->text().contains(keyword, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_assetTable->setRowHidden(row, !match);
    }

    Logger::instance().debug(QStringLiteral("Asset & License"),
        QString(QStringLiteral("搜索: \"%1\"")).arg(keyword));
}