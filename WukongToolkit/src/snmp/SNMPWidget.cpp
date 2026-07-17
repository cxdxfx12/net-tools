#include "snmp/SNMPWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QApplication>
#include <QDateTime>
#include <QLabel>

// ═══════════════════════════════════════════════════════════════════════════════
// SNMPWidget
// ═══════════════════════════════════════════════════════════════════════════════

SNMPWidget::SNMPWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_running(false)
{
    setupUI();
    setupConnections();
    buildMibTree();
}

SNMPWidget::~SNMPWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void SNMPWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Connection configuration ──
    auto* configGroup = new QGroupBox("SNMP 配置");
    auto* configLayout = new QHBoxLayout(configGroup);
    configLayout->setSpacing(12);

    auto styleLineEdit = [](QLineEdit* edit, const QString& placeholder, int minWidth = 120) {
        edit->setPlaceholderText(placeholder);
        edit->setMinimumWidth(minWidth);
        edit->setStyleSheet(
            "QLineEdit {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        );
    };

    auto styleCombo = [](QComboBox* combo, int minWidth = 90) {
        combo->setMinimumWidth(minWidth);
        combo->setStyleSheet(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        );
    };

    // Target host
    auto* hostLayout = new QVBoxLayout();
    auto* hostLabel = new QLabel("目标主机:");
    hostLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_targetEdit = new QLineEdit();
    styleLineEdit(m_targetEdit, "例如: 192.168.1.1 或 switch.example.com", 180);
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(m_targetEdit);
    configLayout->addLayout(hostLayout, 2);

    // Port
    auto* portLayout = new QVBoxLayout();
    auto* portLabel = new QLabel("端口:");
    portLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(161);
    m_portSpin->setFixedWidth(90);
    m_portSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    portLayout->addWidget(portLabel);
    portLayout->addWidget(m_portSpin);
    configLayout->addLayout(portLayout);

    // Community
    auto* communityLayout = new QVBoxLayout();
    auto* communityLabel = new QLabel("Community:");
    communityLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_communityEdit = new QLineEdit();
    m_communityEdit->setText("public");
    styleLineEdit(m_communityEdit, "public", 130);
    communityLayout->addWidget(communityLabel);
    communityLayout->addWidget(m_communityEdit);
    configLayout->addLayout(communityLayout);

    // Version
    auto* versionLayout = new QVBoxLayout();
    auto* versionLabel = new QLabel("版本:");
    versionLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_versionCombo = new QComboBox();
    m_versionCombo->addItem("v1", "1");
    m_versionCombo->addItem("v2c", "2c");
    styleCombo(m_versionCombo, 80);
    versionLayout->addWidget(versionLabel);
    versionLayout->addWidget(m_versionCombo);
    configLayout->addLayout(versionLayout);

    configLayout->addStretch();
    mainLayout->addWidget(configGroup);

    // ── OID & Operation row ──
    auto* oidGroup = new QGroupBox("查询操作");
    auto* oidLayout = new QHBoxLayout(oidGroup);
    oidLayout->setSpacing(12);

    auto* oidLabel = new QLabel("OID:");
    oidLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_oidEdit = new QLineEdit();
    m_oidEdit->setPlaceholderText("例如: 1.3.6.1.2.1.1.1.0 或 sysDescr.0");
    m_oidEdit->setMinimumWidth(280);
    m_oidEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    oidLayout->addWidget(oidLabel);
    oidLayout->addWidget(m_oidEdit, 1);

    m_operationCombo = new QComboBox();
    m_operationCombo->addItem("GET", "get");
    m_operationCombo->addItem("GETNEXT", "getnext");
    m_operationCombo->addItem("WALK", "walk");
    styleCombo(m_operationCombo, 110);
    oidLayout->addWidget(m_operationCombo);

    // Buttons
    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_sendBtn->setFixedHeight(36);

    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #F78989; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_stopBtn->setFixedHeight(36);
    m_stopBtn->setEnabled(false);

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_exportBtn->setFixedHeight(36);

    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #909399; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #B4B4B9; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_clearBtn->setFixedHeight(36);

    oidLayout->addWidget(m_sendBtn);
    oidLayout->addWidget(m_stopBtn);
    oidLayout->addWidget(m_exportBtn);
    oidLayout->addWidget(m_clearBtn);

    mainLayout->addWidget(oidGroup);

    // ── MIB Shortcuts ──
    auto* shortcutGroup = new QGroupBox("MIB 快捷方式");
    auto* shortcutLayout = new QHBoxLayout(shortcutGroup);
    shortcutLayout->setSpacing(8);

    struct ShortcutInfo {
        QString label;
        QString oid;
    };
    const QList<ShortcutInfo> shortcuts = {
        {"系统描述",    "1.3.6.1.2.1.1.1.0"},
        {"系统名称",    "1.3.6.1.2.1.1.5.0"},
        {"运行时间",    "1.3.6.1.2.1.1.3.0"},
        {"接口列表",    "1.3.6.1.2.1.2.2.1.2"},
        {"IP 地址",     "1.3.6.1.2.1.4.20.1.1"},
        {"路由表",      "1.3.6.1.2.1.4.21.1.1"},
        {"ARP 表",      "1.3.6.1.2.1.4.22.1.2"},
        {"CPU 使用率",   "1.3.6.1.4.1.2021.11.11.0"},
        {"内存总量",     "1.3.6.1.4.1.2021.4.5.0"},
        {"磁盘总量",     "1.3.6.1.4.1.2021.9.1.6.1"},
    };

    for (const auto& sc : shortcuts) {
        auto* btn = new QPushButton(sc.label);
        btn->setProperty("snmp_oid", sc.oid);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #25262B; color: #409EFF;"
            "  border: 1px solid #3C3F41; padding: 5px 12px;"
            "  border-radius: 3px; font-size: 12px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3C3F41; color: #66B1FF;"
            "}"
        );
        shortcutLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, &SNMPWidget::onMibShortcutClicked);
    }

    shortcutLayout->addStretch();
    mainLayout->addWidget(shortcutGroup);

    // ── Progress bar ──
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background: #25262B; border: 1px solid #3C3F41;"
        "  border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "  background: #409EFF; border-radius: 3px;"
        "}"
    );
    mainLayout->addWidget(m_progressBar);

    // ── Main content: MIB tree on left, results on right ──
    auto* splitter = new QSplitter(Qt::Horizontal);

    // MIB Tree
    auto* mibGroup = new QGroupBox("MIB 树浏览器");
    auto* mibLayout = new QVBoxLayout(mibGroup);
    mibLayout->setContentsMargins(4, 4, 4, 4);

    m_mibTree = new QTreeWidget();
    m_mibTree->setHeaderLabels({"名称", "OID"});
    m_mibTree->header()->setStretchLastSection(true);
    m_mibTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_mibTree->setColumnWidth(0, 180);
    m_mibTree->setAnimated(true);
    m_mibTree->setAlternatingRowColors(true);
    m_mibTree->setStyleSheet(
        "QTreeWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 12px;"
        "}"
        "QTreeWidget::item { padding: 3px 4px; }"
        "QTreeWidget::item:hover { background-color: #2C2D30; }"
        "QTreeWidget::item:selected { background-color: #3C3F41; }"
        "QHeaderView::section {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  border: none; border-bottom: 2px solid #3C3F41;"
        "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
        "}"
    );
    mibLayout->addWidget(m_mibTree);
    splitter->addWidget(mibGroup);

    // Results area
    auto* resultsGroup = new QGroupBox("查询结果");
    auto* resultsLayout = new QVBoxLayout(resultsGroup);
    resultsLayout->setContentsMargins(4, 4, 4, 4);

    m_resultsArea = new QPlainTextEdit();
    m_resultsArea->setReadOnly(true);
    m_resultsArea->setPlaceholderText("SNMP 查询结果将显示在这里...");
    m_resultsArea->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 13px;"
        "  font-family: 'Menlo', 'Consolas', 'Courier New', monospace;"
        "  padding: 8px;"
        "}"
    );
    resultsLayout->addWidget(m_resultsArea);
    splitter->addWidget(resultsGroup);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({300, 600});
    mainLayout->addWidget(splitter, 1);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void SNMPWidget::setupConnections()
{
    connect(m_sendBtn, &QPushButton::clicked, this, &SNMPWidget::onSend);
    connect(m_stopBtn, &QPushButton::clicked, this, &SNMPWidget::onStop);
    connect(m_exportBtn, &QPushButton::clicked, this, &SNMPWidget::onExportCSV);
    connect(m_clearBtn, &QPushButton::clicked, this, &SNMPWidget::onClear);

    connect(m_mibTree, &QTreeWidget::itemClicked, this, &SNMPWidget::onMibTreeItemClicked);

    connect(m_oidEdit, &QLineEdit::returnPressed, this, &SNMPWidget::onSend);
}

// ─── MIB Tree ────────────────────────────────────────────────────────────────

void SNMPWidget::addMibNode(QTreeWidgetItem* parent, const QString& name, const QString& oid)
{
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, name);
    item->setText(1, oid);
    item->setToolTip(1, oid);
}

void SNMPWidget::buildMibTree()
{
    m_mibTree->clear();

    // ── Root ──
    auto* iso = new QTreeWidgetItem(m_mibTree);
    iso->setText(0, "iso (1)");
    iso->setText(1, "1");

    auto* org = new QTreeWidgetItem(iso);
    org->setText(0, "org (3)");
    org->setText(1, "1.3");

    auto* dod = new QTreeWidgetItem(org);
    dod->setText(0, "dod (6)");
    dod->setText(1, "1.3.6");

    auto* internet = new QTreeWidgetItem(dod);
    internet->setText(0, "internet (1)");
    internet->setText(1, "1.3.6.1");

    // ── mgmt (2) ──
    auto* mgmt = new QTreeWidgetItem(internet);
    mgmt->setText(0, "mgmt (2)");
    mgmt->setText(1, "1.3.6.1.2");

    auto* mib2 = new QTreeWidgetItem(mgmt);
    mib2->setText(0, "mib-2 (1)");
    mib2->setText(1, "1.3.6.1.2.1");

    // ── system (1) ──
    auto* system = new QTreeWidgetItem(mib2);
    system->setText(0, "system (1)");
    system->setText(1, "1.3.6.1.2.1.1");
    addMibNode(system, "sysDescr (1)", "1.3.6.1.2.1.1.1.0");
    addMibNode(system, "sysObjectID (2)", "1.3.6.1.2.1.1.2.0");
    addMibNode(system, "sysUpTime (3)", "1.3.6.1.2.1.1.3.0");
    addMibNode(system, "sysContact (4)", "1.3.6.1.2.1.1.4.0");
    addMibNode(system, "sysName (5)", "1.3.6.1.2.1.1.5.0");
    addMibNode(system, "sysLocation (6)", "1.3.6.1.2.1.1.6.0");

    // ── interfaces (2) ──
    auto* interfaces = new QTreeWidgetItem(mib2);
    interfaces->setText(0, "interfaces (2)");
    interfaces->setText(1, "1.3.6.1.2.1.2");

    auto* ifTable = new QTreeWidgetItem(interfaces);
    ifTable->setText(0, "ifTable (2)");
    ifTable->setText(1, "1.3.6.1.2.1.2.2");
    addMibNode(ifTable, "ifIndex (1)", "1.3.6.1.2.1.2.2.1.1");
    addMibNode(ifTable, "ifDescr (2)", "1.3.6.1.2.1.2.2.1.2");
    addMibNode(ifTable, "ifType (3)", "1.3.6.1.2.1.2.2.1.3");
    addMibNode(ifTable, "ifMtu (4)", "1.3.6.1.2.1.2.2.1.4");
    addMibNode(ifTable, "ifSpeed (5)", "1.3.6.1.2.1.2.2.1.5");
    addMibNode(ifTable, "ifPhysAddress (6)", "1.3.6.1.2.1.2.2.1.6");
    addMibNode(ifTable, "ifAdminStatus (7)", "1.3.6.1.2.1.2.2.1.7");
    addMibNode(ifTable, "ifOperStatus (8)", "1.3.6.1.2.1.2.2.1.8");
    addMibNode(ifTable, "ifInOctets (10)", "1.3.6.1.2.1.2.2.1.10");
    addMibNode(ifTable, "ifOutOctets (16)", "1.3.6.1.2.1.2.2.1.16");

    auto* ifNumber = new QTreeWidgetItem(interfaces);
    ifNumber->setText(0, "ifNumber (1)");
    ifNumber->setText(1, "1.3.6.1.2.1.2.1.0");

    // ── ip (4) ──
    auto* ip = new QTreeWidgetItem(mib2);
    ip->setText(0, "ip (4)");
    ip->setText(1, "1.3.6.1.2.1.4");

    addMibNode(ip, "ipForwarding (1)", "1.3.6.1.2.1.4.1.0");
    addMibNode(ip, "ipDefaultTTL (2)", "1.3.6.1.2.1.4.2.0");

    auto* ipAddrTable = new QTreeWidgetItem(ip);
    ipAddrTable->setText(0, "ipAddrTable (20)");
    ipAddrTable->setText(1, "1.3.6.1.2.1.4.20");
    addMibNode(ipAddrTable, "ipAdEntAddr (1)", "1.3.6.1.2.1.4.20.1.1");
    addMibNode(ipAddrTable, "ipAdEntIfIndex (2)", "1.3.6.1.2.1.4.20.1.2");
    addMibNode(ipAddrTable, "ipAdEntNetMask (3)", "1.3.6.1.2.1.4.20.1.3");

    auto* ipRouteTable = new QTreeWidgetItem(ip);
    ipRouteTable->setText(0, "ipRouteTable (21)");
    ipRouteTable->setText(1, "1.3.6.1.2.1.4.21");
    addMibNode(ipRouteTable, "ipRouteDest (1)", "1.3.6.1.2.1.4.21.1.1");
    addMibNode(ipRouteTable, "ipRouteNextHop (7)", "1.3.6.1.2.1.4.21.1.7");
    addMibNode(ipRouteTable, "ipRouteMask (11)", "1.3.6.1.2.1.4.21.1.11");

    auto* ipNetToMedia = new QTreeWidgetItem(ip);
    ipNetToMedia->setText(0, "ipNetToMediaTable (22)");
    ipNetToMedia->setText(1, "1.3.6.1.2.1.4.22");
    addMibNode(ipNetToMedia, "ipNetToMediaPhysAddress (2)", "1.3.6.1.2.1.4.22.1.2");

    // ── icmp (5) ──
    auto* icmp = new QTreeWidgetItem(mib2);
    icmp->setText(0, "icmp (5)");
    icmp->setText(1, "1.3.6.1.2.1.5");
    addMibNode(icmp, "icmpInMsgs (1)", "1.3.6.1.2.1.5.1.0");
    addMibNode(icmp, "icmpOutMsgs (14)", "1.3.6.1.2.1.5.14.0");

    // ── tcp (6) ──
    auto* tcp = new QTreeWidgetItem(mib2);
    tcp->setText(0, "tcp (6)");
    tcp->setText(1, "1.3.6.1.2.1.6");
    addMibNode(tcp, "tcpConnTable (13)", "1.3.6.1.2.1.6.13.1.1");

    // ── udp (7) ──
    auto* udp = new QTreeWidgetItem(mib2);
    udp->setText(0, "udp (7)");
    udp->setText(1, "1.3.6.1.2.1.7");
    addMibNode(udp, "udpTable (5)", "1.3.6.1.2.1.7.5.1.1");

    // ── snmp (11) ──
    auto* snmpNode = new QTreeWidgetItem(mib2);
    snmpNode->setText(0, "snmp (11)");
    snmpNode->setText(1, "1.3.6.1.2.1.11");
    addMibNode(snmpNode, "snmpInPkts (1)", "1.3.6.1.2.1.11.1.0");
    addMibNode(snmpNode, "snmpOutPkts (2)", "1.3.6.1.2.1.11.2.0");

    // ── private (4) ──
    auto* privateNode = new QTreeWidgetItem(internet);
    privateNode->setText(0, "private (4)");
    privateNode->setText(1, "1.3.6.1.4");

    auto* enterprises = new QTreeWidgetItem(privateNode);
    enterprises->setText(0, "enterprises (1)");
    enterprises->setText(1, "1.3.6.1.4.1");

    // UCD-SNMP-MIB (2021)
    auto* ucd = new QTreeWidgetItem(enterprises);
    ucd->setText(0, "ucdavis (2021)");
    ucd->setText(1, "1.3.6.1.4.1.2021");

    auto* ucdSystem = new QTreeWidgetItem(ucd);
    ucdSystem->setText(0, "systemStats (11)");
    ucdSystem->setText(1, "1.3.6.1.4.1.2021.11");
    addMibNode(ucdSystem, "ssCpuUser (9)", "1.3.6.1.4.1.2021.11.9.0");
    addMibNode(ucdSystem, "ssCpuSystem (10)", "1.3.6.1.4.1.2021.11.10.0");
    addMibNode(ucdSystem, "ssCpuIdle (11)", "1.3.6.1.4.1.2021.11.11.0");

    auto* ucdMem = new QTreeWidgetItem(ucd);
    ucdMem->setText(0, "memory (4)");
    ucdMem->setText(1, "1.3.6.1.4.1.2021.4");
    addMibNode(ucdMem, "memTotalReal (5)", "1.3.6.1.4.1.2021.4.5.0");
    addMibNode(ucdMem, "memAvailReal (6)", "1.3.6.1.4.1.2021.4.6.0");

    auto* ucdDisk = new QTreeWidgetItem(ucd);
    ucdDisk->setText(0, "dskTable (9)");
    ucdDisk->setText(1, "1.3.6.1.4.1.2021.9");
    addMibNode(ucdDisk, "dskTotal (6)", "1.3.6.1.4.1.2021.9.1.6.1");
    addMibNode(ucdDisk, "dskAvail (7)", "1.3.6.1.4.1.2021.9.1.7.1");

    m_mibTree->expandToDepth(1);
}

// ─── Tree Item Click ─────────────────────────────────────────────────────────

void SNMPWidget::onMibTreeItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    QString oid = item->text(1);
    if (!oid.isEmpty()) {
        m_oidEdit->setText(oid);
    }
}

void SNMPWidget::onMibShortcutClicked()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString oid = btn->property("snmp_oid").toString();
    if (!oid.isEmpty()) {
        m_oidEdit->setText(oid);
    }
}

// ─── Command Building ────────────────────────────────────────────────────────

QString SNMPWidget::buildCommand() const
{
    QString op = m_operationCombo->currentData().toString();
    if (op == "get")      return "snmpget";
    if (op == "getnext")  return "snmpgetnext";
    if (op == "walk")     return "snmpwalk";
    return "snmpget";
}

QStringList SNMPWidget::buildArgs() const
{
    QStringList args;
    QString version = m_versionCombo->currentData().toString();
    args << "-v" << version;
    args << "-c" << m_communityEdit->text();
    args << QString("%1:%2").arg(m_targetEdit->text().trimmed()).arg(m_portSpin->value());
    args << m_oidEdit->text().trimmed();
    return args;
}

// ─── Actions ─────────────────────────────────────────────────────────────────

void SNMPWidget::onSend()
{
    QString target = m_targetEdit->text().trimmed();
    if (target.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入目标主机地址。");
        return;
    }

    QString oid = m_oidEdit->text().trimmed();
    if (oid.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入 OID。");
        return;
    }

    // Reset state
    m_results.clear();
    m_resultsArea->clear();
    m_running = true;
    m_currentCommand = buildCommand();
    m_currentOid = oid;

    m_sendBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_targetEdit->setEnabled(false);
    m_portSpin->setEnabled(false);
    m_communityEdit->setEnabled(false);
    m_versionCombo->setEnabled(false);
    m_oidEdit->setEnabled(false);
    m_operationCombo->setEnabled(false);

    // Show progress indicator for WALK
    if (m_operationCombo->currentData().toString() == "walk") {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);
    }

    QStringList args = buildArgs();
    Logger::instance().info("SNMP", QString("执行 %1: %2 %3")
                                      .arg(m_currentCommand)
                                      .arg(target)
                                      .arg(oid));

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SNMPWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPWidget::onProcessFinished);

    m_process->start(m_currentCommand, args);
}

void SNMPWidget::onStop()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    m_running = false;
    Logger::instance().info("SNMP", "SNMP 查询已停止");
}

void SNMPWidget::onClear()
{
    m_resultsArea->clear();
    m_results.clear();
}

// ─── Process Callbacks ───────────────────────────────────────────────────────

void SNMPWidget::onProcessReadyRead()
{
    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromUtf8(data);

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            parseSnmpLine(trimmed);
        }
    }
}

void SNMPWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_running = false;
    m_progressBar->setVisible(false);

    m_sendBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_targetEdit->setEnabled(true);
    m_portSpin->setEnabled(true);
    m_communityEdit->setEnabled(true);
    m_versionCombo->setEnabled(true);
    m_oidEdit->setEnabled(true);
    m_operationCombo->setEnabled(true);

    // Read any remaining output
    if (m_process) {
        QByteArray remaining = m_process->readAllStandardOutput();
        if (!remaining.isEmpty()) {
            QString output = QString::fromUtf8(remaining);
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (!trimmed.isEmpty()) {
                    parseSnmpLine(trimmed);
                }
            }
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (exitCode != 0) {
        QString errorMsg = QString("SNMP 查询失败 (退出码: %1)").arg(exitCode);
        m_resultsArea->appendPlainText(errorMsg);
        Logger::instance().error("SNMP", errorMsg);
        return;
    }

    if (m_results.isEmpty()) {
        m_resultsArea->appendPlainText("未返回任何结果。");
    }

    Logger::instance().info("SNMP",
        QString("%1 完成: %2 条结果").arg(m_currentCommand).arg(m_results.size()));
}

// ─── Parsing ─────────────────────────────────────────────────────────────────

void SNMPWidget::parseSnmpLine(const QString& line)
{
    // SNMP output format: ISO.3.6.1.2.1.1.1.0 = STRING: "Linux server 5.15.0"
    // or: SNMPv2-MIB::sysDescr.0 = STRING: Linux server 5.15.0
    // or: SNMPv2-MIB::sysUpTime.0 = Timeticks: (123456) 0:20:34.56
    // or: iso.3.6.1.2.1.1.1.0 = Counter32: 12345

    // Error messages
    if (line.contains("No Such Object") || line.contains("Unknown Object") ||
        line.contains("No Such Instance") || line.contains("Timeout") ||
        line.contains("No Response")) {
        m_resultsArea->appendPlainText(line);
        Logger::instance().warning("SNMP", line);
        return;
    }

    if (line.contains("Error") && !line.contains("=")) {
        m_resultsArea->appendPlainText(line);
        Logger::instance().error("SNMP", line);
        return;
    }

    // Parse OID = Type: Value
    static QRegularExpression re(
        R"(^(.+?)\s*=\s*(\w+)\s*:\s*(.+)$)"
    );

    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        QString oid = match.captured(1).trimmed();
        QString type = match.captured(2).trimmed();
        QString value = match.captured(3).trimmed();

        // Remove surrounding quotes from STRING values
        if (type == "STRING" && value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }

        appendResult(oid, value, type);
    } else {
        m_resultsArea->appendPlainText(line);
    }
}

void SNMPWidget::appendResult(const QString& oid, const QString& value, const QString& type)
{
    QString displayLine = QString("%1 = %2").arg(oid, value);
    if (!type.isEmpty()) {
        displayLine += QString(" [%1]").arg(type);
    }

    m_resultsArea->appendPlainText(displayLine);
    m_results.append({oid, value, type});
}

// ─── Export ──────────────────────────────────────────────────────────────────

void SNMPWidget::onExportCSV()
{
    if (m_results.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 SNMP 结果",
        QString("snmp_result_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("SNMP", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8
    stream << "OID,值,类型\n";

    for (const auto& r : m_results) {
        QString escapedValue = r.value;
        QString escapedOid = r.oid;
        if (escapedValue.contains(',') || escapedValue.contains('"')) {
            escapedValue.replace("\"", "\"\"");
            escapedValue = "\"" + escapedValue + "\"";
        }
        if (escapedOid.contains(',') || escapedOid.contains('"')) {
            escapedOid.replace("\"", "\"\"");
            escapedOid = "\"" + escapedOid + "\"";
        }
        stream << escapedOid << ","
               << escapedValue << ","
               << r.type << "\n";
    }

    file.close();
    Logger::instance().info("SNMP", "Results exported to: " + filePath);
    QMessageBox::information(this, "导出成功",
                              QString("已导出 %1 条记录到:\n%2").arg(m_results.size()).arg(filePath));
}