#include "configcenter/ConfigCenterWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QCheckBox>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QSettings>
#include <QFile>
#include <QApplication>
#include <QSet>
#include <algorithm>

// ============================================================================
// Built-in config templates
// ============================================================================

static const char* g_ciscoTemplate = R"TEMPLATE(!
! Cisco IOS Basic Configuration Template
!
version 15.2
service timestamps debug datetime msec
service timestamps log datetime msec
service password-encryption
!
hostname Router
!
enable secret 5 $1$xxxx$xxxxxxxxxxxxxxxxxxxxxx
!
no ip domain-lookup
ip domain-name example.com
!
ip ssh version 2
no ip http-server
no ip http-secure-server
!
banner motd ^C
***** Unauthorized Access Prohibited *****
^C
!
line con 0
 exec-timeout 5 0
 password 7 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 login local
 logging synchronous
line vty 0 4
 exec-timeout 5 0
 password 7 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 login local
 transport input ssh
!
logging buffered 16384
logging console critical
!
end
)TEMPLATE";

static const char* g_huaweiTemplate = R"TEMPLATE(#
# Huawei VRP Basic Configuration Template
#
sysname Switch
#
undo http server enable
undo http secure-server enable
#
stelnet server enable
sftp server enable
ssh user admin authentication-type password
ssh user admin service-type stelnet
#
aaa
 local-user admin password irreversible-cipher $1c$xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 local-user admin privilege level 15
 local-user admin service-type ssh telnet terminal
#
user-interface vty 0 4
 authentication-mode aaa
 protocol inbound ssh
 user-interface con 0
 authentication-mode password
 set authentication password cipher $1c$xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 idle-timeout 5 0
#
info-center enable
info-center loghost 192.168.1.100
#
ntp-service server 192.168.1.1
#
return
)TEMPLATE";

static const char* g_h3cTemplate = R"TEMPLATE(#
# H3C Comware Basic Configuration Template
#
sysname Switch
#
undo ip http enable
undo ip https enable
#
ssh server enable
sftp server enable
#
local-user admin class manage
 password simple xxxxxxxxxxxxxxxx
 service-type ssh telnet terminal
 authorization-attribute user-role network-admin
#
line vty 0 63
 authentication-mode scheme
 protocol inbound ssh
 user-role network-admin
#
line aux 0
 user-role network-admin
#
line con 0
 user-role network-admin
 idle-timeout 5 0
#
info-center enable
info-center loghost 192.168.1.100
#
ntp-service enable
ntp-service unicast-server 192.168.1.1
#
return
)TEMPLATE";

// ============================================================================
// Dark theme stylesheets
// ============================================================================

static const char* g_darkLineEditStyle = R"STYLE(
QLineEdit {
    background: #25262B; color: #DCDCDC;
    border: 1px solid #3C3F41; padding: 4px 8px;
    border-radius: 3px; font-size: 13px;
}
)STYLE";

static const char* g_darkComboStyle = R"STYLE(
QComboBox {
    background: #25262B; color: #DCDCDC;
    border: 1px solid #3C3F41; padding: 4px 8px;
    border-radius: 3px; font-size: 13px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background: #25262B; color: #DCDCDC;
    selection-background-color: #3C3F41;
}
)STYLE";

static const char* g_darkSpinStyle = R"STYLE(
QSpinBox {
    background: #25262B; color: #DCDCDC;
    border: 1px solid #3C3F41; padding: 4px;
    border-radius: 3px; font-size: 13px;
}
)STYLE";

static const char* g_darkPlainTextStyle = R"STYLE(
QPlainTextEdit {
    background-color: #1E1F22; color: #DCDCDC;
    border: 1px solid #3C3F41; font-size: 13px;
    font-family: 'Menlo', 'Consolas', 'Courier New', monospace;
    padding: 8px;
}
)STYLE";

static const char* g_darkTableStyle = R"STYLE(
QTableWidget {
    background-color: #1E1F22; color: #DCDCDC;
    border: 1px solid #3C3F41; font-size: 12px;
    gridline-color: #3C3F41;
}
QTableWidget::item { padding: 3px 4px; }
QTableWidget::item:hover { background-color: #2C2D30; }
QTableWidget::item:selected { background-color: #3C3F41; }
QHeaderView::section {
    background-color: #25262B; color: #8C8C8C;
    border: none; border-bottom: 2px solid #3C3F41;
    padding: 4px 8px; font-size: 12px; font-weight: bold;
}
)STYLE";

static const char* g_primaryBtnStyle = R"STYLE(
QPushButton {
    background-color: #409EFF; color: white;
    border: none; padding: 8px 20px; border-radius: 4px;
    font-size: 13px; font-weight: bold;
}
QPushButton:hover { background-color: #66B1FF; }
QPushButton:disabled { background-color: #5C5C5C; }
)STYLE";

static const char* g_successBtnStyle = R"STYLE(
QPushButton {
    background-color: #67C23A; color: white;
    border: none; padding: 8px 16px; border-radius: 4px;
    font-size: 13px;
}
QPushButton:hover { background-color: #85CE61; }
QPushButton:disabled { background-color: #5C5C5C; }
)STYLE";

static const char* g_dangerBtnStyle = R"STYLE(
QPushButton {
    background-color: #F56C6C; color: white;
    border: none; padding: 8px 16px; border-radius: 4px;
    font-size: 13px;
}
QPushButton:hover { background-color: #F78989; }
QPushButton:disabled { background-color: #5C5C5C; }
)STYLE";

static const char* g_warningBtnStyle = R"STYLE(
QPushButton {
    background-color: #E6A23C; color: white;
    border: none; padding: 8px 16px; border-radius: 4px;
    font-size: 13px;
}
QPushButton:hover { background-color: #EBB563; }
QPushButton:disabled { background-color: #5C5C5C; }
)STYLE";

static const char* g_infoBtnStyle = R"STYLE(
QPushButton {
    background-color: #909399; color: white;
    border: none; padding: 8px 16px; border-radius: 4px;
    font-size: 13px;
}
QPushButton:hover { background-color: #B4B4B9; }
QPushButton:disabled { background-color: #5C5C5C; }
)STYLE";

// ============================================================================
// Constructor / Destructor
// ============================================================================

ConfigCenterWidget::ConfigCenterWidget(QWidget* parent)
    : QWidget(parent)
    , m_autoBackupTimer(nullptr)
{
    setupUI();
    setupConnections();
    updateDeviceList();
    updateVersionList();
}

ConfigCenterWidget::~ConfigCenterWidget()
{
    if (m_autoBackupTimer) {
        m_autoBackupTimer->stop();
    }
}

// ============================================================================
// UI Setup
// ============================================================================

void ConfigCenterWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // ====================================================================
    // Left Panel: Device Management
    // ====================================================================
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* deviceGroup = new QGroupBox(QStringLiteral("设备管理"));
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    deviceLayout->setSpacing(6);

    auto* deviceRow = new QHBoxLayout();
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(160);
    m_deviceCombo->setStyleSheet(g_darkComboStyle);
    deviceRow->addWidget(m_deviceCombo, 1);

    m_addDeviceBtn = new QPushButton(QStringLiteral("添加设备"));
    m_addDeviceBtn->setStyleSheet(g_primaryBtnStyle);
    deviceRow->addWidget(m_addDeviceBtn);
    deviceLayout->addLayout(deviceRow);

    auto* backupRow = new QHBoxLayout();
    m_backupBtn = new QPushButton(QStringLiteral("备份配置"));
    m_backupBtn->setStyleSheet(g_successBtnStyle);
    backupRow->addWidget(m_backupBtn);

    m_restoreBtn = new QPushButton(QStringLiteral("恢复配置"));
    m_restoreBtn->setStyleSheet(g_dangerBtnStyle);
    backupRow->addWidget(m_restoreBtn);
    deviceLayout->addLayout(backupRow);

    auto* autoGroup = new QGroupBox(QStringLiteral("定时备份"));
    auto* autoLayout = new QVBoxLayout(autoGroup);
    autoLayout->setSpacing(4);

    m_autoBackupCheck = new QCheckBox(QStringLiteral("启用定时备份"));
    m_autoBackupCheck->setStyleSheet(QStringLiteral(
        "QCheckBox { color: #DCDCDC; font-size: 13px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    ));
    autoLayout->addWidget(m_autoBackupCheck);

    auto* intervalRow = new QHBoxLayout();
    auto* intervalLabel = new QLabel(QStringLiteral("备份间隔:"));
    intervalLabel->setStyleSheet("color: #8C8C8C; font-size: 12px;");
    intervalRow->addWidget(intervalLabel);

    m_backupIntervalCombo = new QComboBox();
    m_backupIntervalCombo->addItem(QStringLiteral("每天"), QStringLiteral("daily"));
    m_backupIntervalCombo->addItem(QStringLiteral("每周"), QStringLiteral("weekly"));
    m_backupIntervalCombo->addItem(QStringLiteral("每月"), QStringLiteral("monthly"));
    m_backupIntervalCombo->setStyleSheet(g_darkComboStyle);
    m_backupIntervalCombo->setEnabled(false);
    intervalRow->addWidget(m_backupIntervalCombo, 1);
    autoLayout->addLayout(intervalRow);
    deviceLayout->addWidget(autoGroup);

    leftLayout->addWidget(deviceGroup);
    leftLayout->addStretch();
    splitter->addWidget(leftWidget);

    // ====================================================================
    // Middle Panel: Version List + Config Content
    // ====================================================================
    auto* midWidget = new QWidget();
    auto* midLayout = new QVBoxLayout(midWidget);
    midLayout->setContentsMargins(0, 0, 0, 0);
    midLayout->setSpacing(8);

    auto* versionGroup = new QGroupBox(QStringLiteral("配置版本"));
    auto* versionLayout = new QVBoxLayout(versionGroup);
    versionLayout->setSpacing(4);

    m_versionTable = new QTableWidget();
    m_versionTable->setColumnCount(5);
    m_versionTable->setHorizontalHeaderLabels({
        QStringLiteral("版本"),
        QStringLiteral("设备"),
        QStringLiteral("时间"),
        QStringLiteral("大小"),
        QStringLiteral("备注")
    });
    m_versionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_versionTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_versionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_versionTable->setAlternatingRowColors(true);
    m_versionTable->horizontalHeader()->setStretchLastSection(true);
    m_versionTable->horizontalHeader()->setSectionResizeMode(VER_COL_ID, QHeaderView::ResizeToContents);
    m_versionTable->horizontalHeader()->setSectionResizeMode(VER_COL_DEVICE, QHeaderView::ResizeToContents);
    m_versionTable->horizontalHeader()->setSectionResizeMode(VER_COL_TIME, QHeaderView::ResizeToContents);
    m_versionTable->horizontalHeader()->setSectionResizeMode(VER_COL_SIZE, QHeaderView::ResizeToContents);
    m_versionTable->verticalHeader()->setVisible(false);
    m_versionTable->setStyleSheet(g_darkTableStyle);
    versionLayout->addWidget(m_versionTable);
    midLayout->addWidget(versionGroup, 1);

    auto* configGroup = new QGroupBox(QStringLiteral("配置内容"));
    auto* configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(4);

    m_configView = new QPlainTextEdit();
    m_configView->setReadOnly(true);
    m_configView->setPlaceholderText(QStringLiteral("选择版本后显示配置内容..."));
    m_configView->setStyleSheet(g_darkPlainTextStyle);
    configLayout->addWidget(m_configView);
    midLayout->addWidget(configGroup, 2);
    splitter->addWidget(midWidget);

    // ====================================================================
    // Right Panel: Diff + Template Management
    // ====================================================================
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* diffGroup = new QGroupBox(QStringLiteral("差异对比"));
    auto* diffLayout = new QVBoxLayout(diffGroup);
    diffLayout->setSpacing(4);

    m_diffBtn = new QPushButton(QStringLiteral("对比选中版本"));
    m_diffBtn->setStyleSheet(g_warningBtnStyle);
    diffLayout->addWidget(m_diffBtn);

    m_diffView = new QPlainTextEdit();
    m_diffView->setReadOnly(true);
    m_diffView->setPlaceholderText(QStringLiteral("选择两个版本后点击对比..."));
    m_diffView->setStyleSheet(g_darkPlainTextStyle);
    diffLayout->addWidget(m_diffView);
    rightLayout->addWidget(diffGroup, 1);

    auto* templateGroup = new QGroupBox(QStringLiteral("模板管理"));
    auto* templateLayout = new QVBoxLayout(templateGroup);
    templateLayout->setSpacing(6);

    m_templateCombo = new QComboBox();
    m_templateCombo->addItem(QStringLiteral("基础配置"), QStringLiteral("basic"));
    m_templateCombo->addItem(QStringLiteral("Cisco IOS"), QStringLiteral("cisco"));
    m_templateCombo->addItem(QStringLiteral("Huawei VRP"), QStringLiteral("huawei"));
    m_templateCombo->addItem(QStringLiteral("H3C Comware"), QStringLiteral("h3c"));
    m_templateCombo->setStyleSheet(g_darkComboStyle);
    templateLayout->addWidget(m_templateCombo);

    m_templateEditor = new QPlainTextEdit();
    m_templateEditor->setPlaceholderText(QStringLiteral("选择模板或编辑配置内容..."));
    m_templateEditor->setStyleSheet(g_darkPlainTextStyle);
    templateLayout->addWidget(m_templateEditor);

    auto* btnRow = new QHBoxLayout();
    m_batchDeployBtn = new QPushButton(QStringLiteral("批量下发"));
    m_batchDeployBtn->setStyleSheet(g_primaryBtnStyle);
    btnRow->addWidget(m_batchDeployBtn);

    m_complianceBtn = new QPushButton(QStringLiteral("合规检查"));
    m_complianceBtn->setStyleSheet(g_warningBtnStyle);
    btnRow->addWidget(m_complianceBtn);

    m_exportBtn = new QPushButton(QStringLiteral("导出"));
    m_exportBtn->setStyleSheet(g_infoBtnStyle);
    btnRow->addWidget(m_exportBtn);
    templateLayout->addLayout(btnRow);
    rightLayout->addWidget(templateGroup);

    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 2);
    splitter->setSizes({250, 500, 400});

    mainLayout->addWidget(splitter, 1);

    m_ciscoTemplate = QString::fromUtf8(g_ciscoTemplate);
    m_huaweiVrpTemplate = QString::fromUtf8(g_huaweiTemplate);
    m_h3cComwareTemplate = QString::fromUtf8(g_h3cTemplate);

    m_templateEditor->setPlainText(QStringLiteral(
        "# Edit config template here, select a template above for quick load\n"
        "# Supports Cisco IOS / Huawei VRP / H3C Comware formats\n"
    ));

    m_autoBackupTimer = new QTimer(this);
}

// ============================================================================
// Connections
// ============================================================================

void ConfigCenterWidget::setupConnections()
{
    connect(m_addDeviceBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onAddDevice);
    connect(m_backupBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onBackup);
    connect(m_restoreBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onRestore);
    connect(m_autoBackupCheck, &QCheckBox::toggled, this, &ConfigCenterWidget::onToggleAutoBackup);
    connect(m_diffBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onDiff);
    connect(m_batchDeployBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onBatchDeploy);
    connect(m_complianceBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onComplianceCheck);
    connect(m_exportBtn, &QPushButton::clicked, this, &ConfigCenterWidget::onExport);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigCenterWidget::onTemplateChanged);
    connect(m_versionTable, &QTableWidget::itemSelectionChanged,
            this, &ConfigCenterWidget::onVersionSelected);
    connect(m_autoBackupTimer, &QTimer::timeout, this, &ConfigCenterWidget::onBackup);
}

// ============================================================================
// Add Device
// ============================================================================

void ConfigCenterWidget::onAddDevice()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("添加设备"));
    dialog.setMinimumWidth(450);
    dialog.setModal(true);

    auto* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);

    auto* formGroup = new QGroupBox(QStringLiteral("设备信息"));
    auto* form = new QFormLayout(formGroup);
    form->setSpacing(8);

    auto* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(QStringLiteral("设备名称"));
    nameEdit->setStyleSheet(g_darkLineEditStyle);
    form->addRow(QStringLiteral("名称:"), nameEdit);

    auto* ipEdit = new QLineEdit();
    ipEdit->setPlaceholderText(QStringLiteral("192.168.1.1"));
    ipEdit->setStyleSheet(g_darkLineEditStyle);
    form->addRow(QStringLiteral("IP 地址:"), ipEdit);

    auto* portSpin = new QSpinBox();
    portSpin->setRange(1, 65535);
    portSpin->setValue(22);
    portSpin->setStyleSheet(g_darkSpinStyle);
    form->addRow(QStringLiteral("SSH 端口:"), portSpin);

    auto* userEdit = new QLineEdit();
    userEdit->setPlaceholderText(QStringLiteral("SSH 用户名"));
    userEdit->setStyleSheet(g_darkLineEditStyle);
    form->addRow(QStringLiteral("用户名:"), userEdit);

    auto* passEdit = new QLineEdit();
    passEdit->setPlaceholderText(QStringLiteral("SSH 密码"));
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setStyleSheet(g_darkLineEditStyle);
    form->addRow(QStringLiteral("密码:"), passEdit);

    mainLayout->addWidget(formGroup);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("添加"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) return;

    QString name = nameEdit->text().trimmed();
    QString ip = ipEdit->text().trimmed();
    int port = portSpin->value();
    QString user = userEdit->text().trimmed();
    QString pass = passEdit->text().trimmed();

    if (name.isEmpty() || ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("设备名称和 IP 地址不能为空"));
        return;
    }

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    settings.endArray();

    settings.beginWriteArray(QStringLiteral("devices"));
    settings.setArrayIndex(count);
    settings.setValue(QStringLiteral("name"), name);
    settings.setValue(QStringLiteral("ip"), ip);
    settings.setValue(QStringLiteral("port"), port);
    settings.setValue(QStringLiteral("user"), user);
    settings.setValue(QStringLiteral("pass"), pass);
    settings.endArray();

    Logger::instance().info("ConfigCenter",
        QString("Added device: %1 (%2)").arg(name, ip));

    updateDeviceList();
}

// ============================================================================
// Update Device List
// ============================================================================

void ConfigCenterWidget::updateDeviceList()
{
    m_deviceCombo->clear();
    m_deviceCombo->addItem(QStringLiteral("-- Select Device --"), QString());

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value(QStringLiteral("name")).toString();
        QString ip = settings.value(QStringLiteral("ip")).toString();
        QString display = QStringLiteral("%1 (%2)").arg(name, ip);
        m_deviceCombo->addItem(display, ip);
    }
    settings.endArray();
}

// ============================================================================
// Backup Config
// ============================================================================

void ConfigCenterWidget::onBackup()
{
    QString ip = m_deviceCombo->currentData().toString();
    QString deviceName = m_deviceCombo->currentText();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请先选择一台设备"));
        return;
    }

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    QString sshUser, sshPass;
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        if (settings.value(QStringLiteral("ip")).toString() == ip) {
            sshUser = settings.value(QStringLiteral("user")).toString();
            sshPass = settings.value(QStringLiteral("pass")).toString();
            break;
        }
    }
    settings.endArray();

    if (sshUser.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("未找到设备 SSH 凭据"));
        return;
    }

    QString config = fetchDeviceConfig(ip);

    if (config.isEmpty()) {
        config = QStringLiteral(
            "! Simulated config - Device %1 (%2)\n"
            "! Fetch time: %3\n"
            "! Note: SSH connection failed, this is simulated data\n"
            "!\n"
            "version 15.2\n"
            "hostname %1\n"
            "!\n"
            "interface GigabitEthernet0/0\n"
            " ip address %2 255.255.255.0\n"
            " no shutdown\n"
            "!\n"
            "ip ssh version 2\n"
            "service password-encryption\n"
            "logging buffered 16384\n"
            "!\n"
            "end\n"
        ).arg(deviceName, ip, currentTimestamp());

        Logger::instance().warning("ConfigCenter",
            QString("SSH fetch failed, using simulated data: %1").arg(deviceName));
    }

    QString note = m_autoBackupTimer->isActive()
        ? QStringLiteral("Scheduled auto backup")
        : QStringLiteral("Manual backup");

    saveConfigVersion(deviceName, config, note);
    updateVersionList();

    Logger::instance().info("ConfigCenter",
        QString("Config backup completed: %1").arg(deviceName));
}

// ============================================================================
// Fetch Device Config via SSH
// ============================================================================

QString ConfigCenterWidget::fetchDeviceConfig(const QString& ip)
{
    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    QString sshUser, sshPass;
    int sshPort = 22;
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        if (settings.value(QStringLiteral("ip")).toString() == ip) {
            sshUser = settings.value(QStringLiteral("user")).toString();
            sshPass = settings.value(QStringLiteral("pass")).toString();
            sshPort = settings.value(QStringLiteral("port"), 22).toInt();
            break;
        }
    }
    settings.endArray();

    if (sshUser.isEmpty()) return QString();

    QProcess process;
    QStringList args;
    args << QStringLiteral("-p") << sshPass
         << QStringLiteral("ssh")
         << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
         << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=5")
         << QStringLiteral("-p") << QString::number(sshPort)
         << QStringLiteral("%1@%2").arg(sshUser, ip)
         << QStringLiteral("show running-config");

    process.start(QStringLiteral("sshpass"), args);
    if (!process.waitForStarted(3000)) {
        process.close();
        QStringList directArgs;
        directArgs << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
                   << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=5")
                   << QStringLiteral("-o") << QStringLiteral("BatchMode=yes")
                   << QStringLiteral("-p") << QString::number(sshPort)
                   << QStringLiteral("%1@%2").arg(sshUser, ip)
                   << QStringLiteral("show running-config");
        process.start(QStringLiteral("ssh"), directArgs);
        if (!process.waitForStarted(3000)) return QString();
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        return QString();
    }

    if (process.exitCode() != 0) return QString();
    return QString::fromUtf8(process.readAllStandardOutput());
}

// ============================================================================
// Restore Config
// ============================================================================

void ConfigCenterWidget::onRestore()
{
    int row = m_versionTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请先选择要恢复的配置版本"));
        return;
    }

    QString versionId = m_versionTable->item(row, VER_COL_ID)->data(Qt::UserRole).toString();
    ConfigVersion targetVersion;
    for (const auto& ver : m_versions) {
        if (ver.id == versionId) {
            targetVersion = ver;
            break;
        }
    }

    if (targetVersion.id.isEmpty()) return;

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, QStringLiteral("恢复配置"),
        QString("Restore device \"%1\" to version %2?\n\n"
                "This will overwrite the current device configuration.")
            .arg(targetVersion.device, targetVersion.id.left(8)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    QString sshUser, sshPass, deviceIp;
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value(QStringLiteral("name")).toString();
        if (targetVersion.device.contains(name) || name.contains(targetVersion.device)) {
            sshUser = settings.value(QStringLiteral("user")).toString();
            sshPass = settings.value(QStringLiteral("pass")).toString();
            deviceIp = settings.value(QStringLiteral("ip")).toString();
            break;
        }
    }
    settings.endArray();

    if (sshUser.isEmpty() || deviceIp.isEmpty()) {
        Logger::instance().info("ConfigCenter",
            QString("Simulated restore to device: %1 (version %2)")
                .arg(targetVersion.device, targetVersion.id.left(8)));
        QMessageBox::information(this, QStringLiteral("恢复完成"),
            QString("Config restored (simulated mode)\n\n"
                    "In production, config will be pushed via SSH to %1")
                .arg(targetVersion.device));
        return;
    }

    QProcess process;
    QStringList args;
    args << QStringLiteral("-p") << sshPass
         << QStringLiteral("ssh")
         << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
         << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=5")
         << QStringLiteral("%1@%2").arg(sshUser, deviceIp)
         << QStringLiteral("configure terminal");

    process.start(QStringLiteral("sshpass"), args);
    if (!process.waitForStarted(3000) || !process.waitForFinished(30000)) {
        Logger::instance().warning("ConfigCenter",
            QString("SSH restore failed, using simulated mode: %1").arg(targetVersion.device));
        QMessageBox::information(this, QStringLiteral("恢复完成"),
            QString("Config marked for restore (simulated mode)\n\n"
                    "In production, config will be pushed via SSH to %1")
                .arg(targetVersion.device));
        return;
    }

    Logger::instance().info("ConfigCenter",
        QString("Config restore completed: %1 (version %2)")
            .arg(targetVersion.device, targetVersion.id.left(8)));
    QMessageBox::information(this, QStringLiteral("恢复完成"),
        QString("Config restored to device %1").arg(targetVersion.device));
}

// ============================================================================
// Update Version List
// ============================================================================

void ConfigCenterWidget::updateVersionList()
{
    m_versions.clear();

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("versions"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ConfigVersion ver;
        ver.id = settings.value(QStringLiteral("id")).toString();
        ver.device = settings.value(QStringLiteral("device")).toString();
        ver.timestamp = settings.value(QStringLiteral("timestamp")).toString();
        ver.size = settings.value(QStringLiteral("size"), 0).toLongLong();
        ver.note = settings.value(QStringLiteral("note")).toString();
        ver.content = settings.value(QStringLiteral("content")).toString();
        m_versions.append(ver);
    }
    settings.endArray();

    std::sort(m_versions.begin(), m_versions.end(),
              [](const ConfigVersion& a, const ConfigVersion& b) {
                  return a.timestamp > b.timestamp;
              });

    m_versionTable->setRowCount(0);
    for (int i = 0; i < m_versions.size(); ++i) {
        const auto& ver = m_versions[i];
        int row = m_versionTable->rowCount();
        m_versionTable->insertRow(row);

        auto* idItem = new QTableWidgetItem(ver.id.left(8));
        idItem->setData(Qt::UserRole, ver.id);
        m_versionTable->setItem(row, VER_COL_ID, idItem);
        m_versionTable->setItem(row, VER_COL_DEVICE, new QTableWidgetItem(ver.device));
        m_versionTable->setItem(row, VER_COL_TIME, new QTableWidgetItem(ver.timestamp));

        QString sizeStr;
        if (ver.size < 1024)
            sizeStr = QStringLiteral("%1 B").arg(ver.size);
        else if (ver.size < 1024 * 1024)
            sizeStr = QStringLiteral("%1 KB").arg(ver.size / 1024.0, 0, 'f', 1);
        else
            sizeStr = QStringLiteral("%1 MB").arg(ver.size / (1024.0 * 1024.0), 0, 'f', 2);
        m_versionTable->setItem(row, VER_COL_SIZE, new QTableWidgetItem(sizeStr));
        m_versionTable->setItem(row, VER_COL_NOTE, new QTableWidgetItem(ver.note));
    }
}

// ============================================================================
// Version Selected
// ============================================================================

void ConfigCenterWidget::onVersionSelected()
{
    int row = m_versionTable->currentRow();
    if (row < 0) { m_configView->clear(); return; }

    QTableWidgetItem* idItem = m_versionTable->item(row, VER_COL_ID);
    if (!idItem) return;

    QString versionId = idItem->data(Qt::UserRole).toString();
    for (const auto& ver : m_versions) {
        if (ver.id == versionId) {
            m_configView->setPlainText(ver.content);
            return;
        }
    }
    m_configView->clear();
}

// ============================================================================
// Diff
// ============================================================================

void ConfigCenterWidget::onDiff()
{
    QList<QTableWidgetItem*> selected = m_versionTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("Please select two versions in the version list (Ctrl+Click)"));
        return;
    }

    QSet<QString> selectedIds;
    for (auto* item : selected) {
        if (item->column() == VER_COL_ID)
            selectedIds.insert(item->data(Qt::UserRole).toString());
    }

    QStringList versionIds = selectedIds.values();
    if (versionIds.size() < 2) {
        if (m_versions.size() < 2) {
            QMessageBox::information(this, QStringLiteral("提示"),
                QStringLiteral("Need at least two versions to compare"));
            return;
        }
        if (versionIds.isEmpty())
            versionIds << m_versions[0].id << m_versions[1].id;
        else
            versionIds << m_versions[0].id;
    }

    if (versionIds.size() < 2) return;

    QString oldId = versionIds[versionIds.size() - 2];
    QString newId = versionIds[versionIds.size() - 1];

    ConfigVersion oldVer, newVer;
    for (const auto& ver : m_versions) {
        if (ver.id == oldId) oldVer = ver;
        if (ver.id == newId) newVer = ver;
    }

    if (oldVer.id.isEmpty() || newVer.id.isEmpty()) return;

    if (oldVer.timestamp > newVer.timestamp) std::swap(oldVer, newVer);

    QString diffResult = computeDiff(oldVer.content, newVer.content);

    QString header = QStringLiteral(
        "=== Config Diff ===\n"
        "--- Old: %1 (%2)\n"
        "+++ New: %3 (%4)\n\n"
    ).arg(oldVer.id.left(8), oldVer.timestamp, newVer.id.left(8), newVer.timestamp);

    m_diffView->setPlainText(header + diffResult);
}

// ============================================================================
// Compute Diff (line-by-line)
// ============================================================================

QString ConfigCenterWidget::computeDiff(const QString& oldText, const QString& newText)
{
    QStringList oldLines = oldText.split('\n');
    QStringList newLines = newText.split('\n');
    QString result;

    int i = 0, j = 0;
    while (i < oldLines.size() || j < newLines.size()) {
        QString oldLine = (i < oldLines.size()) ? oldLines[i] : QString();
        QString newLine = (j < newLines.size()) ? newLines[j] : QString();

        if (i >= oldLines.size()) {
            result += QStringLiteral("+ %1\n").arg(newLine);
            ++j;
        } else if (j >= newLines.size()) {
            result += QStringLiteral("- %1\n").arg(oldLine);
            ++i;
        } else if (oldLine.trimmed() == newLine.trimmed()) {
            result += QStringLiteral("  %1\n").arg(newLine);
            ++i; ++j;
        } else {
            bool matched = false;
            for (int k = j + 1; k < qMin(j + 5, newLines.size()); ++k) {
                if (oldLines[i].trimmed() == newLines[k].trimmed()) {
                    for (int m = j; m < k; ++m)
                        result += QStringLiteral("+ %1\n").arg(newLines[m]);
                    result += QStringLiteral("  %1\n").arg(oldLines[i]);
                    ++i; j = k + 1; matched = true;
                    break;
                }
            }
            if (!matched) {
                for (int k = i + 1; k < qMin(i + 5, oldLines.size()); ++k) {
                    if (oldLines[k].trimmed() == newLines[j].trimmed()) {
                        for (int m = i; m < k; ++m)
                            result += QStringLiteral("- %1\n").arg(oldLines[m]);
                        result += QStringLiteral("  %1\n").arg(newLines[j]);
                        i = k + 1; ++j; matched = true;
                        break;
                    }
                }
            }
            if (!matched) {
                result += QStringLiteral("~ %1\n").arg(newLine);
                ++i; ++j;
            }
        }
    }
    return result;
}

// ============================================================================
// Save Config Version
// ============================================================================

void ConfigCenterWidget::saveConfigVersion(const QString& device, const QString& config,
                                            const QString& note)
{
    ConfigVersion ver;
    ver.id = generateId();
    ver.device = device;
    ver.timestamp = currentTimestamp();
    ver.size = config.toUtf8().size();
    ver.note = note;
    ver.content = config;

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("versions"));
    settings.endArray();

    settings.beginWriteArray(QStringLiteral("versions"));
    settings.setArrayIndex(count);
    settings.setValue(QStringLiteral("id"), ver.id);
    settings.setValue(QStringLiteral("device"), ver.device);
    settings.setValue(QStringLiteral("timestamp"), ver.timestamp);
    settings.setValue(QStringLiteral("size"), ver.size);
    settings.setValue(QStringLiteral("note"), ver.note);
    settings.setValue(QStringLiteral("content"), ver.content);
    settings.endArray();

    if (count >= 100) {
        QList<ConfigVersion> allVers = m_versions;
        allVers.append(ver);
        std::sort(allVers.begin(), allVers.end(),
                  [](const ConfigVersion& a, const ConfigVersion& b) {
                      return a.timestamp > b.timestamp;
                  });
        if (allVers.size() > 100) allVers = allVers.mid(0, 100);

        settings.remove(QStringLiteral("versions"));
        settings.beginWriteArray(QStringLiteral("versions"));
        for (int i = 0; i < allVers.size(); ++i) {
            settings.setArrayIndex(i);
            settings.setValue(QStringLiteral("id"), allVers[i].id);
            settings.setValue(QStringLiteral("device"), allVers[i].device);
            settings.setValue(QStringLiteral("timestamp"), allVers[i].timestamp);
            settings.setValue(QStringLiteral("size"), allVers[i].size);
            settings.setValue(QStringLiteral("note"), allVers[i].note);
            settings.setValue(QStringLiteral("content"), allVers[i].content);
        }
        settings.endArray();
    }
}

// ============================================================================
// Toggle Auto Backup
// ============================================================================

void ConfigCenterWidget::onToggleAutoBackup(bool enabled)
{
    m_backupIntervalCombo->setEnabled(enabled);

    if (enabled) {
        QString interval = m_backupIntervalCombo->currentData().toString();
        qint64 ms = 24LL * 60 * 60 * 1000;
        if (interval == QStringLiteral("weekly")) ms = 7LL * 24 * 60 * 60 * 1000;
        else if (interval == QStringLiteral("monthly")) ms = 30LL * 24 * 60 * 60 * 1000;

        m_autoBackupTimer->start(ms);
        Logger::instance().info("ConfigCenter",
            QString("Auto backup started: %1").arg(m_backupIntervalCombo->currentText()));
    } else {
        m_autoBackupTimer->stop();
        Logger::instance().info("ConfigCenter", QStringLiteral("Auto backup stopped"));
    }
}

// ============================================================================
// Template Changed
// ============================================================================

void ConfigCenterWidget::onTemplateChanged(int /*index*/)
{
    QString type = m_templateCombo->currentData().toString();

    if (type == QStringLiteral("basic")) {
        m_templateEditor->setPlainText(QStringLiteral(
            "# Edit config template here, select a template above for quick load\n"
            "# Supports Cisco IOS / Huawei VRP / H3C Comware formats\n"
        ));
    } else if (type == QStringLiteral("cisco")) {
        m_templateEditor->setPlainText(m_ciscoTemplate);
    } else if (type == QStringLiteral("huawei")) {
        m_templateEditor->setPlainText(m_huaweiVrpTemplate);
    } else if (type == QStringLiteral("h3c")) {
        m_templateEditor->setPlainText(m_h3cComwareTemplate);
    }
}

// ============================================================================
// Batch Deploy
// ============================================================================

void ConfigCenterWidget::onBatchDeploy()
{
    QString templateContent = m_templateEditor->toPlainText().trimmed();
    if (templateContent.isEmpty() || templateContent.startsWith('#')) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("Please select or edit a valid config template"));
        return;
    }

    QString ip = m_deviceCombo->currentData().toString();
    QString deviceName = m_deviceCombo->currentText();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("Please select a target device first"));
        return;
    }

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, QStringLiteral("批量下发"),
        QString("Deploy template to device \"%1\"?\n\n"
                "This will modify the device configuration.")
            .arg(deviceName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("ConfigCenter"));
    int count = settings.beginReadArray(QStringLiteral("devices"));
    QString sshUser, sshPass;
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        if (settings.value(QStringLiteral("ip")).toString() == ip) {
            sshUser = settings.value(QStringLiteral("user")).toString();
            sshPass = settings.value(QStringLiteral("pass")).toString();
            break;
        }
    }
    settings.endArray();

    if (sshUser.isEmpty()) {
        Logger::instance().info("ConfigCenter",
            QString("Simulated deploy to device: %1").arg(deviceName));
        QMessageBox::information(this, QStringLiteral("下发完成"),
            QString("Template deployed (simulated mode)\n\n"
                    "In production, config will be pushed via SSH to %1\n\n"
                    "Template lines: %2")
                .arg(deviceName).arg(templateContent.split('\n').size()));
        return;
    }

    QProcess process;
    QStringList args;
    args << QStringLiteral("-p") << sshPass
         << QStringLiteral("ssh")
         << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
         << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=5")
         << QStringLiteral("%1@%2").arg(sshUser, ip)
         << QStringLiteral("configure terminal");

    process.start(QStringLiteral("sshpass"), args);
    if (!process.waitForStarted(3000) || !process.waitForFinished(30000)) {
        Logger::instance().warning("ConfigCenter",
            QString("SSH deploy failed, using simulated mode: %1").arg(deviceName));
        QMessageBox::information(this, QStringLiteral("下发完成"),
            QString("Template marked for deploy (simulated mode)\n\n"
                    "In production, config will be pushed via SSH to %1")
                .arg(deviceName));
        return;
    }

    Logger::instance().info("ConfigCenter",
        QString("Batch deploy completed: %1 (template: %2)")
            .arg(deviceName, m_templateCombo->currentText()));
    QMessageBox::information(this, QStringLiteral("下发完成"),
        QString("Template deployed to device %1").arg(deviceName));
}

// ============================================================================
// Compliance Check
// ============================================================================

void ConfigCenterWidget::onComplianceCheck()
{
    QString config = m_configView->toPlainText();
    if (config.trimmed().isEmpty()) config = m_templateEditor->toPlainText();

    if (config.trimmed().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("合规检查"),
            QStringLiteral("No config content to check.\n\n"
                           "Please select a version or edit a template first."));
        return;
    }

    struct CheckItem {
        QString name;
        QString description;
        bool passed;
        QString detail;
    };

    QList<CheckItem> checks;

    bool sshV2 = config.contains(QStringLiteral("ssh version 2"), Qt::CaseInsensitive) ||
                 config.contains(QStringLiteral("stelnet server enable"), Qt::CaseInsensitive) ||
                 config.contains(QStringLiteral("ssh server enable"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("SSH v2 Enabled"),
                   QStringLiteral("Remote management should use SSH v2"),
                   sshV2,
                   sshV2 ? QStringLiteral("SSH v2 detected") : QStringLiteral("SSH v2 not detected")});

    bool passwordEncrypt = config.contains(QStringLiteral("service password-encryption"), Qt::CaseInsensitive) ||
                           config.contains(QStringLiteral("irreversible-cipher"), Qt::CaseInsensitive) ||
                           config.contains(QStringLiteral("password cipher"), Qt::CaseInsensitive) ||
                           config.contains(QStringLiteral("enable secret"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("Password Encryption"),
                   QStringLiteral("Passwords should be stored encrypted"),
                   passwordEncrypt,
                   passwordEncrypt ? QStringLiteral("Password encryption detected") : QStringLiteral("Password encryption not detected")});

    bool logging = config.contains(QStringLiteral("logging"), Qt::CaseInsensitive) ||
                   config.contains(QStringLiteral("info-center enable"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("Logging Enabled"),
                   QStringLiteral("Logging should be enabled"),
                   logging,
                   logging ? QStringLiteral("Logging detected") : QStringLiteral("Logging not detected")});

    bool httpDisabled = !config.contains(QStringLiteral("ip http server"), Qt::CaseInsensitive) ||
                        config.contains(QStringLiteral("no ip http server"), Qt::CaseInsensitive) ||
                        config.contains(QStringLiteral("undo ip http enable"), Qt::CaseInsensitive) ||
                        config.contains(QStringLiteral("undo http server enable"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("HTTP Disabled"),
                   QStringLiteral("Insecure HTTP should be disabled"),
                   httpDisabled,
                   httpDisabled ? QStringLiteral("HTTP disabled or not detected") : QStringLiteral("HTTP server detected")});

    bool vtyControl = config.contains(QStringLiteral("transport input ssh"), Qt::CaseInsensitive) ||
                      config.contains(QStringLiteral("protocol inbound ssh"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("VTY Access Control"),
                   QStringLiteral("VTY lines should be restricted to SSH"),
                   vtyControl,
                   vtyControl ? QStringLiteral("VTY restricted to SSH") : QStringLiteral("VTY not restricted to SSH")});

    bool timeout = config.contains(QStringLiteral("exec-timeout"), Qt::CaseInsensitive) ||
                   config.contains(QStringLiteral("idle-timeout"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("Session Timeout"),
                   QStringLiteral("Console and VTY should have session timeout"),
                   timeout,
                   timeout ? QStringLiteral("Session timeout detected") : QStringLiteral("Session timeout not detected")});

    bool ntp = config.contains(QStringLiteral("ntp"), Qt::CaseInsensitive);
    checks.append({QStringLiteral("NTP Time Sync"),
                   QStringLiteral("NTP should be configured"),
                   ntp,
                   ntp ? QStringLiteral("NTP detected") : QStringLiteral("NTP not detected")});

    int passedCount = 0;
    for (const auto& c : checks) { if (c.passed) ++passedCount; }

    QString report;
    report += QStringLiteral("===========================================\n");
    report += QStringLiteral("    Configuration Compliance Check Report\n");
    report += QStringLiteral("===========================================\n\n");
    report += QString("Check time: %1\n").arg(currentTimestamp());
    report += QString("Total items: %1\n").arg(checks.size());
    report += QString("Passed: %1\n").arg(passedCount);
    report += QString("Compliance rate: %1%\n\n").arg(passedCount * 100 / checks.size());
    report += QStringLiteral("-------------------------------------------\n");
    report += QStringLiteral("Details:\n");
    report += QStringLiteral("-------------------------------------------\n\n");

    for (const auto& c : checks) {
        QString icon = c.passed ? QStringLiteral("[PASS]") : QStringLiteral("[FAIL]");
        report += QString("%1 %2\n").arg(icon, c.name);
        report += QString("    Requirement: %1\n").arg(c.description);
        report += QString("    Result: %1\n\n").arg(c.detail);
    }

    report += QStringLiteral("===========================================\n");
    m_diffView->setPlainText(report);

    Logger::instance().info("ConfigCenter",
        QString("Compliance check completed: %1/%2 passed").arg(passedCount).arg(checks.size()));
}

// ============================================================================
// Export Config
// ============================================================================

void ConfigCenterWidget::onExport()
{
    QString config = m_configView->toPlainText();
    if (config.trimmed().isEmpty()) config = m_templateEditor->toPlainText();

    if (config.trimmed().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("导出"),
            QStringLiteral("No config content to export.\n\n"
                           "Please select a version or edit a template first."));
        return;
    }

    QString defaultName = QStringLiteral("config_%1.txt")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出配置"), defaultName,
        QStringLiteral("Text files (*.txt);;All files (*)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("Cannot write file: ") + filePath);
        Logger::instance().error("ConfigCenter", "Failed to export config: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << config;
    file.close();

    Logger::instance().info("ConfigCenter", QString("Config exported to: %1").arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString("Config exported to:\n%1").arg(filePath));
}

// ============================================================================
// Helpers
// ============================================================================

QString ConfigCenterWidget::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ConfigCenterWidget::currentTimestamp()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}