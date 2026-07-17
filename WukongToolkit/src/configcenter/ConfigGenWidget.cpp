#include "configcenter/ConfigGenWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// ConfigGenWidget 实现
// ============================================================================

ConfigGenWidget::ConfigGenWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

ConfigGenWidget::~ConfigGenWidget() = default;

// ─── UI Setup ──────────────────────────────────────────────────────────

void ConfigGenWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 左侧: 参数区 ──
    auto* leftPanel = new QWidget();
    leftPanel->setFixedWidth(380);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 设备类型
    auto* vendorGroup = new QGroupBox("设备类型");
    auto* vendorLayout = new QFormLayout(vendorGroup);
    vendorLayout->setSpacing(6);

    m_vendorCombo = new QComboBox();
    m_vendorCombo->addItems({"Cisco IOS", "Huawei VRP", "H3C Comware"});
    vendorLayout->addRow("厂商:", m_vendorCombo);

    m_hostnameEdit = new QLineEdit();
    m_hostnameEdit->setPlaceholderText("Core-Switch-01");
    vendorLayout->addRow("主机名:", m_hostnameEdit);

    m_domainEdit = new QLineEdit();
    m_domainEdit->setPlaceholderText("example.com");
    vendorLayout->addRow("域名:", m_domainEdit);

    m_ntpServerEdit = new QLineEdit();
    m_ntpServerEdit->setPlaceholderText("ntp.example.com");
    vendorLayout->addRow("NTP 服务器:", m_ntpServerEdit);

    leftLayout->addWidget(vendorGroup);

    // 接口参数
    auto* intfGroup = new QGroupBox("接口配置");
    auto* intfLayout = new QFormLayout(intfGroup);
    intfLayout->setSpacing(6);

    m_vlanCountSpin = new QSpinBox();
    m_vlanCountSpin->setRange(1, 100);
    m_vlanCountSpin->setValue(4);
    intfLayout->addRow("VLAN 数量:", m_vlanCountSpin);

    m_vlanIpPrefixEdit = new QLineEdit();
    m_vlanIpPrefixEdit->setPlaceholderText("10.0.0.0/24");
    intfLayout->addRow("VLAN IP 前缀:", m_vlanIpPrefixEdit);

    m_mgmtIpEdit = new QLineEdit();
    m_mgmtIpEdit->setPlaceholderText("192.168.1.1");
    intfLayout->addRow("管理 IP:", m_mgmtIpEdit);

    m_mgmtMaskEdit = new QLineEdit();
    m_mgmtMaskEdit->setPlaceholderText("255.255.255.0");
    intfLayout->addRow("管理掩码:", m_mgmtMaskEdit);

    m_mgmtGatewayEdit = new QLineEdit();
    m_mgmtGatewayEdit->setPlaceholderText("192.168.1.254");
    intfLayout->addRow("默认网关:", m_mgmtGatewayEdit);

    leftLayout->addWidget(intfGroup);

    // 安全参数
    auto* secGroup = new QGroupBox("安全配置");
    auto* secLayout = new QFormLayout(secGroup);
    secLayout->setSpacing(6);

    m_adminUserEdit = new QLineEdit();
    m_adminUserEdit->setPlaceholderText("admin");
    secLayout->addRow("管理员用户:", m_adminUserEdit);

    m_adminPassEdit = new QLineEdit();
    m_adminPassEdit->setEchoMode(QLineEdit::Password);
    m_adminPassEdit->setPlaceholderText("输入密码");
    secLayout->addRow("管理员密码:", m_adminPassEdit);

    m_enableSshCheck = new QCheckBox("启用 SSH (v2)");
    m_enableSshCheck->setChecked(true);
    
    secLayout->addRow("", m_enableSshCheck);

    m_enableSnmpCheck = new QCheckBox("启用 SNMP (v2c)");
    m_enableSnmpCheck->setChecked(true);
    
    secLayout->addRow("", m_enableSnmpCheck);

    m_snmpCommunityEdit = new QLineEdit();
    m_snmpCommunityEdit->setPlaceholderText("public");
    secLayout->addRow("SNMP 团体字:", m_snmpCommunityEdit);

    m_snmpTrapHostEdit = new QLineEdit();
    m_snmpTrapHostEdit->setPlaceholderText("192.168.1.100");
    secLayout->addRow("SNMP Trap 主机:", m_snmpTrapHostEdit);

    leftLayout->addWidget(secGroup);

    // 路由参数
    auto* routeGroup = new QGroupBox("路由配置");
    auto* routeLayout = new QFormLayout(routeGroup);
    routeLayout->setSpacing(6);

    m_enableOspfCheck = new QCheckBox("启用 OSPF");
    
    routeLayout->addRow("", m_enableOspfCheck);

    m_ospfRouterIdEdit = new QLineEdit();
    m_ospfRouterIdEdit->setPlaceholderText("1.1.1.1");
    routeLayout->addRow("OSPF Router-ID:", m_ospfRouterIdEdit);

    m_ospfAreaEdit = new QLineEdit();
    m_ospfAreaEdit->setPlaceholderText("0");
    routeLayout->addRow("OSPF Area:", m_ospfAreaEdit);

    m_enableBgpCheck = new QCheckBox("启用 BGP");
    
    routeLayout->addRow("", m_enableBgpCheck);

    m_bgpAsEdit = new QLineEdit();
    m_bgpAsEdit->setPlaceholderText("65001");
    routeLayout->addRow("BGP AS:", m_bgpAsEdit);

    m_bgpNeighborEdit = new QLineEdit();
    m_bgpNeighborEdit->setPlaceholderText("10.0.0.2");
    routeLayout->addRow("BGP 邻居:", m_bgpNeighborEdit);

    leftLayout->addWidget(routeGroup);

    leftLayout->addStretch();

    // 滚动区域包装
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(leftPanel);
    scrollArea->setWidgetResizable(true);
    // ── 右侧: 输出区 ──
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    // 操作按钮
    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(8);

    m_generateBtn = new QPushButton("生成配置");
    m_generateBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 10px 24px; border-radius: 4px;"
        "  font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );

    m_exportBtn = new QPushButton("导出配置");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 10px 24px; border-radius: 4px;"
        "  font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
    );

    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #F85149;"
        "  border: 1px solid #30363D; padding: 10px 24px;"
        "  border-radius: 4px; font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );

    actionRow->addWidget(m_generateBtn);
    actionRow->addWidget(m_exportBtn);
    actionRow->addStretch();
    actionRow->addWidget(m_clearBtn);
    rightLayout->addLayout(actionRow);

    // 配置输出
    auto* outputGroup = new QGroupBox("生成的配置");
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(4);

    m_outputText = new QPlainTextEdit();
    m_outputText->setReadOnly(true);
    m_outputText->setPlaceholderText(QString::fromUtf8("点击「生成配置」按钮，将在此处显示生成的设备配置..."));
    m_outputText->setLineWrapMode(QPlainTextEdit::NoWrap);
    outputLayout->addWidget(m_outputText, 1);

    rightLayout->addWidget(outputGroup, 1);

    // ── 水平布局 ──
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(rightPanel, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void ConfigGenWidget::setupConnections()
{
    connect(m_generateBtn, &QPushButton::clicked, this, &ConfigGenWidget::onGenerate);
    connect(m_exportBtn, &QPushButton::clicked, this, &ConfigGenWidget::onExport);
    connect(m_clearBtn, &QPushButton::clicked, this, &ConfigGenWidget::onClear);
    connect(m_vendorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigGenWidget::onVendorChanged);
}

// ─── Slot: Vendor Changed ──────────────────────────────────────────────

void ConfigGenWidget::onVendorChanged(int index)
{
    Q_UNUSED(index)
    // 可以在不同厂商间切换默认值
}

// ─── Slot: Generate ────────────────────────────────────────────────────

void ConfigGenWidget::onGenerate()
{
    int vendor = m_vendorCombo->currentIndex();
    QString config;

    switch (vendor) {
    case 0: config = generateCiscoConfig(); break;
    case 1: config = generateHuaweiConfig(); break;
    case 2: config = generateH3CConfig(); break;
    default: config = generateBaseConfig(); break;
    }

    m_outputText->setPlainText(config);
    Logger::instance().info("ConfigGen", "生成" + m_vendorCombo->currentText() + "配置");
}

// ─── Cisco IOS Config ──────────────────────────────────────────────────

QString ConfigGenWidget::generateCiscoConfig()
{
    QString hostname = m_hostnameEdit->text().trimmed();
    if (hostname.isEmpty()) hostname = "Switch";

    QString config;
    config += "! ============================================\n";
    config += "! " + m_vendorCombo->currentText() + " 配置\n";
    config += "! 生成时间: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\n";
    config += "! ============================================\n\n";

    // 基础配置
    config += "version 15.2\n";
    config += "service timestamps debug datetime msec\n";
    config += "service timestamps log datetime msec\n";
    config += "service password-encryption\n";
    config += "!\n";
    config += "hostname " + hostname + "\n";
    config += "!\n";
    config += "boot-start-marker\n";
    config += "boot-end-marker\n";
    config += "!\n";

    // 域名
    QString domain = m_domainEdit->text().trimmed();
    if (!domain.isEmpty()) {
        config += "ip domain-name " + domain + "\n";
    }

    // NTP
    QString ntp = m_ntpServerEdit->text().trimmed();
    if (!ntp.isEmpty()) {
        config += "ntp server " + ntp + "\n";
    }
    config += "!\n";

    // 管理接口
    QString mgmtIp = m_mgmtIpEdit->text().trimmed();
    QString mgmtMask = m_mgmtMaskEdit->text().trimmed();
    QString mgmtGw = m_mgmtGatewayEdit->text().trimmed();

    if (!mgmtIp.isEmpty()) {
        config += "interface Vlan1\n";
        config += " description Management\n";
        config += " ip address " + mgmtIp + " " + (mgmtMask.isEmpty() ? "255.255.255.0" : mgmtMask) + "\n";
        config += " no shutdown\n";
        config += "!\n";
    }

    // VLAN 接口
    int vlanCount = m_vlanCountSpin->value();
    QString ipPrefix = m_vlanIpPrefixEdit->text().trimmed();
    for (int i = 1; i <= vlanCount; ++i) {
        config += "interface Vlan" + QString::number(i + 9) + "\n";
        if (!ipPrefix.isEmpty()) {
            config += " ip address " + ipPrefix + "." + QString::number(i) + " 255.255.255.0\n";
        }
        config += " no shutdown\n";
        config += "!\n";
    }

    // 上行接口
    config += "interface GigabitEthernet0/1\n";
    config += " description Uplink\n";
    config += " switchport mode trunk\n";
    config += " switchport trunk allowed vlan 1,";
    for (int i = 1; i <= vlanCount; ++i) {
        config += QString::number(i + 9);
        if (i < vlanCount) config += ",";
    }
    config += "\n no shutdown\n";
    config += "!\n";

    // 默认路由
    if (!mgmtGw.isEmpty()) {
        config += "ip route 0.0.0.0 0.0.0.0 " + mgmtGw + "\n";
        config += "!\n";
    }

    // SSH
    if (m_enableSshCheck->isChecked()) {
        QString user = m_adminUserEdit->text().trimmed();
        if (user.isEmpty()) user = "admin";
        QString pass = m_adminPassEdit->text().trimmed();
        if (pass.isEmpty()) pass = "Changeme123";

        config += "ip ssh version 2\n";
        config += "!\n";
        config += "username " + user + " privilege 15 secret " + pass + "\n";
        config += "!\n";
        config += "line vty 0 4\n";
        config += " login local\n";
        config += " transport input ssh\n";
        config += "!\n";
    }

    // SNMP
    if (m_enableSnmpCheck->isChecked()) {
        QString community = m_snmpCommunityEdit->text().trimmed();
        if (community.isEmpty()) community = "public";
        QString trapHost = m_snmpTrapHostEdit->text().trimmed();

        config += "snmp-server community " + community + " RO\n";
        if (!trapHost.isEmpty()) {
            config += "snmp-server host " + trapHost + " version 2c " + community + "\n";
        }
        config += "!\n";
    }

    // OSPF
    if (m_enableOspfCheck->isChecked()) {
        QString routerId = m_ospfRouterIdEdit->text().trimmed();
        QString area = m_ospfAreaEdit->text().trimmed();
        if (area.isEmpty()) area = "0";

        config += "router ospf 1\n";
        if (!routerId.isEmpty()) {
            config += " router-id " + routerId + "\n";
        }
        config += " network " + (mgmtIp.isEmpty() ? "192.168.1.0" : mgmtIp) + " 0.0.0.255 area " + area + "\n";
        config += "!\n";
    }

    // BGP
    if (m_enableBgpCheck->isChecked()) {
        QString as = m_bgpAsEdit->text().trimmed();
        QString neighbor = m_bgpNeighborEdit->text().trimmed();
        if (as.isEmpty()) as = "65001";

        config += "router bgp " + as + "\n";
        config += " bgp log-neighbor-changes\n";
        if (!neighbor.isEmpty()) {
            config += " neighbor " + neighbor + " remote-as " + as + "\n";
        }
        config += "!\n";
    }

    config += "end\n";
    return config;
}

// ─── Huawei VRP Config ─────────────────────────────────────────────────

QString ConfigGenWidget::generateHuaweiConfig()
{
    QString hostname = m_hostnameEdit->text().trimmed();
    if (hostname.isEmpty()) hostname = "Switch";

    QString config;
    config += "# ============================================\n";
    config += "# " + m_vendorCombo->currentText() + " 配置\n";
    config += "# 生成时间: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\n";
    config += "# ============================================\n\n";

    config += "sysname " + hostname + "\n";
    config += "#\n";

    // 管理 VLAN
    QString mgmtIp = m_mgmtIpEdit->text().trimmed();
    QString mgmtMask = m_mgmtMaskEdit->text().trimmed();
    if (!mgmtIp.isEmpty()) {
        config += "interface Vlanif 1\n";
        config += " description Management\n";
        config += " ip address " + mgmtIp + " " + (mgmtMask.isEmpty() ? "255.255.255.0" : mgmtMask) + "\n";
        config += "#\n";
    }

    // VLAN 配置
    int vlanCount = m_vlanCountSpin->value();
    config += "vlan batch 10";
    for (int i = 1; i < vlanCount; ++i) {
        config += " " + QString::number(10 + i);
    }
    config += "\n#\n";

    QString ipPrefix = m_vlanIpPrefixEdit->text().trimmed();
    for (int i = 0; i < vlanCount; ++i) {
        int vlanId = 10 + i;
        config += "interface Vlanif " + QString::number(vlanId) + "\n";
        if (!ipPrefix.isEmpty()) {
            config += " ip address " + ipPrefix + "." + QString::number(i + 1) + " 255.255.255.0\n";
        }
        config += "#\n";
    }

    // 上行接口
    config += "interface GigabitEthernet0/0/1\n";
    config += " description Uplink\n";
    config += " port link-type trunk\n";
    config += " port trunk allow-pass vlan 1";
    for (int i = 0; i < vlanCount; ++i) {
        config += " " + QString::number(10 + i);
    }
    config += "\n undo shutdown\n";
    config += "#\n";

    // 默认路由
    QString mgmtGw = m_mgmtGatewayEdit->text().trimmed();
    if (!mgmtGw.isEmpty()) {
        config += "ip route-static 0.0.0.0 0.0.0.0 " + mgmtGw + "\n";
        config += "#\n";
    }

    // SSH
    if (m_enableSshCheck->isChecked()) {
        QString user = m_adminUserEdit->text().trimmed();
        if (user.isEmpty()) user = "admin";
        QString pass = m_adminPassEdit->text().trimmed();
        if (pass.isEmpty()) pass = "Changeme123";

        config += "stelnet server enable\n";
        config += "ssh user " + user + " authentication-type password\n";
        config += "ssh user " + user + " service-type stelnet\n";
        config += "aaa\n";
        config += " local-user " + user + " password cipher " + pass + "\n";
        config += " local-user " + user + " privilege level 15\n";
        config += " local-user " + user + " service-type ssh\n";
        config += "#\n";
        config += "user-interface vty 0 4\n";
        config += " authentication-mode aaa\n";
        config += " protocol inbound ssh\n";
        config += "#\n";
    }

    // SNMP
    if (m_enableSnmpCheck->isChecked()) {
        QString community = m_snmpCommunityEdit->text().trimmed();
        if (community.isEmpty()) community = "public";
        QString trapHost = m_snmpTrapHostEdit->text().trimmed();

        config += "snmp-agent\n";
        config += "snmp-agent community read " + community + "\n";
        if (!trapHost.isEmpty()) {
            config += "snmp-agent target-host trap address udp-domain " + trapHost + " params securityname " + community + "\n";
        }
        config += "#\n";
    }

    // OSPF
    if (m_enableOspfCheck->isChecked()) {
        QString routerId = m_ospfRouterIdEdit->text().trimmed();
        QString area = m_ospfAreaEdit->text().trimmed();
        if (area.isEmpty()) area = "0";

        config += "ospf 1\n";
        if (!routerId.isEmpty()) {
            config += " router-id " + routerId + "\n";
        }
        config += " area " + area + "\n";
        config += "  network " + (mgmtIp.isEmpty() ? "192.168.1.0" : mgmtIp) + " 0.0.0.255\n";
        config += "#\n";
    }

    // BGP
    if (m_enableBgpCheck->isChecked()) {
        QString as = m_bgpAsEdit->text().trimmed();
        QString neighbor = m_bgpNeighborEdit->text().trimmed();
        if (as.isEmpty()) as = "65001";

        config += "bgp " + as + "\n";
        config += " router-id " + (m_ospfRouterIdEdit->text().trimmed().isEmpty() ? "1.1.1.1" : m_ospfRouterIdEdit->text().trimmed()) + "\n";
        if (!neighbor.isEmpty()) {
            config += " peer " + neighbor + " as-number " + as + "\n";
        }
        config += "#\n";
    }

    config += "return\n";
    return config;
}

// ─── H3C Comware Config ────────────────────────────────────────────────

QString ConfigGenWidget::generateH3CConfig()
{
    // H3C 语法与 Huawei 类似
    QString config = generateHuaweiConfig();
    config.replace("stelnet server enable", "ssh server enable");
    return config;
}

// ─── Base Config ───────────────────────────────────────────────────────

QString ConfigGenWidget::generateBaseConfig()
{
    return generateCiscoConfig();
}

// ─── Slot: Export ──────────────────────────────────────────────────────

void ConfigGenWidget::onExport()
{
    QString config = m_outputText->toPlainText();
    if (config.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的配置，请先生成。");
        return;
    }

    QString vendor = m_vendorCombo->currentText().replace(" ", "_");
    QString defaultName = m_hostnameEdit->text().trimmed().isEmpty()
                              ? "config" : m_hostnameEdit->text().trimmed();
    defaultName += "_" + vendor + ".cfg";

    QString filePath = QFileDialog::getSaveFileName(this, "导出配置", defaultName,
                                                     "配置文件 (*.cfg *.txt);;所有文件 (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << config;
    file.close();

    Logger::instance().info("ConfigGen", "导出配置: " + filePath);
    QMessageBox::information(this, "导出成功", "配置已导出到:\n" + filePath);
}

// ─── Slot: Clear ───────────────────────────────────────────────────────

void ConfigGenWidget::onClear()
{
    m_outputText->clear();
}
