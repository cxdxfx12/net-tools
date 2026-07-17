#include "WOLWidget.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUuid>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QApplication>
#include <QStyle>
#include <QSplitter>

// ============================================================================
// Constructor / Destructor
// ============================================================================

WOLWidget::WOLWidget(QWidget* parent)
    : QWidget(parent)
    , m_socket(new QUdpSocket(this))
{
    setupUI();
    setupConnections();
    createTables();
    loadDevices();
    loadHistory();
    refreshDeviceTable();
    refreshHistoryTable();
}

WOLWidget::~WOLWidget() = default;

// ============================================================================
// UI Setup
// ============================================================================

void WOLWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Send Section ──
    auto* sendGroup = new QGroupBox(QStringLiteral("发送 Wake-on-LAN"));
    auto* sendLayout = new QVBoxLayout(sendGroup);
    sendLayout->setSpacing(6);

    auto* sendRow = new QHBoxLayout();
    sendRow->setSpacing(8);

    m_macEdit = new QLineEdit();
    m_macEdit->setPlaceholderText(QStringLiteral("XX:XX:XX:XX:XX:XX"));
    m_macEdit->setMinimumWidth(180);
    sendRow->addWidget(m_macEdit, 1);

    m_broadcastEdit = new QLineEdit();
    m_broadcastEdit->setText(QStringLiteral("255.255.255.255"));
    m_broadcastEdit->setMinimumWidth(140);
    sendRow->addWidget(m_broadcastEdit);

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(9);
    m_portSpin->setMinimumWidth(70);
    m_portSpin->setToolTip(QStringLiteral("目标端口"));
    sendRow->addWidget(m_portSpin);

    m_sendBtn = new QPushButton(QStringLiteral("发送唤醒包"));
    m_sendBtn->setMinimumWidth(110);
    sendRow->addWidget(m_sendBtn);

    sendLayout->addLayout(sendRow);

    auto* optionsRow = new QHBoxLayout();
    optionsRow->setSpacing(8);

    m_multiSubnetCheck = new QCheckBox(QStringLiteral("发送到多个子网"));
    m_multiSubnetCheck->setToolTip(QStringLiteral("同时向多个常见子网广播地址发送唤醒包"));
    optionsRow->addWidget(m_multiSubnetCheck);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet(QStringLiteral("color: #888;"));
    optionsRow->addWidget(m_statusLabel, 1);

    sendLayout->addLayout(optionsRow);

    mainLayout->addWidget(sendGroup);

    // ── Splitter: Device list + History ──
    auto* splitter = new QSplitter(Qt::Vertical);

    // ── Device List Section ──
    auto* deviceGroup = new QGroupBox(QStringLiteral("设备列表"));
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    deviceLayout->setSpacing(4);

    auto* deviceToolbar = new QHBoxLayout();
    deviceToolbar->setSpacing(4);

    m_addDeviceBtn = new QPushButton(QStringLiteral("添加"));
    m_editDeviceBtn = new QPushButton(QStringLiteral("编辑"));
    m_deleteDeviceBtn = new QPushButton(QStringLiteral("删除"));
    m_editDeviceBtn->setEnabled(false);
    m_deleteDeviceBtn->setEnabled(false);

    deviceToolbar->addWidget(m_addDeviceBtn);
    deviceToolbar->addWidget(m_editDeviceBtn);
    deviceToolbar->addWidget(m_deleteDeviceBtn);
    deviceToolbar->addStretch();

    deviceLayout->addLayout(deviceToolbar);

    m_deviceTable = new QTableWidget();
    m_deviceTable->setColumnCount(5);
    m_deviceTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("MAC 地址"),
        QStringLiteral("IP 地址"),
        QStringLiteral("上次唤醒"),
        QStringLiteral("状态")
    });
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(DEV_COL_NAME, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(DEV_COL_MAC, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(DEV_COL_IP, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(DEV_COL_LAST_WOKEN, QHeaderView::ResizeToContents);
    m_deviceTable->verticalHeader()->setVisible(false);
    deviceLayout->addWidget(m_deviceTable);

    splitter->addWidget(deviceGroup);

    // ── History Section ──
    auto* historyGroup = new QGroupBox(QStringLiteral("发送历史"));
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    auto* historyToolbar = new QHBoxLayout();
    historyToolbar->setSpacing(4);

    m_clearHistoryBtn = new QPushButton(QStringLiteral("清空"));
    m_exportHistoryBtn = new QPushButton(QStringLiteral("导出"));

    historyToolbar->addWidget(m_clearHistoryBtn);
    historyToolbar->addWidget(m_exportHistoryBtn);
    historyToolbar->addStretch();

    historyLayout->addLayout(historyToolbar);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(7);
    m_historyTable->setHorizontalHeaderLabels({
        QStringLiteral("设备名称"),
        QStringLiteral("MAC 地址"),
        QStringLiteral("目标 IP"),
        QStringLiteral("广播地址"),
        QStringLiteral("端口"),
        QStringLiteral("发送时间"),
        QStringLiteral("状态")
    });
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_NAME, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_MAC, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_TARGET_IP, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_BROADCAST, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_PORT, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(HIST_COL_TIME, QHeaderView::ResizeToContents);
    m_historyTable->verticalHeader()->setVisible(false);
    historyLayout->addWidget(m_historyTable);

    splitter->addWidget(historyGroup);

    mainLayout->addWidget(splitter, 1);
}

void WOLWidget::setupConnections()
{
    connect(m_sendBtn, &QPushButton::clicked, this, &WOLWidget::onSendWOL);
    connect(m_addDeviceBtn, &QPushButton::clicked, this, &WOLWidget::onAddDevice);
    connect(m_editDeviceBtn, &QPushButton::clicked, this, &WOLWidget::onEditDevice);
    connect(m_deleteDeviceBtn, &QPushButton::clicked, this, &WOLWidget::onDeleteDevice);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &WOLWidget::onClearHistory);
    connect(m_exportHistoryBtn, &QPushButton::clicked, this, &WOLWidget::onExportHistory);

    connect(m_deviceTable, &QTableWidget::cellDoubleClicked,
            this, &WOLWidget::onDeviceTableDoubleClicked);
    connect(m_deviceTable, &QTableWidget::itemSelectionChanged,
            this, &WOLWidget::onDeviceTableSelectionChanged);
    connect(m_historyTable, &QTableWidget::cellDoubleClicked,
            this, &WOLWidget::onHistoryTableDoubleClicked);
}

// ============================================================================
// Database Tables
// ============================================================================

void WOLWidget::createTables()
{
    QSqlQuery query(DatabaseManager::instance().database());

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS wol_devices ("
            "id TEXT PRIMARY KEY,"
            "name TEXT,"
            "mac TEXT,"
            "ip TEXT,"
            "last_woken TEXT,"
            "status INTEGER DEFAULT 0,"
            "create_time TEXT,"
            "update_time TEXT"
            ")")) {
        Logger::instance().error("WOL",
            "Create wol_devices table failed: " + query.lastError().text());
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS wol_history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT,"
            "mac TEXT,"
            "target_ip TEXT,"
            "broadcast_addr TEXT,"
            "port INTEGER DEFAULT 9,"
            "send_time TEXT,"
            "status INTEGER DEFAULT 0,"
            "error_msg TEXT"
            ")")) {
        Logger::instance().error("WOL",
            "Create wol_history table failed: " + query.lastError().text());
    }
}

// ============================================================================
// Data Loading
// ============================================================================

void WOLWidget::loadDevices()
{
    m_devices = queryAllDevices();
    Logger::instance().info("WOL", QString("Loaded %1 WOL devices").arg(m_devices.size()));
}

void WOLWidget::loadHistory()
{
    m_history = queryAllHistory();
}

void WOLWidget::refreshDeviceTable()
{
    m_deviceTable->setRowCount(0);
    for (const auto& dev : m_devices) {
        int row = m_deviceTable->rowCount();
        m_deviceTable->insertRow(row);

        m_deviceTable->setItem(row, DEV_COL_NAME,
            new QTableWidgetItem(dev.name));
        m_deviceTable->setItem(row, DEV_COL_MAC,
            new QTableWidgetItem(dev.mac));
        m_deviceTable->setItem(row, DEV_COL_IP,
            new QTableWidgetItem(dev.ip));

        QString lastWoken = dev.lastWoken.isEmpty()
            ? QStringLiteral("从未")
            : dev.lastWoken;
        m_deviceTable->setItem(row, DEV_COL_LAST_WOKEN,
            new QTableWidgetItem(lastWoken));

        QString statusText;
        if (dev.status == 1) {
            statusText = QStringLiteral("成功");
        } else if (dev.status == 2) {
            statusText = QStringLiteral("失败");
        } else {
            statusText = QStringLiteral("未知");
        }
        m_deviceTable->setItem(row, DEV_COL_STATUS,
            new QTableWidgetItem(statusText));

        // Store device id in first column
        m_deviceTable->item(row, DEV_COL_NAME)->setData(Qt::UserRole, dev.id);
    }
}

void WOLWidget::refreshHistoryTable()
{
    m_historyTable->setRowCount(0);
    for (const auto& hist : m_history) {
        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);

        m_historyTable->setItem(row, HIST_COL_NAME,
            new QTableWidgetItem(hist.name));
        m_historyTable->setItem(row, HIST_COL_MAC,
            new QTableWidgetItem(hist.mac));
        m_historyTable->setItem(row, HIST_COL_TARGET_IP,
            new QTableWidgetItem(hist.targetIp));
        m_historyTable->setItem(row, HIST_COL_BROADCAST,
            new QTableWidgetItem(hist.broadcastAddr));
        m_historyTable->setItem(row, HIST_COL_PORT,
            new QTableWidgetItem(QString::number(hist.port)));
        m_historyTable->setItem(row, HIST_COL_TIME,
            new QTableWidgetItem(hist.sendTime));

        QString statusText;
        if (hist.status == 0) {
            statusText = QStringLiteral("发送中");
        } else if (hist.status == 1) {
            statusText = QStringLiteral("成功");
        } else {
            statusText = hist.errorMsg.isEmpty()
                ? QStringLiteral("失败")
                : QStringLiteral("失败: ") + hist.errorMsg;
        }
        m_historyTable->setItem(row, HIST_COL_STATUS,
            new QTableWidgetItem(statusText));

        m_historyTable->item(row, HIST_COL_NAME)->setData(Qt::UserRole, hist.id);
    }
}

// ============================================================================
// Slot: Send WOL
// ============================================================================

void WOLWidget::onSendWOL()
{
    QString mac = m_macEdit->text().trimmed();
    if (mac.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("请输入 MAC 地址"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #E06C75;"));
        return;
    }

    QString normalized = normalizeMac(mac);
    if (normalized.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("MAC 地址格式无效"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #E06C75;"));
        return;
    }

    QString broadcastAddr = m_broadcastEdit->text().trimmed();
    if (broadcastAddr.isEmpty()) {
        broadcastAddr = QStringLiteral("255.255.255.255");
    }

    int port = m_portSpin->value();

    // Check if device exists in list
    WOLDeviceInfo dev = queryDeviceByMac(normalized);
    QString deviceName = dev.id.isEmpty() ? QString() : dev.name;

    if (m_multiSubnetCheck->isChecked()) {
        QStringList subnets = getSubnetBroadcasts(broadcastAddr);
        if (subnets.isEmpty()) {
            m_statusLabel->setText(QStringLiteral("多子网模式: 正在发送到 %1").arg(broadcastAddr));
        } else {
            m_statusLabel->setText(
                QStringLiteral("多子网模式: 正在发送到 %1 个子网").arg(subnets.size()));
        }
        m_statusLabel->setStyleSheet(QStringLiteral("color: #E5C07B;"));

        for (const QString& subnet : subnets) {
            sendMagicPacket(normalized, subnet, port, deviceName);
        }
    } else {
        sendMagicPacket(normalized, broadcastAddr, port, deviceName);
    }

    Logger::instance().info("WOL",
        QString("Sent WOL packet to MAC=%1 broadcast=%2 port=%3")
            .arg(normalized, broadcastAddr).arg(port));
}

void WOLWidget::sendMagicPacket(const QString& mac, const QString& broadcastAddr,
                                int port, const QString& deviceName)
{
    QByteArray packet = buildMagicPacket(mac);
    if (packet.isEmpty()) {
        return;
    }

    qint64 bytesSent = m_socket->writeDatagram(packet, QHostAddress(broadcastAddr), port);

    // Record history
    WOLHistoryItem hist;
    hist.name = deviceName;
    hist.mac = mac;
    hist.targetIp = broadcastAddr;
    hist.broadcastAddr = broadcastAddr;
    hist.port = port;
    hist.sendTime = currentTimestamp();
    if (bytesSent > 0) {
        hist.status = 1;
        m_statusLabel->setText(
            QStringLiteral("唤醒包已发送 (%1 bytes) → %2").arg(bytesSent).arg(broadcastAddr));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #98C379;"));
    } else {
        hist.status = 2;
        hist.errorMsg = m_socket->errorString();
        m_statusLabel->setText(
            QStringLiteral("发送失败: %1").arg(m_socket->errorString()));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #E06C75;"));
    }
    insertHistory(hist);

    // Update device status
    if (!deviceName.isEmpty()) {
        WOLDeviceInfo dev = queryDeviceByMac(mac);
        if (!dev.id.isEmpty()) {
            updateDeviceStatus(dev.id, hist.status);
        }
    }

    loadHistory();
    refreshHistoryTable();
    loadDevices();
    refreshDeviceTable();
}

// ============================================================================
// Magic Packet Construction
// ============================================================================

QByteArray WOLWidget::buildMagicPacket(const QString& mac) const
{
    QStringList parts = parseMacAddress(mac);
    if (parts.size() != 6) {
        return QByteArray();
    }

    QByteArray packet;
    packet.resize(102);

    // 6 bytes of 0xFF
    for (int i = 0; i < 6; ++i) {
        packet[i] = static_cast<char>(0xFF);
    }

    // 16 repetitions of the MAC address
    for (int rep = 0; rep < 16; ++rep) {
        for (int i = 0; i < 6; ++i) {
            bool ok;
            quint8 byte = static_cast<quint8>(parts[i].toInt(&ok, 16));
            packet[6 + rep * 6 + i] = static_cast<char>(byte);
        }
    }

    return packet;
}

QStringList WOLWidget::parseMacAddress(const QString& mac) const
{
    // Remove common separators
    QString cleaned = mac.toUpper();
    cleaned.remove(QRegularExpression(QStringLiteral("[-:.]")));
    cleaned = cleaned.trimmed();

    if (cleaned.length() != 12) {
        return QStringList();
    }

    QStringList parts;
    for (int i = 0; i < 12; i += 2) {
        bool ok;
        cleaned.mid(i, 2).toInt(&ok, 16);
        if (!ok) {
            return QStringList();
        }
        parts.append(cleaned.mid(i, 2));
    }
    return parts;
}

QString WOLWidget::normalizeMac(const QString& mac) const
{
    QStringList parts = parseMacAddress(mac);
    if (parts.size() != 6) {
        return QString();
    }
    return parts.join(QStringLiteral(":"));
}

QStringList WOLWidget::getSubnetBroadcasts(const QString& baseBroadcast) const
{
    // Common subnet broadcast addresses
    QStringList subnets;
    subnets << QStringLiteral("255.255.255.255");

    // If base is already 255.255.255.255, add common subnets
    if (baseBroadcast == QStringLiteral("255.255.255.255")) {
        subnets << QStringLiteral("192.168.0.255")
                << QStringLiteral("192.168.1.255")
                << QStringLiteral("192.168.2.255")
                << QStringLiteral("10.0.0.255")
                << QStringLiteral("10.0.1.255")
                << QStringLiteral("172.16.0.255")
                << QStringLiteral("172.16.1.255");
    } else {
        subnets << baseBroadcast;
    }

    // Deduplicate
    QStringList result;
    for (const QString& s : subnets) {
        if (!result.contains(s)) {
            result.append(s);
        }
    }
    return result;
}

// ============================================================================
// Slot: Add Device
// ============================================================================

void WOLWidget::onAddDevice()
{
    WOLDeviceInfo info;
    info.id = generateId();
    info.createTime = currentTimestamp();
    info.updateTime = info.createTime;
    showDeviceDialog(info, false);
}

// ============================================================================
// Slot: Edit Device
// ============================================================================

void WOLWidget::onEditDevice()
{
    int row = m_deviceTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem* item = m_deviceTable->item(row, DEV_COL_NAME);
    if (!item) return;

    QString deviceId = item->data(Qt::UserRole).toString();
    WOLDeviceInfo info = queryDeviceById(deviceId);
    if (info.id.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("编辑设备"), QStringLiteral("设备不存在"));
        return;
    }
    showDeviceDialog(info, true);
}

// ============================================================================
// Slot: Delete Device
// ============================================================================

void WOLWidget::onDeleteDevice()
{
    int row = m_deviceTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem* item = m_deviceTable->item(row, DEV_COL_NAME);
    if (!item) return;

    QString deviceId = item->data(Qt::UserRole).toString();
    QString deviceName = item->text();

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, QStringLiteral("删除设备"),
        QString("确定要删除设备 \"%1\" 吗？").arg(deviceName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (deleteDevice(deviceId)) {
        Logger::instance().info("WOL", "Deleted WOL device: " + deviceName);
        loadDevices();
        refreshDeviceTable();
    }
}

// ============================================================================
// Slot: Device Table Double Click (Quick Send)
// ============================================================================

void WOLWidget::onDeviceTableDoubleClicked(int row, int /*column*/)
{
    if (row < 0) return;

    QTableWidgetItem* macItem = m_deviceTable->item(row, DEV_COL_MAC);
    if (!macItem) return;

    QString mac = macItem->text();
    m_macEdit->setText(mac);

    QTableWidgetItem* ipItem = m_deviceTable->item(row, DEV_COL_IP);
    if (ipItem && !ipItem->text().isEmpty()) {
        // Try to derive broadcast from IP
        QString ip = ipItem->text();
        QStringList parts = ip.split('.');
        if (parts.size() == 4) {
            parts[3] = QStringLiteral("255");
            m_broadcastEdit->setText(parts.join('.'));
        }
    }

    onSendWOL();
}

// ============================================================================
// Slot: Device Table Selection Changed
// ============================================================================

void WOLWidget::onDeviceTableSelectionChanged()
{
    int row = m_deviceTable->currentRow();
    bool hasSelection = (row >= 0);
    m_editDeviceBtn->setEnabled(hasSelection);
    m_deleteDeviceBtn->setEnabled(hasSelection);
}

// ============================================================================
// Slot: Clear History
// ============================================================================

void WOLWidget::onClearHistory()
{
    QMessageBox::StandardButton ret = QMessageBox::question(
        this, QStringLiteral("清空历史"),
        QStringLiteral("确定要清空所有发送历史吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (clearHistory()) {
        Logger::instance().info("WOL", "History cleared");
        loadHistory();
        refreshHistoryTable();
    }
}

// ============================================================================
// Slot: Export History
// ============================================================================

void WOLWidget::onExportHistory()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出历史"), QStringLiteral("wol_history.csv"),
        QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法写入文件: ") + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "设备名称,MAC地址,目标IP,广播地址,端口,发送时间,状态\n";

    QList<WOLHistoryItem> items = queryAllHistory();
    for (const auto& hist : items) {
        QString statusText = (hist.status == 1) ? QStringLiteral("成功") : QStringLiteral("失败");
        QStringList row;
        row << QStringLiteral("\"%1\"").arg(hist.name)
            << hist.mac
            << hist.targetIp
            << hist.broadcastAddr
            << QString::number(hist.port)
            << hist.sendTime
            << statusText;
        stream << row.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("WOL",
        QString("Exported %1 history entries to %2").arg(items.size()).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出完成"),
        QString("已导出 %1 条历史记录").arg(items.size()));
}

// ============================================================================
// Slot: History Table Double Click (Re-send)
// ============================================================================

void WOLWidget::onHistoryTableDoubleClicked(int row, int /*column*/)
{
    if (row < 0) return;

    QTableWidgetItem* macItem = m_historyTable->item(row, HIST_COL_MAC);
    QTableWidgetItem* broadcastItem = m_historyTable->item(row, HIST_COL_BROADCAST);
    QTableWidgetItem* portItem = m_historyTable->item(row, HIST_COL_PORT);

    if (macItem) {
        m_macEdit->setText(macItem->text());
    }
    if (broadcastItem) {
        m_broadcastEdit->setText(broadcastItem->text());
    }
    if (portItem) {
        m_portSpin->setValue(portItem->text().toInt());
    }

    // Scroll to top
    m_macEdit->setFocus();
}

// ============================================================================
// Device Dialog
// ============================================================================

void WOLWidget::showDeviceDialog(WOLDeviceInfo& info, bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? QStringLiteral("编辑设备") : QStringLiteral("添加设备"));
    dialog.setMinimumWidth(420);
    dialog.setModal(true);

    auto* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);

    auto* formGroup = new QGroupBox(QStringLiteral("设备信息"));
    auto* form = new QFormLayout(formGroup);
    form->setSpacing(8);

    auto* nameEdit = new QLineEdit(info.name);
    nameEdit->setPlaceholderText(QStringLiteral("设备名称"));
    form->addRow(QStringLiteral("名称:"), nameEdit);

    auto* macEdit = new QLineEdit(info.mac);
    macEdit->setPlaceholderText(QStringLiteral("XX:XX:XX:XX:XX:XX"));
    form->addRow(QStringLiteral("MAC 地址:"), macEdit);

    auto* ipEdit = new QLineEdit(info.ip);
    ipEdit->setPlaceholderText(QStringLiteral("192.168.1.100"));
    form->addRow(QStringLiteral("IP 地址:"), ipEdit);

    mainLayout->addWidget(formGroup);

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(isEdit
        ? QStringLiteral("保存") : QStringLiteral("添加"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) return;

    info.name = nameEdit->text().trimmed();
    info.mac = normalizeMac(macEdit->text().trimmed());
    info.ip = ipEdit->text().trimmed();
    info.updateTime = currentTimestamp();

    if (info.name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("设备名称不能为空"));
        return;
    }
    if (info.mac.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("MAC 地址格式无效"));
        return;
    }

    if (isEdit) {
        if (updateDevice(info)) {
            Logger::instance().info("WOL", "Updated WOL device: " + info.name);
            loadDevices();
            refreshDeviceTable();
        }
    } else {
        if (insertDevice(info)) {
            Logger::instance().info("WOL", "Added WOL device: " + info.name);
            loadDevices();
            refreshDeviceTable();
        }
    }
}

void WOLWidget::updateDeviceStatus(const QString& deviceId, int status)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE wol_devices SET status = ?, last_woken = ?, update_time = ? WHERE id = ?");
    query.addBindValue(status);
    query.addBindValue(currentTimestamp());
    query.addBindValue(currentTimestamp());
    query.addBindValue(deviceId);

    if (!query.exec()) {
        Logger::instance().error("WOL",
            "Update device status failed: " + query.lastError().text());
    }
}

// ============================================================================
// CRUD: wol_devices
// ============================================================================

bool WOLWidget::insertDevice(const WOLDeviceInfo& info)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "INSERT INTO wol_devices (id, name, mac, ip, last_woken, status, "
        "create_time, update_time) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(info.id);
    query.addBindValue(info.name);
    query.addBindValue(info.mac);
    query.addBindValue(info.ip);
    query.addBindValue(info.lastWoken.isEmpty() ? QVariant() : info.lastWoken);
    query.addBindValue(info.status);
    query.addBindValue(info.createTime);
    query.addBindValue(info.updateTime);

    if (!query.exec()) {
        Logger::instance().error("WOL",
            "Insert wol_device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

bool WOLWidget::updateDevice(const WOLDeviceInfo& info)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "UPDATE wol_devices SET name = ?, mac = ?, ip = ?, last_woken = ?, "
        "status = ?, update_time = ? WHERE id = ?");
    query.addBindValue(info.name);
    query.addBindValue(info.mac);
    query.addBindValue(info.ip);
    query.addBindValue(info.lastWoken.isEmpty() ? QVariant() : info.lastWoken);
    query.addBindValue(info.status);
    query.addBindValue(info.updateTime);
    query.addBindValue(info.id);

    if (!query.exec()) {
        Logger::instance().error("WOL",
            "Update wol_device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

bool WOLWidget::deleteDevice(const QString& deviceId)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM wol_devices WHERE id = ?");
    query.addBindValue(deviceId);

    if (!query.exec()) {
        Logger::instance().error("WOL",
            "Delete wol_device failed: " + query.lastError().text());
        return false;
    }
    return true;
}

QList<WOLDeviceInfo> WOLWidget::queryAllDevices()
{
    QList<WOLDeviceInfo> result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, name, mac, ip, last_woken, status, "
               "create_time, update_time FROM wol_devices ORDER BY name");

    while (query.next()) {
        WOLDeviceInfo info;
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.mac = query.value(2).toString();
        info.ip = query.value(3).toString();
        info.lastWoken = query.value(4).toString();
        info.status = query.value(5).toInt();
        info.createTime = query.value(6).toString();
        info.updateTime = query.value(7).toString();
        result.append(info);
    }
    return result;
}

WOLDeviceInfo WOLWidget::queryDeviceById(const QString& deviceId)
{
    WOLDeviceInfo info;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, mac, ip, last_woken, status, "
                  "create_time, update_time FROM wol_devices WHERE id = ?");
    query.addBindValue(deviceId);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.mac = query.value(2).toString();
        info.ip = query.value(3).toString();
        info.lastWoken = query.value(4).toString();
        info.status = query.value(5).toInt();
        info.createTime = query.value(6).toString();
        info.updateTime = query.value(7).toString();
    }
    return info;
}

WOLDeviceInfo WOLWidget::queryDeviceByMac(const QString& mac)
{
    WOLDeviceInfo info;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, mac, ip, last_woken, status, "
                  "create_time, update_time FROM wol_devices WHERE mac = ?");
    query.addBindValue(mac);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.mac = query.value(2).toString();
        info.ip = query.value(3).toString();
        info.lastWoken = query.value(4).toString();
        info.status = query.value(5).toInt();
        info.createTime = query.value(6).toString();
        info.updateTime = query.value(7).toString();
    }
    return info;
}

// ============================================================================
// CRUD: wol_history
// ============================================================================

bool WOLWidget::insertHistory(const WOLHistoryItem& item)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "INSERT INTO wol_history (name, mac, target_ip, broadcast_addr, port, "
        "send_time, status, error_msg) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(item.name);
    query.addBindValue(item.mac);
    query.addBindValue(item.targetIp);
    query.addBindValue(item.broadcastAddr);
    query.addBindValue(item.port);
    query.addBindValue(item.sendTime);
    query.addBindValue(item.status);
    query.addBindValue(item.errorMsg.isEmpty() ? QVariant() : item.errorMsg);

    if (!query.exec()) {
        Logger::instance().error("WOL",
            "Insert wol_history failed: " + query.lastError().text());
        return false;
    }
    return true;
}

QList<WOLHistoryItem> WOLWidget::queryAllHistory()
{
    QList<WOLHistoryItem> result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, name, mac, target_ip, broadcast_addr, port, "
               "send_time, status, error_msg FROM wol_history ORDER BY id DESC");

    while (query.next()) {
        WOLHistoryItem item;
        item.id = query.value(0).toInt();
        item.name = query.value(1).toString();
        item.mac = query.value(2).toString();
        item.targetIp = query.value(3).toString();
        item.broadcastAddr = query.value(4).toString();
        item.port = query.value(5).toInt();
        item.sendTime = query.value(6).toString();
        item.status = query.value(7).toInt();
        item.errorMsg = query.value(8).toString();
        result.append(item);
    }
    return result;
}

bool WOLWidget::clearHistory()
{
    QSqlQuery query(DatabaseManager::instance().database());
    if (!query.exec("DELETE FROM wol_history")) {
        Logger::instance().error("WOL",
            "Clear wol_history failed: " + query.lastError().text());
        return false;
    }
    return true;
}

// ============================================================================
// Helpers
// ============================================================================

QString WOLWidget::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString WOLWidget::currentTimestamp()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}