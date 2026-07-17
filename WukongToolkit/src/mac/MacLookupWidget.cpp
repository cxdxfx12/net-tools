#include "mac/MacLookupWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
#include <QInputDialog>

// ============================================================================
// 内置 OUI 厂商数据库 — 精选常见厂商 (与 MacToolkitWidget 共享)
// ============================================================================
static const QHash<QString, QString>& ouiDatabase()
{
    static const QHash<QString, QString> db = {
        // 网络设备厂商
        {"00:00:0C", "Cisco Systems"},   {"00:01:42", "Cisco Systems"},
        {"00:1A:A1", "Cisco Systems"},   {"00:1E:49", "Cisco Systems"},
        {"00:23:04", "Cisco Systems"},   {"04:DA:D2", "Cisco Systems"},
        {"00:0D:6E", "Huawei Technologies"}, {"00:18:82", "Huawei Technologies"},
        {"00:1E:10", "Huawei Technologies"}, {"00:E0:FC", "Huawei Technologies"},
        {"00:0F:E2", "H3C Technologies"},    {"00:1B:2F", "H3C Technologies"},
        {"00:23:89", "H3C Technologies"},    {"0C:DA:41", "H3C Technologies"},
        {"00:05:85", "Juniper Networks"},    {"00:12:1E", "Juniper Networks"},
        {"00:1F:12", "Juniper Networks"},    {"00:23:9C", "Juniper Networks"},
        {"00:09:0F", "Fortinet"},            {"08:5B:0E", "Fortinet"},
        {"00:1C:73", "Arista Networks"},     {"00:25:90", "Arista Networks"},
        {"00:15:6D", "Ubiquiti Networks"},   {"04:18:D6", "Ubiquiti Networks"},
        {"24:A4:3C", "Ubiquiti Networks"},
        {"00:0C:42", "TP-Link Technologies"},{"00:1A:70", "TP-Link Technologies"},
        {"14:CC:20", "TP-Link Technologies"},{"50:C7:BF", "TP-Link Technologies"},
        {"00:14:6C", "Netgear"},             {"00:1F:33", "Netgear"},
        {"04:A1:51", "Netgear"},             {"2C:30:33", "Netgear"},
        {"00:0D:88", "D-Link Corporation"},  {"00:1B:11", "D-Link Corporation"},
        {"1C:5F:2B", "D-Link Corporation"},
        // 服务器/PC 厂商
        {"00:03:93", "Apple Inc."},          {"00:1E:C2", "Apple Inc."},
        {"00:23:DF", "Apple Inc."},          {"00:25:BC", "Apple Inc."},
        {"00:26:BB", "Apple Inc."},          {"04:0C:CE", "Apple Inc."},
        {"00:02:B3", "Intel Corporation"},   {"00:0C:F1", "Intel Corporation"},
        {"00:1E:64", "Intel Corporation"},   {"00:21:5C", "Intel Corporation"},
        {"00:06:5B", "Dell Inc."},           {"00:14:22", "Dell Inc."},
        {"00:1E:C9", "Dell Inc."},           {"00:24:E8", "Dell Inc."},
        {"00:02:A5", "HP Inc."},             {"00:11:85", "HP Inc."},
        {"00:1E:0B", "HP Inc."},             {"00:24:81", "HP Inc."},
        {"00:00:4E", "Lenovo Group"},        {"00:0A:E4", "Lenovo Group"},
        {"00:16:41", "Lenovo Group"},        {"00:24:7E", "Lenovo Group"},
        {"00:1A:8C", "Sony Corporation"},    {"00:24:BE", "Sony Corporation"},
        {"00:00:F0", "Samsung Electronics"}, {"00:15:99", "Samsung Electronics"},
        {"00:23:39", "Samsung Electronics"},
        {"04:5A:95", "Xiaomi Communications"},{"08:10:74", "Xiaomi Communications"},
        {"34:CD:6D", "Xiaomi Communications"},{"64:09:80", "Xiaomi Communications"},
        // 虚拟化
        {"00:0C:29", "VMware Inc."},         {"00:50:56", "VMware Inc."},
        {"00:05:69", "VMware Inc."},         {"08:00:27", "Oracle VirtualBox"},
        {"00:15:5D", "Microsoft Hyper-V"},   {"00:16:3E", "Xen/Citrix"},
        {"52:54:00", "QEMU/KVM"},
        // 芯片厂商
        {"00:10:18", "Broadcom"},            {"00:16:B4", "Broadcom"},
        {"00:14:D1", "Realtek Semiconductor"},{"00:E0:4C", "Realtek Semiconductor"},
        {"00:04:4B", "NVIDIA Corporation"},  {"00:1A:64", "NVIDIA Corporation"},
        // 其他常见
        {"00:00:0E", "Fujitsu"},             {"00:00:1A", "AMD"},
        {"00:00:1B", "Novell"},              {"00:00:1D", "Cabletron"},
        {"00:00:4C", "NEC Corporation"},     {"00:00:5E", "ICANN/IANA"},
        {"00:00:6C", "Foxconn"},             {"00:00:AA", "Xerox Corporation"},
        {"00:00:C0", "Western Digital"},     {"00:00:C9", "Emulex"},
        {"00:00:E2", "Acer"},                {"00:00:F8", "Samsung (legacy)"},
        {"00:00:FE", "Nokia"},               {"00:01:63", "Nokia"},
        {"00:02:16", "Cisco (legacy)"},      {"00:02:3F", "Compal"},
        {"00:02:55", "IBM"},                 {"00:02:8A", "AMBIT Micro"},
        {"00:02:A1", "World Wide Packets"},  {"00:02:B3", "Intel"},
        {"00:02:E3", "Lite-On"},             {"00:03:1A", "Hewlett-Packard"},
        {"00:03:47", "Intel"},               {"00:03:52", "Asus"},
        {"00:03:6B", "Cisco"},               {"00:03:BA", "Oracle/Sun"},
        {"00:03:D9", "NEC"},                 {"00:03:F2", "MediaTek"},
        {"00:04:0B", "3Com"},                {"00:04:13", "Snom Technology"},
        {"00:04:23", "Intel"},               {"00:04:5A", "Linksys"},
        {"00:04:76", "3Com"},                {"00:04:96", "Extreme Networks"},
        {"00:05:1E", "Brocade"},             {"00:05:4E", "Alcatel"},
        {"00:05:9A", "Cisco"},               {"00:05:DC", "Cisco"},
        {"00:06:29", "IBM"},                 {"00:07:4C", "Zyxel"},
        {"00:07:95", "Netgear"},             {"00:07:E9", "Intel"},
        {"00:08:02", "HP"},                  {"00:08:74", "Dell"},
        {"00:09:6B", "IBM"},                 {"00:0A:0D", "Intel"},
        {"00:0A:F7", "Broadcom"},            {"00:0B:6A", "Cisco"},
        {"00:0B:82", "Grandstream"},         {"00:0B:AB", "Intel"},
        {"00:0B:DB", "Dell"},                {"00:0C:29", "VMware"},
        {"00:0D:56", "Dell"},                {"00:0D:60", "Intel"},
        {"00:0D:6E", "Huawei"},              {"00:0D:88", "D-Link"},
        {"00:0D:B4", "Cisco"},               {"00:0D:E5", "Samsung"},
        {"00:0E:0C", "Intel"},               {"00:0E:7F", "HP"},
        {"00:0F:1F", "Dell"},                {"00:0F:23", "Cisco"},
        {"00:0F:B0", "Lenovo"},              {"00:0F:FE", "Cisco"},
        {"00:10:83", "HP"},                  {"00:10:DB", "Juniper"},
        {"00:11:0A", "HP"},                  {"00:11:11", "Intel"},
        {"00:11:25", "IBM/Lenovo"},          {"00:11:43", "Dell"},
        {"00:11:85", "HP"},                  {"00:11:95", "D-Link"},
        {"00:12:3F", "Dell"},                {"00:12:47", "Samsung"},
        {"00:12:79", "Cisco"},               {"00:12:F0", "Intel"},
        {"00:13:02", "Intel"},               {"00:13:21", "HP"},
        {"00:13:46", "D-Link"},              {"00:13:72", "Dell"},
        {"00:13:8F", "Cisco"},               {"00:14:38", "HP"},
        {"00:14:4F", "NVIDIA"},              {"00:14:6C", "Netgear"},
        {"00:14:7C", "Cisco"},               {"00:14:BF", "Cisco"},
        {"00:14:D1", "Realtek"},             {"00:14:F6", "Juniper"},
        {"00:15:00", "Intel"},               {"00:15:2B", "Cisco"},
        {"00:15:58", "Lenovo"},              {"00:15:60", "HP"},
        {"00:15:6D", "Ubiquiti"},            {"00:15:6F", "Cisco"},
        {"00:15:99", "Samsung"},             {"00:15:C5", "Dell"},
        {"00:16:35", "HP"},                  {"00:16:3E", "Xen"},
        {"00:16:41", "Intel"},               {"00:16:6B", "Samsung"},
        {"00:16:B4", "Broadcom"},            {"00:16:C7", "Cisco"},
        {"00:16:DB", "Samsung"},             {"00:16:EA", "Intel"},
        {"00:17:9A", "D-Link"},              {"00:17:A4", "HP"},
        {"00:17:C9", "Samsung"},             {"00:17:CB", "Juniper"},
        {"00:17:F2", "Apple"},               {"00:17:FA", "Microsoft"},
        {"00:18:0A", "Broadcom"},            {"00:18:4D", "Netgear"},
        {"00:18:82", "Huawei"},              {"00:18:8B", "Dell"},
        {"00:18:AF", "Samsung"},             {"00:18:DE", "Intel"},
        {"00:18:FE", "HP"},                  {"00:19:5B", "Cisco"},
        {"00:19:B9", "Dell"},                {"00:19:BB", "HP"},
        {"00:19:D1", "Intel"},               {"00:19:E0", "TP-Link"},
        {"00:19:E2", "Juniper"},             {"00:1A:4A", "Citrix"},
        {"00:1A:4B", "HP"},                  {"00:1A:64", "NVIDIA"},
        {"00:1A:6B", "Intel"},               {"00:1A:70", "TP-Link"},
        {"00:1A:73", "D-Link"},              {"00:1A:8A", "Samsung"},
        {"00:1A:8C", "Sony"},                {"00:1A:9F", "Cisco"},
        {"00:1A:A0", "Dell"},                {"00:1A:A1", "Cisco"},
        {"00:1A:C4", "Juniper"},             {"00:1B:0C", "Cisco"},
        {"00:1B:11", "D-Link"},              {"00:1B:21", "Intel"},
        {"00:1B:2F", "H3C"},                 {"00:1B:59", "Broadcom"},
        {"00:1B:6F", "NVIDIA"},              {"00:1B:77", "Intel"},
        {"00:1B:78", "HP"},                  {"00:1B:98", "Samsung"},
        {"00:1B:C0", "Juniper"},             {"00:1C:14", "VMware"},
        {"00:1C:23", "Dell"},                {"00:1C:25", "Lenovo"},
        {"00:1C:42", "Parallels"},           {"00:1C:43", "Samsung"},
        {"00:1C:73", "Arista"},              {"00:1C:7E", "D-Link"},
        {"00:1C:BF", "Intel"},               {"00:1C:C4", "HP"},
        {"00:1D:09", "Dell"},                {"00:1D:0D", "HP"},
        {"00:1D:0F", "TP-Link"},             {"00:1D:25", "Samsung"},
        {"00:1D:4F", "Broadcom"},            {"00:1D:72", "Cisco"},
        {"00:1D:D8", "Microsoft"},           {"00:1D:F6", "Samsung"},
        {"00:1E:0B", "HP"},                  {"00:1E:10", "Huawei"},
        {"00:1E:37", "Lenovo"},              {"00:1E:4F", "Dell"},
        {"00:1E:49", "Cisco"},               {"00:1E:58", "D-Link"},
        {"00:1E:64", "Intel"},               {"00:1E:68", "NVIDIA"},
        {"00:1E:7B", "Broadcom"},            {"00:1E:7D", "Samsung"},
        {"00:1E:C2", "Apple"},               {"00:1E:C9", "Dell"},
        {"00:1F:12", "Juniper"},             {"00:1F:29", "HP"},
        {"00:1F:33", "Netgear"},             {"00:1F:3B", "Intel"},
        {"00:1F:9E", "Cisco"},               {"00:1F:CC", "Samsung"},
        {"00:1F:F3", "Apple"},               {"00:20:6B", "Cisco"},
        {"00:21:19", "Samsung"},             {"00:21:27", "TP-Link"},
        {"00:21:55", "Cisco"},               {"00:21:5A", "HP"},
        {"00:21:5C", "Intel"},               {"00:21:63", "Broadcom"},
        {"00:21:6A", "Apple"},               {"00:21:70", "Dell"},
        {"00:21:9B", "Dell"},                {"00:21:D7", "Cisco"},
        {"00:21:F6", "Oracle VM"},           {"00:21:FE", "HP"},
        {"00:22:15", "Lenovo"},              {"00:22:19", "Dell"},
        {"00:22:3F", "Netgear"},             {"00:22:41", "Apple"},
        {"00:22:48", "Microsoft"},           {"00:22:55", "Cisco"},
        {"00:22:64", "HP"},                  {"00:22:67", "NVIDIA"},
        {"00:22:83", "Juniper"},             {"00:22:B0", "D-Link"},
        {"00:22:BD", "Cisco"},               {"00:22:DE", "Broadcom"},
        {"00:23:04", "Cisco"},               {"00:23:12", "Apple"},
        {"00:23:13", "Intel"},               {"00:23:39", "Samsung"},
        {"00:23:7D", "HP"},                  {"00:23:89", "H3C"},
        {"00:23:9C", "Juniper"},             {"00:23:AE", "Dell"},
        {"00:23:CD", "TP-Link"},             {"00:23:DF", "Apple"},
        {"00:23:EB", "Cisco"},               {"00:24:01", "D-Link"},
        {"00:24:14", "Cisco"},               {"00:24:36", "Apple"},
        {"00:24:54", "Samsung"},             {"00:24:7B", "Broadcom"},
        {"00:24:7E", "Lenovo"},              {"00:24:81", "HP"},
        {"00:24:BE", "Sony"},                {"00:24:D7", "Intel"},
        {"00:24:E8", "Dell"},                {"00:24:F0", "Cisco"},
        {"00:25:00", "Apple"},               {"00:25:08", "Broadcom"},
        {"00:25:38", "Samsung"},             {"00:25:64", "Dell"},
        {"00:25:68", "NVIDIA"},              {"00:25:84", "Cisco"},
        {"00:25:86", "TP-Link"},             {"00:25:90", "Arista"},
        {"00:25:9E", "Huawei"},              {"00:25:AE", "Microsoft"},
        {"00:25:B3", "HP"},                  {"00:25:BC", "Apple"},
        {"00:26:08", "Apple"},               {"00:26:0B", "Cisco"},
        {"00:26:2D", "Lenovo"},              {"00:26:55", "HP"},
        {"00:26:5A", "D-Link"},              {"00:26:82", "Broadcom"},
        {"00:26:88", "Juniper"},             {"00:26:B9", "Dell"},
        {"00:26:BB", "Apple"},               {"00:26:C6", "Intel"},
        {"00:27:19", "TP-Link"},             {"00:30:65", "Apple"},
        {"00:30:C1", "HP"},                  {"00:50:56", "VMware"},
        {"00:E0:4C", "Realtek"},             {"00:E0:FC", "Huawei"},
    };
    return db;
}

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// MacLookupWidget 实现
// ============================================================================

MacLookupWidget::MacLookupWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

MacLookupWidget::~MacLookupWidget() = default;

// ─── UI Setup ──────────────────────────────────────────────────────────

void MacLookupWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 输入区 ──
    auto* inputGroup = new QGroupBox("MAC 地址查询");
    auto* inputLayout = new QHBoxLayout(inputGroup);
    inputLayout->setSpacing(8);

    m_macInput = new QLineEdit();
    m_macInput->setPlaceholderText("输入 MAC 地址或 OUI 前缀，如 00:11:22:33:44:55 或 00:11:22");
    inputLayout->addWidget(m_macInput, 1);

    m_lookupBtn = new QPushButton("查询");
    m_lookupBtn->setFixedHeight(32);
    inputLayout->addWidget(m_lookupBtn);

    m_batchLookupBtn = new QPushButton("批量查询");
    m_batchLookupBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #58A6FF;"
        "  border: 1px solid #30363D; padding: 8px 20px;"
        "  border-radius: 4px; font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    inputLayout->addWidget(m_batchLookupBtn);

    mainLayout->addWidget(inputGroup);

    // ── 结果区 ──
    auto* resultGroup = new QGroupBox("查询结果");
    auto* resultLayout = new QFormLayout(resultGroup);
    resultLayout->setSpacing(6);

    m_ouiEdit = new QLineEdit();
    m_ouiEdit->setReadOnly(true);
    m_ouiEdit->setPlaceholderText("—");
    resultLayout->addRow("OUI 前缀:", m_ouiEdit);

    m_vendorEdit = new QLineEdit();
    m_vendorEdit->setReadOnly(true);
    m_vendorEdit->setPlaceholderText("—");
    resultLayout->addRow("厂商:", m_vendorEdit);

    m_typeEdit = new QLineEdit();
    m_typeEdit->setReadOnly(true);
    m_typeEdit->setPlaceholderText("—");
    resultLayout->addRow("地址类型:", m_typeEdit);

    mainLayout->addWidget(resultGroup);

    // ── 历史区 ──
    auto* historyGroup = new QGroupBox("查询历史");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(4);
    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );
    toolbar->addWidget(m_exportBtn);
    toolbar->addStretch();
    m_clearBtn = new QPushButton("清空历史");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #F85149;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    toolbar->addWidget(m_clearBtn);
    historyLayout->addLayout(toolbar);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(4);
    m_historyTable->setHorizontalHeaderLabels({"MAC 地址", "厂商", "OUI", "时间"});
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_MAC, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_VENDOR, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_OUI, QHeaderView::ResizeToContents);
    m_historyTable->verticalHeader()->setVisible(false);
    historyLayout->addWidget(m_historyTable, 1);

    mainLayout->addWidget(historyGroup, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void MacLookupWidget::setupConnections()
{
    connect(m_lookupBtn, &QPushButton::clicked, this, &MacLookupWidget::onLookup);
    connect(m_batchLookupBtn, &QPushButton::clicked, this, &MacLookupWidget::onBatchLookup);
    connect(m_macInput, &QLineEdit::returnPressed, this, &MacLookupWidget::onLookup);
    connect(m_exportBtn, &QPushButton::clicked, this, &MacLookupWidget::onExport);
    connect(m_clearBtn, &QPushButton::clicked, this, &MacLookupWidget::onClear);
    connect(m_historyTable, &QTableWidget::cellDoubleClicked,
            this, &MacLookupWidget::onHistoryItemDoubleClicked);
}

// ─── OUI Lookup ────────────────────────────────────────────────────────

QString MacLookupWidget::lookupOUI(const QString& macPrefix)
{
    QString prefix = macPrefix.toUpper();
    return ouiDatabase().value(prefix);
}

// ─── Slot: Lookup ──────────────────────────────────────────────────────

void MacLookupWidget::onLookup()
{
    QString mac = m_macInput->text().trimmed();
    if (mac.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 MAC 地址或 OUI 前缀。");
        return;
    }

    // 清理输入
    QString cleaned = mac.toUpper().remove(QRegularExpression("[^0-9A-F]"));
    if (cleaned.length() < 6) {
        QMessageBox::warning(this, "格式错误",
                             "MAC 地址格式无效。需要至少 6 位十六进制字符 (OUI 前缀)。\n"
                             "支持格式: XX:XX:XX:XX:XX:XX 或 XX:XX:XX");
        return;
    }

    // 提取 OUI (前 6 位)
    QString ouiCleaned = cleaned.left(6);
    QString ouiFormatted;
    for (int i = 0; i < 6; i += 2) {
        if (i > 0) ouiFormatted += ":";
        ouiFormatted += ouiCleaned.mid(i, 2);
    }

    // 查找厂商
    QString vendor = lookupOUI(ouiFormatted);

    // 更新结果
    m_ouiEdit->setText(ouiFormatted);
    m_vendorEdit->setText(vendor.isEmpty() ? "未知厂商 (不在本地数据库中)" : vendor);

    // 检测地址类型
    bool ok = false;
    int firstByte = cleaned.mid(0, 2).toInt(&ok, 16);
    QString type;
    if (ok) {
        if (cleaned == "FFFFFFFFFFFF") {
            type = "广播地址 (Broadcast)";
        } else if (firstByte & 0x01) {
            type = "多播地址 (Multicast)";
        } else {
            type = "单播地址 (Unicast)";
        }
        if (firstByte & 0x02) {
            type += ", 本地管理 (LAA)";
        } else {
            type += ", 全局唯一 (OUI)";
        }
    }
    m_typeEdit->setText(type);

    // 添加到历史
    addToHistory(mac, vendor.isEmpty() ? "未知" : vendor);

    Logger::instance().info("MacLookup",
                            QString("查询 MAC: %1 → 厂商: %2")
                                .arg(mac, vendor.isEmpty() ? "未知" : vendor));
}

// ─── Slot: Batch Lookup ────────────────────────────────────────────────

void MacLookupWidget::onBatchLookup()
{
    bool ok;
    QString input = QInputDialog::getMultiLineText(
        this, "批量 MAC 查询",
        "每行输入一个 MAC 地址或 OUI 前缀:",
        "", &ok);

    if (!ok || input.trimmed().isEmpty()) return;

    QStringList lines = input.split('\n', Qt::SkipEmptyParts);
    int found = 0, unknown = 0;

    for (const QString& line : lines) {
        QString mac = line.trimmed();
        if (mac.isEmpty()) continue;

        QString cleaned = mac.toUpper().remove(QRegularExpression("[^0-9A-F]"));
        if (cleaned.length() < 6) continue;

        QString ouiCleaned = cleaned.left(6);
        QString ouiFormatted;
        for (int i = 0; i < 6; i += 2) {
            if (i > 0) ouiFormatted += ":";
            ouiFormatted += ouiCleaned.mid(i, 2);
        }

        QString vendor = lookupOUI(ouiFormatted);
        if (vendor.isEmpty()) {
            vendor = "未知";
            unknown++;
        } else {
            found++;
        }

        addToHistory(mac, vendor);
    }

    Logger::instance().info("MacLookup",
                            QString("批量查询完成: %1 找到, %2 未知").arg(found).arg(unknown));
    QMessageBox::information(this, "批量查询完成",
                             QString("处理 %1 条记录\n找到: %2\n未知: %3")
                                 .arg(lines.size()).arg(found).arg(unknown));
}

// ─── Add Result ────────────────────────────────────────────────────────

void MacLookupWidget::addResult(const QString& mac, const QString& vendor, const QString& oui)
{
    Q_UNUSED(mac)
    Q_UNUSED(vendor)
    Q_UNUSED(oui)
    // 通过 addToHistory 处理
}

// ─── Add To History ────────────────────────────────────────────────────

void MacLookupWidget::addToHistory(const QString& mac, const QString& vendor)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    auto* macItem = new QTableWidgetItem(mac);
    macItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_MAC, macItem);

    auto* vendorItem = new QTableWidgetItem(vendor);
    vendorItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_VENDOR, vendorItem);

    QString cleaned = mac.toUpper().remove(QRegularExpression("[^0-9A-F]"));
    QString ouiFormatted;
    if (cleaned.length() >= 6) {
        for (int i = 0; i < 6; i += 2) {
            if (i > 0) ouiFormatted += ":";
            ouiFormatted += cleaned.mid(i, 2);
        }
    }
    auto* ouiItem = new QTableWidgetItem(ouiFormatted);
    ouiItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_OUI, ouiItem);

    auto* timeItem = new QTableWidgetItem(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    timeItem->setTextAlignment(Qt::AlignCenter);
    m_historyTable->setItem(row, HIST_COL_TIME, timeItem);

    m_historyTable->scrollToBottom();
}

// ─── Slot: History Item Double Clicked ─────────────────────────────────

void MacLookupWidget::onHistoryItemDoubleClicked(int row, int col)
{
    Q_UNUSED(col)
    auto* item = m_historyTable->item(row, HIST_COL_MAC);
    if (item) {
        m_macInput->setText(item->text());
        onLookup();
    }
}

// ─── Slot: Export ──────────────────────────────────────────────────────

void MacLookupWidget::onExport()
{
    if (m_historyTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "历史记录为空，无需导出。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出查询历史",
                                                     "mac_lookup.csv",
                                                     "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "\xEF\xBB\xBF";
    stream << "MAC 地址,厂商,OUI,时间\n";

    for (int row = 0; row < m_historyTable->rowCount(); ++row) {
        for (int col = 0; col < m_historyTable->columnCount(); ++col) {
            if (col > 0) stream << ",";
            auto* item = m_historyTable->item(row, col);
            if (item) {
                QString text = item->text();
                if (text.contains(",") || text.contains("\"")) {
                    text.replace("\"", "\"\"");
                    text = "\"" + text + "\"";
                }
                stream << text;
            }
        }
        stream << "\n";
    }
    file.close();

    Logger::instance().info("MacLookup", "导出历史: " + filePath);
    QMessageBox::information(this, "导出成功",
                             QString("已导出 %1 条记录到:\n%2")
                                 .arg(m_historyTable->rowCount()).arg(filePath));
}

// ─── Slot: Clear ───────────────────────────────────────────────────────

void MacLookupWidget::onClear()
{
    m_historyTable->setRowCount(0);
    m_ouiEdit->clear();
    m_vendorEdit->clear();
    m_typeEdit->clear();
}
