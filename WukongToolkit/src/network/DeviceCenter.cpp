#include "network/DeviceCenter.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QUuid>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QClipboard>
#include <QStyle>

// ============================================================================
// Forward declarations for static helpers
// ============================================================================

static void setItemVisibleRecursive(QTreeWidgetItem* item, bool visible);
static void applyFilterRecursive(QTreeWidgetItem* item, const QString& filter);
static QTreeWidgetItem* findGroupItemRecursive(QTreeWidgetItem* item, const QString& groupId);
static QTreeWidgetItem* findDeviceItemRecursive(QTreeWidgetItem* item, const QString& deviceId);
static QStringList parseCSVLine(const QString& line);
static QString escapeCSVField(const QString& field);

// ============================================================================
// Constructor / Destructor
// ============================================================================

DeviceCenter::DeviceCenter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
    loadGroups();
    loadDevices();
    populateTree();
}

DeviceCenter::~DeviceCenter() = default;

// ============================================================================
// UI Setup
// ============================================================================

void DeviceCenter::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // --- Toolbar: search + action buttons ---
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(4);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索设备...");
    m_searchEdit->setClearButtonEnabled(true);
    toolbarLayout->addWidget(m_searchEdit, 1);

    m_addBtn = new QPushButton("添加");
    m_editBtn = new QPushButton("编辑");
    m_deleteBtn = new QPushButton("删除");
    m_importBtn = new QPushButton("导入");
    m_exportBtn = new QPushButton("导出");
    m_addGroupBtn = new QPushButton("+分组");
    m_deleteGroupBtn = new QPushButton("-分组");

    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_deleteGroupBtn->setEnabled(false);

    toolbarLayout->addWidget(m_addBtn);
    toolbarLayout->addWidget(m_editBtn);
    toolbarLayout->addWidget(m_deleteBtn);
    toolbarLayout->addWidget(m_importBtn);
    toolbarLayout->addWidget(m_exportBtn);
    toolbarLayout->addWidget(m_addGroupBtn);
    toolbarLayout->addWidget(m_deleteGroupBtn);

    mainLayout->addLayout(toolbarLayout);

    // --- Device tree ---
    m_deviceTree = new QTreeWidget();
    m_deviceTree->setColumnCount(4);
    m_deviceTree->setHeaderLabels({"名称", "主机地址", "厂商", "分组"});
    m_deviceTree->setRootIsDecorated(true);
    m_deviceTree->setAlternatingRowColors(true);
    m_deviceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_deviceTree->setSortingEnabled(false);
    m_deviceTree->header()->setStretchLastSection(true);
    m_deviceTree->header()->setSectionResizeMode(TREE_COL_NAME, QHeaderView::Stretch);
    m_deviceTree->header()->setSectionResizeMode(TREE_COL_HOST, QHeaderView::ResizeToContents);
    m_deviceTree->header()->setSectionResizeMode(TREE_COL_VENDOR, QHeaderView::ResizeToContents);
    m_deviceTree->header()->setSectionResizeMode(TREE_COL_GROUP, QHeaderView::ResizeToContents);
    mainLayout->addWidget(m_deviceTree, 1);
}

void DeviceCenter::setupConnections()
{
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DeviceCenter::onSearchTextChanged);

    connect(m_addBtn, &QPushButton::clicked, this, &DeviceCenter::onAddDevice);
    connect(m_editBtn, &QPushButton::clicked, this, &DeviceCenter::onEditDevice);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DeviceCenter::onDeleteDevice);
    connect(m_importBtn, &QPushButton::clicked, this, &DeviceCenter::onImportCSV);
    connect(m_exportBtn, &QPushButton::clicked, this, &DeviceCenter::onExportCSV);
    connect(m_addGroupBtn, &QPushButton::clicked, this, &DeviceCenter::onAddGroup);
    connect(m_deleteGroupBtn, &QPushButton::clicked, this, &DeviceCenter::onDeleteGroup);

    connect(m_deviceTree, &QTreeWidget::itemDoubleClicked,
            this, &DeviceCenter::onTreeItemDoubleClicked);
    connect(m_deviceTree, &QTreeWidget::itemClicked,
            this, &DeviceCenter::onTreeItemClicked);
    connect(m_deviceTree, &QTreeWidget::customContextMenuRequested,
            this, &DeviceCenter::onTreeContextMenu);
}

// ============================================================================
// Public
// ============================================================================

void DeviceCenter::refresh()
{
    loadGroups();
    loadDevices();
    populateTree();
}

// ============================================================================
// Data Loading
// ============================================================================

void DeviceCenter::loadDevices()
{
    m_devices = queryAllDevices();
    Logger::instance().info("DeviceCenter", QString("Loaded %1 devices").arg(m_devices.size()));
}

void DeviceCenter::loadGroups()
{
    m_groups = queryAllGroups();
    Logger::instance().info("DeviceCenter", QString("Loaded %1 groups").arg(m_groups.size()));
}

void DeviceCenter::populateTree()
{
    m_deviceTree->clear();

    // Build a lookup: groupId -> QTreeWidgetItem*
    QMap<QString, QTreeWidgetItem*> groupItems;
    // Also handle parent-child group hierarchy
    QMap<QString, DeviceGroupInfo> groupMap;
    for (const auto& g : m_groups) {
        groupMap[g.id] = g;
    }

    // Create top-level group items first, then attach children
    for (const auto& g : m_groups) {
        if (g.parentId.isEmpty() || !groupMap.contains(g.parentId)) {
            auto* item = new QTreeWidgetItem();
            item->setText(TREE_COL_NAME, g.name);
            item->setData(TREE_COL_NAME, Qt::UserRole, g.id);
            item->setData(TREE_COL_NAME, Qt::UserRole + 1, "group");
            item->setIcon(TREE_COL_NAME, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            groupItems[g.id] = item;
            m_deviceTree->addTopLevelItem(item);
        }
    }

    // Attach child groups
    for (const auto& g : m_groups) {
        if (!g.parentId.isEmpty() && groupItems.contains(g.parentId)) {
            auto* item = new QTreeWidgetItem();
            item->setText(TREE_COL_NAME, g.name);
            item->setData(TREE_COL_NAME, Qt::UserRole, g.id);
            item->setData(TREE_COL_NAME, Qt::UserRole + 1, "group");
            item->setIcon(TREE_COL_NAME, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            groupItems[g.parentId]->addChild(item);
            groupItems[g.id] = item;
        }
    }

    // --- Ungrouped devices ---
    for (const auto& dev : m_devices) {
        if (dev.groupId.isEmpty() || !groupItems.contains(dev.groupId)) {
            auto* item = new QTreeWidgetItem();
            item->setText(TREE_COL_NAME, dev.name);
            item->setText(TREE_COL_HOST, QString("%1:%2").arg(dev.host).arg(dev.port));
            item->setText(TREE_COL_VENDOR, vendorToString(dev.vendor));
            item->setText(TREE_COL_GROUP, dev.groupName);
            item->setData(TREE_COL_NAME, Qt::UserRole, dev.id);
            item->setData(TREE_COL_NAME, Qt::UserRole + 1, "device");
            if (dev.favorite) {
                item->setIcon(TREE_COL_NAME, QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
            }
            m_deviceTree->addTopLevelItem(item);
        }
    }

    // --- Grouped devices ---
    for (const auto& dev : m_devices) {
        if (!dev.groupId.isEmpty() && groupItems.contains(dev.groupId)) {
            auto* item = new QTreeWidgetItem();
            item->setText(TREE_COL_NAME, dev.name);
            item->setText(TREE_COL_HOST, QString("%1:%2").arg(dev.host).arg(dev.port));
            item->setText(TREE_COL_VENDOR, vendorToString(dev.vendor));
            item->setText(TREE_COL_GROUP, dev.groupName);
            item->setData(TREE_COL_NAME, Qt::UserRole, dev.id);
            item->setData(TREE_COL_NAME, Qt::UserRole + 1, "device");
            if (dev.favorite) {
                item->setIcon(TREE_COL_NAME, QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
            }
            groupItems[dev.groupId]->addChild(item);
        }
    }

    m_deviceTree->expandAll();
    applyFilter();
}

void DeviceCenter::applyFilter()
{
    QString filter = m_searchEdit->text().trimmed().toLower();
    if (filter.isEmpty()) {
        // Show all items
        for (int i = 0; i < m_deviceTree->topLevelItemCount(); ++i) {
            setItemVisibleRecursive(m_deviceTree->topLevelItem(i), true);
        }
        return;
    }

    // Hide items that don't match
    for (int i = 0; i < m_deviceTree->topLevelItemCount(); ++i) {
        auto* item = m_deviceTree->topLevelItem(i);
        applyFilterRecursive(item, filter);
    }
}

static void setItemVisibleRecursive(QTreeWidgetItem* item, bool visible)
{
    item->setHidden(!visible);
    for (int i = 0; i < item->childCount(); ++i) {
        setItemVisibleRecursive(item->child(i), visible);
    }
}

static void applyFilterRecursive(QTreeWidgetItem* item, const QString& filter)
{
    QString type = item->data(0, Qt::UserRole + 1).toString();
    bool match = false;

    for (int col = 0; col < item->columnCount(); ++col) {
        if (item->text(col).toLower().contains(filter)) {
            match = true;
            break;
        }
    }

    // Also check children for groups
    bool childMatch = false;
    for (int i = 0; i < item->childCount(); ++i) {
        applyFilterRecursive(item->child(i), filter);
        if (!item->child(i)->isHidden()) {
            childMatch = true;
        }
    }

    if (type == "group") {
        item->setHidden(!(match || childMatch));
        if (match || childMatch) {
            item->setExpanded(true);
        }
    } else {
        item->setHidden(!match);
    }
}

// ============================================================================
// Slot: Search
// ============================================================================

void DeviceCenter::onSearchTextChanged(const QString& /*text*/)
{
    applyFilter();
}

// ============================================================================
// Slot: Add Device
// ============================================================================

void DeviceCenter::onAddDevice()
{
    DeviceInfo info;
    info.id = generateId();
    info.createTime = currentTimestamp();
    info.updateTime = info.createTime;
    showDeviceDialog(info, false);
}

// ============================================================================
// Slot: Edit Device
// ============================================================================

void DeviceCenter::onEditDevice()
{
    QTreeWidgetItem* item = m_deviceTree->currentItem();
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();
    if (type != "device") return;

    QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
    DeviceInfo info = queryDeviceById(deviceId);
    if (info.id.isEmpty()) {
        QMessageBox::warning(this, "编辑设备", "设备不存在");
        return;
    }
    showDeviceDialog(info, true);
}

// ============================================================================
// Slot: Delete Device
// ============================================================================

void DeviceCenter::onDeleteDevice()
{
    QTreeWidgetItem* item = m_deviceTree->currentItem();
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();
    if (type != "device") return;

    QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
    QString deviceName = item->text(TREE_COL_NAME);

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "删除设备",
        QString("确定要删除设备 \"%1\" 吗？").arg(deviceName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (deleteDevice(deviceId)) {
        Logger::instance().info("DeviceCenter", "Deleted device: " + deviceName);
        refresh();
    }
}

// ============================================================================
// Slot: Import CSV
// ============================================================================

void DeviceCenter::onImportCSV()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "导入 CSV", QString(), "CSV 文件 (*.csv);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导入失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    int imported = 0;
    int skipped = 0;
    bool isFirstLine = true;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList fields = parseCSVLine(line);

        if (isFirstLine) {
            isFirstLine = false;
            // Check if it's a header line (starts with name/host)
            if (!fields.isEmpty() &&
                (fields[0].toLower() == "name" || fields[0].toLower() == "名称")) {
                continue;
            }
        }

        if (fields.size() < 3) {
            skipped++;
            continue;
        }

        DeviceInfo info;
        info.id = generateId();
        info.name = fields.value(0).trimmed();
        info.host = fields.value(1).trimmed();
        info.port = fields.value(2).toInt();
        if (info.port == 0) info.port = 22;
        info.username = fields.value(3).trimmed();
        info.password = fields.value(4).trimmed();
        info.protocol = fields.value(5).trimmed();
        if (info.protocol.isEmpty()) info.protocol = "SSH";
        info.vendor = stringToVendor(fields.value(6).trimmed());
        info.model = fields.value(7).trimmed();
        info.version = fields.value(8).trimmed();
        info.remark = fields.value(9).trimmed();
        info.favorite = (fields.value(10).trimmed().toLower() == "yes" ||
                         fields.value(10).trimmed() == "1");
        QString groupName = fields.value(11).trimmed();
        if (!groupName.isEmpty()) {
            info.groupId = groupIdByName(groupName);
            if (info.groupId.isEmpty()) {
                // Auto-create group
                DeviceGroupInfo g;
                g.id = generateId();
                g.name = groupName;
                insertGroup(g);
                info.groupId = g.id;
                loadGroups();
            }
            info.groupName = groupName;
        }
        info.createTime = currentTimestamp();
        info.updateTime = info.createTime;

        if (insertDevice(info)) {
            imported++;
        } else {
            skipped++;
        }
    }

    file.close();

    Logger::instance().info("DeviceCenter",
        QString("CSV import: %1 imported, %2 skipped").arg(imported).arg(skipped));
    QMessageBox::information(this, "导入完成",
        QString("成功导入 %1 个设备，跳过 %2 条记录").arg(imported).arg(skipped));

    refresh();
}

// ============================================================================
// Slot: Export CSV
// ============================================================================

void DeviceCenter::onExportCSV()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 CSV", "devices.csv", "CSV 文件 (*.csv);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    // Header
    stream << "name,host,port,username,password,protocol,vendor,model,version,remark,favorite,group\n";

    QList<DeviceInfo> devices = queryAllDevices();
    for (const auto& dev : devices) {
        QStringList row;
        row << escapeCSVField(dev.name)
            << escapeCSVField(dev.host)
            << QString::number(dev.port)
            << escapeCSVField(dev.username)
            << escapeCSVField(dev.password)
            << escapeCSVField(dev.protocol)
            << escapeCSVField(vendorToString(dev.vendor))
            << escapeCSVField(dev.model)
            << escapeCSVField(dev.version)
            << escapeCSVField(dev.remark)
            << (dev.favorite ? "yes" : "no")
            << escapeCSVField(dev.groupName);
        stream << row.join(",") << "\n";
    }

    file.close();

    Logger::instance().info("DeviceCenter",
        QString("Exported %1 devices to %2").arg(devices.size()).arg(filePath));
    QMessageBox::information(this, "导出完成",
        QString("已导出 %1 个设备").arg(devices.size()));
}

// ============================================================================
// Slot: Tree Double Click
// ============================================================================

void DeviceCenter::onTreeItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();
    if (type != "device") return;

    QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
    DeviceInfo info = queryDeviceById(deviceId);
    if (info.id.isEmpty()) return;

    emit deviceDoubleClicked(info.host, info.port, info.username, info.password);
}

// ============================================================================
// Slot: Tree Click (selection)
// ============================================================================

void DeviceCenter::onTreeItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();

    m_editBtn->setEnabled(type == "device");
    m_deleteBtn->setEnabled(type == "device");
    m_deleteGroupBtn->setEnabled(type == "group");

    if (type == "device") {
        QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
        emit deviceSelected(deviceId);
    }
}

// ============================================================================
// Slot: Context Menu
// ============================================================================

void DeviceCenter::onTreeContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_deviceTree->itemAt(pos);
    QMenu menu(this);

    if (!item) {
        // Right-click on empty space
        auto* addAction = menu.addAction("添加设备");
        connect(addAction, &QAction::triggered, this, &DeviceCenter::onAddDevice);
        menu.addSeparator();
        auto* addGroupAction = menu.addAction("添加分组");
        connect(addGroupAction, &QAction::triggered, this, &DeviceCenter::onAddGroup);
        menu.exec(m_deviceTree->viewport()->mapToGlobal(pos));
        return;
    }

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();

    if (type == "device") {
        QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();

        auto* connectAction = menu.addAction("连接");
        auto* editAction = menu.addAction("编辑");
        auto* favoriteAction = menu.addAction("收藏/取消收藏");
        auto* copyHostAction = menu.addAction("复制主机地址");
        menu.addSeparator();
        auto* deleteAction = menu.addAction("删除");

        QAction* chosen = menu.exec(m_deviceTree->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == connectAction) {
            DeviceInfo info = queryDeviceById(deviceId);
            if (!info.id.isEmpty()) {
                emit deviceDoubleClicked(info.host, info.port, info.username, info.password);
            }
        } else if (chosen == editAction) {
            onEditDevice();
        } else if (chosen == favoriteAction) {
            onToggleFavorite();
        } else if (chosen == copyHostAction) {
            DeviceInfo info = queryDeviceById(deviceId);
            if (!info.id.isEmpty()) {
                QApplication::clipboard()->setText(
                    QString("%1:%2").arg(info.host).arg(info.port));
            }
        } else if (chosen == deleteAction) {
            onDeleteDevice();
        }
    } else if (type == "group") {
        QString groupId = item->data(TREE_COL_NAME, Qt::UserRole).toString();

        auto* addDeviceAction = menu.addAction("添加设备到此分组");
        auto* renameAction = menu.addAction("重命名分组");
        menu.addSeparator();
        auto* deleteGroupAction = menu.addAction("删除分组");

        QAction* chosen = menu.exec(m_deviceTree->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == addDeviceAction) {
            DeviceInfo info;
            info.id = generateId();
            info.groupId = groupId;
            info.groupName = item->text(TREE_COL_NAME);
            info.createTime = currentTimestamp();
            info.updateTime = info.createTime;
            showDeviceDialog(info, false);
        } else if (chosen == renameAction) {
            bool ok;
            QString newName = QInputDialog::getText(
                this, "重命名分组", "分组名称:", QLineEdit::Normal,
                item->text(TREE_COL_NAME), &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                QSqlQuery query(DatabaseManager::instance().database());
                query.prepare("UPDATE device_group SET name = ? WHERE id = ?");
                query.addBindValue(newName.trimmed());
                query.addBindValue(groupId);
                if (query.exec()) {
                    refresh();
                }
            }
        } else if (chosen == deleteGroupAction) {
            onDeleteGroup();
        }
    }
}

// ============================================================================
// Slot: Add Group
// ============================================================================

void DeviceCenter::onAddGroup()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, "添加分组", "分组名称:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    DeviceGroupInfo info;
    info.id = generateId();
    info.name = name.trimmed();
    info.sort = m_groups.size();

    if (insertGroup(info)) {
        Logger::instance().info("DeviceCenter", "Added group: " + info.name);
        refresh();
    }
}

// ============================================================================
// Slot: Delete Group
// ============================================================================

void DeviceCenter::onDeleteGroup()
{
    QTreeWidgetItem* item = m_deviceTree->currentItem();
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();
    if (type != "group") return;

    QString groupId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
    QString groupName = item->text(TREE_COL_NAME);

    // Check if group has devices
    bool hasDevices = false;
    for (const auto& dev : m_devices) {
        if (dev.groupId == groupId) {
            hasDevices = true;
            break;
        }
    }

    QMessageBox::StandardButton ret;
    if (hasDevices) {
        ret = QMessageBox::question(
            this, "删除分组",
            QString("分组 \"%1\" 下还有设备，确定要删除吗？\n设备将移至未分组。").arg(groupName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    } else {
        ret = QMessageBox::question(
            this, "删除分组",
            QString("确定要删除分组 \"%1\" 吗？").arg(groupName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    }

    if (ret != QMessageBox::Yes) return;

    // Move devices in this group to ungrouped
    if (hasDevices) {
        QSqlQuery query(DatabaseManager::instance().database());
        query.prepare("UPDATE device SET group_id = NULL WHERE group_id = ?");
        query.addBindValue(groupId);
        query.exec();
    }

    if (deleteGroup(groupId)) {
        Logger::instance().info("DeviceCenter", "Deleted group: " + groupName);
        refresh();
    }
}

// ============================================================================
// Slot: Toggle Favorite
// ============================================================================

void DeviceCenter::onToggleFavorite()
{
    QTreeWidgetItem* item = m_deviceTree->currentItem();
    if (!item) return;

    QString type = item->data(TREE_COL_NAME, Qt::UserRole + 1).toString();
    if (type != "device") return;

    QString deviceId = item->data(TREE_COL_NAME, Qt::UserRole).toString();
    DeviceInfo info = queryDeviceById(deviceId);
    if (info.id.isEmpty()) return;

    info.favorite = !info.favorite;
    info.updateTime = currentTimestamp();

    if (updateDevice(info)) {
        refresh();
    }
}

// ============================================================================
// Device CRUD
// ============================================================================

bool DeviceCenter::insertDevice(const DeviceInfo& info)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "INSERT INTO device (id, name, host, port, username, password, "
        "protocol, vendor, model, version, group_id, remark, favorite, "
        "create_time, update_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(info.id);
    query.addBindValue(info.name);
    query.addBindValue(info.host);
    query.addBindValue(info.port);
    query.addBindValue(info.username);
    query.addBindValue(info.password);
    query.addBindValue(info.protocol);
    query.addBindValue(info.vendor);
    query.addBindValue(info.model);
    query.addBindValue(info.version);
    query.addBindValue(info.groupId.isEmpty() ? QVariant() : info.groupId);
    query.addBindValue(info.remark);
    query.addBindValue(info.favorite ? 1 : 0);
    query.addBindValue(info.createTime);
    query.addBindValue(info.updateTime);

    if (!query.exec()) {
        Logger::instance().error("DeviceCenter",
            "Insert device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

bool DeviceCenter::updateDevice(const DeviceInfo& info)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "UPDATE device SET name = ?, host = ?, port = ?, username = ?, password = ?, "
        "protocol = ?, vendor = ?, model = ?, version = ?, group_id = ?, "
        "remark = ?, favorite = ?, update_time = ? WHERE id = ?");
    query.addBindValue(info.name);
    query.addBindValue(info.host);
    query.addBindValue(info.port);
    query.addBindValue(info.username);
    query.addBindValue(info.password);
    query.addBindValue(info.protocol);
    query.addBindValue(info.vendor);
    query.addBindValue(info.model);
    query.addBindValue(info.version);
    query.addBindValue(info.groupId.isEmpty() ? QVariant() : info.groupId);
    query.addBindValue(info.remark);
    query.addBindValue(info.favorite ? 1 : 0);
    query.addBindValue(info.updateTime);
    query.addBindValue(info.id);

    if (!query.exec()) {
        Logger::instance().error("DeviceCenter",
            "Update device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

bool DeviceCenter::deleteDevice(const QString& deviceId)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM device WHERE id = ?");
    query.addBindValue(deviceId);

    if (!query.exec()) {
        Logger::instance().error("DeviceCenter",
            "Delete device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

QList<DeviceInfo> DeviceCenter::queryAllDevices()
{
    QList<DeviceInfo> result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, name, host, port, username, password, protocol, "
               "vendor, model, version, group_id, remark, favorite, "
               "create_time, update_time FROM device ORDER BY name");

    while (query.next()) {
        DeviceInfo info;
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.host = query.value(2).toString();
        info.port = query.value(3).toInt();
        info.username = query.value(4).toString();
        info.password = query.value(5).toString();
        info.protocol = query.value(6).toString();
        info.vendor = query.value(7).toInt();
        info.model = query.value(8).toString();
        info.version = query.value(9).toString();
        info.groupId = query.value(10).toString();
        info.remark = query.value(11).toString();
        info.favorite = (query.value(12).toInt() != 0);
        info.createTime = query.value(13).toString();
        info.updateTime = query.value(14).toString();
        info.groupName = groupNameById(info.groupId);
        result.append(info);
    }
    return result;
}

DeviceInfo DeviceCenter::queryDeviceById(const QString& deviceId)
{
    DeviceInfo info;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, host, port, username, password, protocol, "
                  "vendor, model, version, group_id, remark, favorite, "
                  "create_time, update_time FROM device WHERE id = ?");
    query.addBindValue(deviceId);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.host = query.value(2).toString();
        info.port = query.value(3).toInt();
        info.username = query.value(4).toString();
        info.password = query.value(5).toString();
        info.protocol = query.value(6).toString();
        info.vendor = query.value(7).toInt();
        info.model = query.value(8).toString();
        info.version = query.value(9).toString();
        info.groupId = query.value(10).toString();
        info.remark = query.value(11).toString();
        info.favorite = (query.value(12).toInt() != 0);
        info.createTime = query.value(13).toString();
        info.updateTime = query.value(14).toString();
        info.groupName = groupNameById(info.groupId);
    }
    return info;
}

// ============================================================================
// Group CRUD
// ============================================================================

bool DeviceCenter::insertGroup(const DeviceGroupInfo& info)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "INSERT INTO device_group (id, parent_id, name, sort) VALUES (?, ?, ?, ?)");
    query.addBindValue(info.id);
    query.addBindValue(info.parentId.isEmpty() ? QVariant() : info.parentId);
    query.addBindValue(info.name);
    query.addBindValue(info.sort);

    if (!query.exec()) {
        Logger::instance().error("DeviceCenter",
            "Insert group failed: " + query.lastError().text());
        return false;
    }
    return true;
}

bool DeviceCenter::deleteGroup(const QString& groupId)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM device_group WHERE id = ?");
    query.addBindValue(groupId);

    if (!query.exec()) {
        Logger::instance().error("DeviceCenter",
            "Delete group failed: " + query.lastError().text());
        return false;
    }
    return true;
}

QList<DeviceGroupInfo> DeviceCenter::queryAllGroups()
{
    QList<DeviceGroupInfo> result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, parent_id, name, sort FROM device_group ORDER BY sort, name");

    while (query.next()) {
        DeviceGroupInfo info;
        info.id = query.value(0).toString();
        info.parentId = query.value(1).toString();
        info.name = query.value(2).toString();
        info.sort = query.value(3).toInt();
        result.append(info);
    }
    return result;
}

QString DeviceCenter::groupNameById(const QString& groupId)
{
    if (groupId.isEmpty()) return QString();
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT name FROM device_group WHERE id = ?");
    query.addBindValue(groupId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

QString DeviceCenter::groupIdByName(const QString& name)
{
    if (name.isEmpty()) return QString();
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id FROM device_group WHERE name = ?");
    query.addBindValue(name);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

// ============================================================================
// Helper Methods
// ============================================================================

QString DeviceCenter::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString DeviceCenter::currentTimestamp()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

void DeviceCenter::showDeviceDialog(DeviceInfo& info, bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? "编辑设备" : "添加设备");
    dialog.setMinimumWidth(480);
    dialog.setModal(true);

    auto* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);

    // --- Basic info group ---
    auto* basicGroup = new QGroupBox("基本信息");
    auto* basicForm = new QFormLayout(basicGroup);
    basicForm->setSpacing(6);

    auto* nameEdit = new QLineEdit(info.name);
    nameEdit->setPlaceholderText("设备名称");
    basicForm->addRow("名称:", nameEdit);

    auto* hostEdit = new QLineEdit(info.host);
    hostEdit->setPlaceholderText("192.168.1.1");
    basicForm->addRow("主机地址:", hostEdit);

    auto* portSpin = new QSpinBox();
    portSpin->setRange(1, 65535);
    portSpin->setValue(info.port);
    basicForm->addRow("端口:", portSpin);

    auto* protocolCombo = new QComboBox();
    protocolCombo->addItems({"SSH", "Telnet", "Serial", "HTTP", "HTTPS"});
    protocolCombo->setCurrentText(info.protocol);
    basicForm->addRow("协议:", protocolCombo);

    mainLayout->addWidget(basicGroup);

    // --- Auth group ---
    auto* authGroup = new QGroupBox("认证信息");
    auto* authForm = new QFormLayout(authGroup);
    authForm->setSpacing(6);

    auto* userEdit = new QLineEdit(info.username);
    userEdit->setPlaceholderText("admin");
    authForm->addRow("用户名:", userEdit);

    auto* passwordEdit = new QLineEdit(info.password);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("输入密码");
    authForm->addRow("密码:", passwordEdit);

    mainLayout->addWidget(authGroup);

    // --- Detail group ---
    auto* detailGroup = new QGroupBox("设备详情");
    auto* detailForm = new QFormLayout(detailGroup);
    detailForm->setSpacing(6);

    auto* vendorCombo = new QComboBox();
    vendorCombo->addItems({"未知", "Cisco", "Huawei", "H3C", "Juniper", "Arista", "Ruijie", "其他"});
    vendorCombo->setCurrentIndex(info.vendor);
    detailForm->addRow("厂商:", vendorCombo);

    auto* modelEdit = new QLineEdit(info.model);
    modelEdit->setPlaceholderText("设备型号");
    detailForm->addRow("型号:", modelEdit);

    auto* versionEdit = new QLineEdit(info.version);
    versionEdit->setPlaceholderText("软件版本");
    detailForm->addRow("版本:", versionEdit);

    auto* groupCombo = new QComboBox();
    groupCombo->addItem("(无分组)", QString());
    for (const auto& g : m_groups) {
        groupCombo->addItem(g.name, g.id);
    }
    if (!info.groupId.isEmpty()) {
        int idx = groupCombo->findData(info.groupId);
        if (idx >= 0) groupCombo->setCurrentIndex(idx);
    }
    detailForm->addRow("分组:", groupCombo);

    mainLayout->addWidget(detailGroup);

    // --- Remark group ---
    auto* remarkGroup = new QGroupBox("备注");
    auto* remarkLayout = new QVBoxLayout(remarkGroup);

    auto* remarkEdit = new QTextEdit();
    remarkEdit->setPlaceholderText("备注信息...");
    remarkEdit->setPlainText(info.remark);
    remarkEdit->setMaximumHeight(80);
    remarkLayout->addWidget(remarkEdit);

    mainLayout->addWidget(remarkGroup);

    // --- Favorite ---
    auto* favoriteCheck = new QCheckBox("收藏此设备");
    favoriteCheck->setChecked(info.favorite);
    mainLayout->addWidget(favoriteCheck);

    // --- Buttons ---
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(isEdit ? "保存" : "添加");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) return;

    // Collect data
    info.name = nameEdit->text().trimmed();
    info.host = hostEdit->text().trimmed();
    info.port = portSpin->value();
    info.protocol = protocolCombo->currentText();
    info.username = userEdit->text().trimmed();
    info.password = passwordEdit->text();
    info.vendor = vendorCombo->currentIndex();
    info.model = modelEdit->text().trimmed();
    info.version = versionEdit->text().trimmed();
    info.groupId = groupCombo->currentData().toString();
    info.groupName = groupCombo->currentText();
    if (info.groupName == "(无分组)") info.groupName.clear();
    info.remark = remarkEdit->toPlainText().trimmed();
    info.favorite = favoriteCheck->isChecked();
    info.updateTime = currentTimestamp();

    if (info.name.isEmpty() || info.host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "名称和主机地址不能为空");
        return;
    }

    if (isEdit) {
        if (updateDevice(info)) {
            Logger::instance().info("DeviceCenter", "Updated device: " + info.name);
            refresh();
        }
    } else {
        if (insertDevice(info)) {
            Logger::instance().info("DeviceCenter", "Added device: " + info.name);
            refresh();
        }
    }
}

QString DeviceCenter::vendorToString(int vendor) const
{
    switch (vendor) {
        case 0: return "未知";
        case 1: return "Cisco";
        case 2: return "Huawei";
        case 3: return "H3C";
        case 4: return "Juniper";
        case 5: return "Arista";
        case 6: return "Ruijie";
        case 7: return "其他";
        default: return "未知";
    }
}

int DeviceCenter::stringToVendor(const QString& str) const
{
    QString s = str.trimmed().toLower();
    if (s == "cisco" || s == "思科") return 1;
    if (s == "huawei" || s == "华为") return 2;
    if (s == "h3c" || s == "华三") return 3;
    if (s == "juniper" || s == "瞻博") return 4;
    if (s == "arista") return 5;
    if (s == "ruijie" || s == "锐捷") return 6;
    if (s == "other" || s == "其他" || s == "其它") return 7;
    return 0;
}

QTreeWidgetItem* DeviceCenter::findGroupItem(const QString& groupId)
{
    for (int i = 0; i < m_deviceTree->topLevelItemCount(); ++i) {
        auto* item = findGroupItemRecursive(m_deviceTree->topLevelItem(i), groupId);
        if (item) return item;
    }
    return nullptr;
}

static QTreeWidgetItem* findGroupItemRecursive(QTreeWidgetItem* item, const QString& groupId)
{
    if (item->data(0, Qt::UserRole + 1).toString() == "group" &&
        item->data(0, Qt::UserRole).toString() == groupId) {
        return item;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        auto* found = findGroupItemRecursive(item->child(i), groupId);
        if (found) return found;
    }
    return nullptr;
}

QTreeWidgetItem* DeviceCenter::findDeviceItem(const QString& deviceId)
{
    for (int i = 0; i < m_deviceTree->topLevelItemCount(); ++i) {
        auto* item = findDeviceItemRecursive(m_deviceTree->topLevelItem(i), deviceId);
        if (item) return item;
    }
    return nullptr;
}

static QTreeWidgetItem* findDeviceItemRecursive(QTreeWidgetItem* item, const QString& deviceId)
{
    if (item->data(0, Qt::UserRole + 1).toString() == "device" &&
        item->data(0, Qt::UserRole).toString() == deviceId) {
        return item;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        auto* found = findDeviceItemRecursive(item->child(i), deviceId);
        if (found) return found;
    }
    return nullptr;
}

// ============================================================================
// CSV Helpers
// ============================================================================

static QStringList parseCSVLine(const QString& line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.append(field.trimmed());
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.append(field.trimmed());
    return fields;
}

static QString escapeCSVField(const QString& field)
{
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace('"', "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}