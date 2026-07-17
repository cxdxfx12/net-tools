#include "mib/MibWidget.h"
#include "log/Logger.h"
#include "database/DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QListWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QLabel>

// ═══════════════════════════════════════════════════════════════════════════════
// MibWidget
// ═══════════════════════════════════════════════════════════════════════════════

MibWidget::MibWidget(QWidget* parent)
    : QWidget(parent)
    , m_snmpProcess(nullptr)
{
    setupUI();
    setupConnections();
    setupBuiltinOidDatabase();
    buildBuiltinOidTree();
}

MibWidget::~MibWidget()
{
    if (m_snmpProcess) {
        m_snmpProcess->kill();
        m_snmpProcess->waitForFinished(3000);
        delete m_snmpProcess;
        m_snmpProcess = nullptr;
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void MibWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Toolbar row ──
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    m_importMibBtn = new QPushButton(QStringLiteral("导入 MIB"));
    m_importMibBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
    ));
    m_importMibBtn->setFixedHeight(32);

    m_deleteMibBtn = new QPushButton(QStringLiteral("删除 MIB"));
    m_deleteMibBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #F78989; }"
    ));
    m_deleteMibBtn->setFixedHeight(32);

    m_searchBtn = new QPushButton(QStringLiteral("搜索"));
    m_searchBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
    ));
    m_searchBtn->setFixedHeight(32);

    m_favoriteBtn = new QPushButton(QStringLiteral("收藏夹"));
    m_favoriteBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #E6A23C; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #EBB563; }"
    ));
    m_favoriteBtn->setFixedHeight(32);

    m_exportBtn = new QPushButton(QStringLiteral("导出 CSV"));
    m_exportBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 16px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
    ));
    m_exportBtn->setFixedHeight(32);

    toolbarLayout->addWidget(m_importMibBtn);
    toolbarLayout->addWidget(m_deleteMibBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_searchBtn);
    toolbarLayout->addWidget(m_favoriteBtn);
    toolbarLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(toolbarLayout);

    // ── Search row ──
    auto* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(8);
    auto* searchLabel = new QLabel(QStringLiteral("OID 搜索:"));
    searchLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    m_oidSearchEdit = new QLineEdit();
    m_oidSearchEdit->setPlaceholderText(QStringLiteral("输入 OID 或名称进行搜索，例如: 1.3.6.1.2.1.1 或 sysDescr"));
    m_oidSearchEdit->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 6px 10px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    ));
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_oidSearchEdit, 1);
    mainLayout->addLayout(searchLayout);

    // ── Main content: three panels ──
    auto* splitter = new QSplitter(Qt::Horizontal);

    // ── Left panel: MIB file list ──
    auto* mibFileGroup = new QGroupBox(QStringLiteral("MIB 文件"));
    auto* mibFileLayout = new QVBoxLayout(mibFileGroup);
    mibFileLayout->setContentsMargins(4, 4, 4, 4);

    m_mibFileList = new QListWidget();
    m_mibFileList->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 12px;"
        "}"
        "QListWidget::item { padding: 4px 6px; }"
        "QListWidget::item:hover { background-color: #2C2D30; }"
        "QListWidget::item:selected { background-color: #3C3F41; }"
    ));
    mibFileLayout->addWidget(m_mibFileList);
    splitter->addWidget(mibFileGroup);

    // ── Center panel: OID tree ──
    auto* oidTreeGroup = new QGroupBox(QStringLiteral("OID 树"));
    auto* oidTreeLayout = new QVBoxLayout(oidTreeGroup);
    oidTreeLayout->setContentsMargins(4, 4, 4, 4);

    m_oidTree = new QTreeWidget();
    m_oidTree->setHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("OID"),
        QStringLiteral("类型"),
        QStringLiteral("访问")
    });
    m_oidTree->header()->setStretchLastSection(true);
    m_oidTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_oidTree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_oidTree->setColumnWidth(0, 200);
    m_oidTree->setColumnWidth(1, 220);
    m_oidTree->setColumnWidth(2, 80);
    m_oidTree->setAnimated(true);
    m_oidTree->setAlternatingRowColors(true);
    m_oidTree->setStyleSheet(QStringLiteral(
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
    ));
    oidTreeLayout->addWidget(m_oidTree);
    splitter->addWidget(oidTreeGroup);

    // ── Right panel: detail + SNMP test ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    // Detail area
    auto* detailGroup = new QGroupBox(QStringLiteral("OID 详情"));
    auto* detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->setContentsMargins(4, 4, 4, 4);

    m_detailArea = new QPlainTextEdit();
    m_detailArea->setReadOnly(true);
    m_detailArea->setPlaceholderText(QStringLiteral("点击 OID 树中的节点查看详情..."));
    m_detailArea->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 13px;"
        "  font-family: 'Menlo', 'Consolas', 'Courier New', monospace;"
        "  padding: 8px;"
        "}"
    ));
    detailLayout->addWidget(m_detailArea);
    rightLayout->addWidget(detailGroup, 1);

    // SNMP test area
    auto* snmpGroup = new QGroupBox(QStringLiteral("SNMP 测试"));
    auto* snmpLayout = new QVBoxLayout(snmpGroup);
    snmpLayout->setContentsMargins(4, 4, 4, 4);
    snmpLayout->setSpacing(6);

    auto* snmpFormLayout = new QFormLayout();
    snmpFormLayout->setSpacing(6);

    m_deviceIpEdit = new QLineEdit();
    m_deviceIpEdit->setPlaceholderText(QStringLiteral("例如: 192.168.1.1"));
    m_deviceIpEdit->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    ));

    m_communityEdit = new QLineEdit();
    m_communityEdit->setText(QStringLiteral("public"));
    m_communityEdit->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    ));

    auto* ipLabel = new QLabel(QStringLiteral("设备 IP:"));
    ipLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    auto* communityLabel = new QLabel(QStringLiteral("Community:"));
    communityLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");

    snmpFormLayout->addRow(ipLabel, m_deviceIpEdit);
    snmpFormLayout->addRow(communityLabel, m_communityEdit);
    snmpLayout->addLayout(snmpFormLayout);

    auto* snmpBtnLayout = new QHBoxLayout();
    snmpBtnLayout->setSpacing(8);

    m_getBtn = new QPushButton(QStringLiteral("SNMP Get"));
    m_getBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 6px 14px; border-radius: 4px;"
        "  font-size: 12px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    ));
    m_getBtn->setFixedHeight(30);

    m_walkBtn = new QPushButton(QStringLiteral("SNMP Walk"));
    m_walkBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #E6A23C; color: white;"
        "  border: none; padding: 6px 14px; border-radius: 4px;"
        "  font-size: 12px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #EBB563; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    ));
    m_walkBtn->setFixedHeight(30);

    snmpBtnLayout->addWidget(m_getBtn);
    snmpBtnLayout->addWidget(m_walkBtn);
    snmpBtnLayout->addStretch();
    snmpLayout->addLayout(snmpBtnLayout);

    m_snmpResultArea = new QPlainTextEdit();
    m_snmpResultArea->setReadOnly(true);
    m_snmpResultArea->setPlaceholderText(QStringLiteral("SNMP 测试结果将显示在这里..."));
    m_snmpResultArea->setMaximumHeight(150);
    m_snmpResultArea->setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; font-size: 12px;"
        "  font-family: 'Menlo', 'Consolas', 'Courier New', monospace;"
        "  padding: 6px;"
        "}"
    ));
    snmpLayout->addWidget(m_snmpResultArea);

    rightLayout->addWidget(snmpGroup);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 2);
    splitter->setSizes({200, 400, 350});
    mainLayout->addWidget(splitter, 1);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void MibWidget::setupConnections()
{
    connect(m_importMibBtn, &QPushButton::clicked, this, &MibWidget::onImportMib);
    connect(m_deleteMibBtn, &QPushButton::clicked, this, &MibWidget::onDeleteMib);
    connect(m_searchBtn, &QPushButton::clicked, this, &MibWidget::onSearchOid);
    connect(m_favoriteBtn, &QPushButton::clicked, this, &MibWidget::onAddFavorite);
    connect(m_oidTree, &QTreeWidget::itemClicked, this, &MibWidget::onOidTreeItemClicked);
    connect(m_getBtn, &QPushButton::clicked, this, &MibWidget::onSnmpGet);
    connect(m_walkBtn, &QPushButton::clicked, this, &MibWidget::onSnmpWalk);
    connect(m_exportBtn, &QPushButton::clicked, this, &MibWidget::onExport);
    connect(m_oidSearchEdit, &QLineEdit::returnPressed, this, &MibWidget::onSearchOid);
}

// ─── Built-in OID Database ──────────────────────────────────────────────────

void MibWidget::setupBuiltinOidDatabase()
{
    // ── system group (1.3.6.1.2.1.1) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.1.0"),
        {QStringLiteral("sysDescr"), QStringLiteral("1.3.6.1.2.1.1.1.0"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
         QStringLiteral("A textual description of the entity.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.2.0"),
        {QStringLiteral("sysObjectID"), QStringLiteral("1.3.6.1.2.1.1.2.0"),
         QStringLiteral("OBJECT IDENTIFIER"), QStringLiteral("read-only"),
         QStringLiteral("The vendor's authoritative identification of the network management subsystem.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.3.0"),
        {QStringLiteral("sysUpTime"), QStringLiteral("1.3.6.1.2.1.1.3.0"),
         QStringLiteral("TimeTicks"), QStringLiteral("read-only"),
         QStringLiteral("The time since the network management portion of the system was last re-initialized.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.4.0"),
        {QStringLiteral("sysContact"), QStringLiteral("1.3.6.1.2.1.1.4.0"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
         QStringLiteral("The textual identification of the contact person for this managed node.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.5.0"),
        {QStringLiteral("sysName"), QStringLiteral("1.3.6.1.2.1.1.5.0"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
         QStringLiteral("An administratively-assigned name for this managed node.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.6.0"),
        {QStringLiteral("sysLocation"), QStringLiteral("1.3.6.1.2.1.1.6.0"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
         QStringLiteral("The physical location of this node.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.1.7.0"),
        {QStringLiteral("sysServices"), QStringLiteral("1.3.6.1.2.1.1.7.0"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("A value which indicates the set of services that this entity may potentially offer.")});

    // ── interfaces group (1.3.6.1.2.1.2) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.1.0"),
        {QStringLiteral("ifNumber"), QStringLiteral("1.3.6.1.2.1.2.1.0"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("The number of network interfaces present on this system.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.1"),
        {QStringLiteral("ifIndex"), QStringLiteral("1.3.6.1.2.1.2.2.1.1"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("A unique value for each interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.2"),
        {QStringLiteral("ifDescr"), QStringLiteral("1.3.6.1.2.1.2.2.1.2"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
         QStringLiteral("A textual string containing information about the interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.3"),
        {QStringLiteral("ifType"), QStringLiteral("1.3.6.1.2.1.2.2.1.3"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("The type of interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.4"),
        {QStringLiteral("ifMtu"), QStringLiteral("1.3.6.1.2.1.2.2.1.4"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("The size of the largest datagram which can be sent/received on the interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.5"),
        {QStringLiteral("ifSpeed"), QStringLiteral("1.3.6.1.2.1.2.2.1.5"),
         QStringLiteral("Gauge32"), QStringLiteral("read-only"),
         QStringLiteral("An estimate of the interface's current bandwidth in bits per second.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.6"),
        {QStringLiteral("ifPhysAddress"), QStringLiteral("1.3.6.1.2.1.2.2.1.6"),
         QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
         QStringLiteral("The interface's address at the protocol layer immediately below the network layer.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.7"),
        {QStringLiteral("ifAdminStatus"), QStringLiteral("1.3.6.1.2.1.2.2.1.7"),
         QStringLiteral("INTEGER"), QStringLiteral("read-write"),
         QStringLiteral("The desired state of the interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.8"),
        {QStringLiteral("ifOperStatus"), QStringLiteral("1.3.6.1.2.1.2.2.1.8"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("The current operational state of the interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.10"),
        {QStringLiteral("ifInOctets"), QStringLiteral("1.3.6.1.2.1.2.2.1.10"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of octets received on the interface.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.2.2.1.16"),
        {QStringLiteral("ifOutOctets"), QStringLiteral("1.3.6.1.2.1.2.2.1.16"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of octets transmitted out of the interface.")});

    // ── ip group (1.3.6.1.2.1.4) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.1.0"),
        {QStringLiteral("ipForwarding"), QStringLiteral("1.3.6.1.2.1.4.1.0"),
         QStringLiteral("INTEGER"), QStringLiteral("read-write"),
         QStringLiteral("The indication of whether this entity is acting as an IP gateway.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.2.0"),
        {QStringLiteral("ipDefaultTTL"), QStringLiteral("1.3.6.1.2.1.4.2.0"),
         QStringLiteral("INTEGER"), QStringLiteral("read-write"),
         QStringLiteral("The default value inserted into the Time-To-Live field of the IP header.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.3.0"),
        {QStringLiteral("ipInReceives"), QStringLiteral("1.3.6.1.2.1.4.3.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of input datagrams received from interfaces.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.4.0"),
        {QStringLiteral("ipInHdrErrors"), QStringLiteral("1.3.6.1.2.1.4.4.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of input datagrams discarded due to errors in their IP headers.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.5.0"),
        {QStringLiteral("ipInAddrErrors"), QStringLiteral("1.3.6.1.2.1.4.5.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of input datagrams discarded because the destination IP address was invalid.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.6.0"),
        {QStringLiteral("ipForwDatagrams"), QStringLiteral("1.3.6.1.2.1.4.6.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of input datagrams for which this entity was not their final IP destination.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.4.9.0"),
        {QStringLiteral("ipOutRequests"), QStringLiteral("1.3.6.1.2.1.4.9.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of IP datagrams which local IP user-protocols supplied to IP.")});

    // ── icmp group (1.3.6.1.2.1.5) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.5.1.0"),
        {QStringLiteral("icmpInMsgs"), QStringLiteral("1.3.6.1.2.1.5.1.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of ICMP messages which the entity received.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.5.2.0"),
        {QStringLiteral("icmpInErrors"), QStringLiteral("1.3.6.1.2.1.5.2.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of ICMP messages which the entity received but had ICMP-specific errors.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.5.14.0"),
        {QStringLiteral("icmpOutMsgs"), QStringLiteral("1.3.6.1.2.1.5.14.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of ICMP messages which this entity attempted to send.")});

    // ── tcp group (1.3.6.1.2.1.6) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.6.1.0"),
        {QStringLiteral("tcpRtoAlgorithm"), QStringLiteral("1.3.6.1.2.1.6.1.0"),
         QStringLiteral("INTEGER"), QStringLiteral("read-only"),
         QStringLiteral("The algorithm used to determine the timeout value used for retransmitting unacknowledged octets.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.6.5.0"),
        {QStringLiteral("tcpActiveOpens"), QStringLiteral("1.3.6.1.2.1.6.5.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of times TCP connections have made a direct transition to the SYN-SENT state.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.6.6.0"),
        {QStringLiteral("tcpPassiveOpens"), QStringLiteral("1.3.6.1.2.1.6.6.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The number of times TCP connections have made a direct transition to the SYN-RCVD state.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.6.9.0"),
        {QStringLiteral("tcpInSegs"), QStringLiteral("1.3.6.1.2.1.6.9.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of segments received, including those received in error.")});

    // ── udp group (1.3.6.1.2.1.7) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.7.1.0"),
        {QStringLiteral("udpInDatagrams"), QStringLiteral("1.3.6.1.2.1.7.1.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of UDP datagrams delivered to UDP users.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.7.2.0"),
        {QStringLiteral("udpNoPorts"), QStringLiteral("1.3.6.1.2.1.7.2.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of received UDP datagrams for which there was no application at the destination port.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.7.4.0"),
        {QStringLiteral("udpOutDatagrams"), QStringLiteral("1.3.6.1.2.1.7.4.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of UDP datagrams sent from this entity.")});

    // ── snmp group (1.3.6.1.2.1.11) ──
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.11.1.0"),
        {QStringLiteral("snmpInPkts"), QStringLiteral("1.3.6.1.2.1.11.1.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of messages delivered to the SNMP entity from the transport service.")});
    m_oidDatabase.insert(QStringLiteral("1.3.6.1.2.1.11.2.0"),
        {QStringLiteral("snmpOutPkts"), QStringLiteral("1.3.6.1.2.1.11.2.0"),
         QStringLiteral("Counter32"), QStringLiteral("read-only"),
         QStringLiteral("The total number of SNMP messages which were passed from the SNMP protocol entity to the transport service.")});

    Logger::instance().info("MIB", QStringLiteral("内置 OID 数据库已加载 %1 条记录").arg(m_oidDatabase.size()));
}

// ─── Build OID Tree ─────────────────────────────────────────────────────────

void MibWidget::buildBuiltinOidTree()
{
    m_oidTree->clear();

    // ── Root: iso ──
    auto* iso = new QTreeWidgetItem(m_oidTree);
    iso->setText(0, QStringLiteral("iso (1)"));
    iso->setText(1, QStringLiteral("1"));
    iso->setText(2, QStringLiteral(""));
    iso->setText(3, QStringLiteral(""));

    auto* org = new QTreeWidgetItem(iso);
    org->setText(0, QStringLiteral("org (3)"));
    org->setText(1, QStringLiteral("1.3"));
    org->setText(2, QStringLiteral(""));
    org->setText(3, QStringLiteral(""));

    auto* dod = new QTreeWidgetItem(org);
    dod->setText(0, QStringLiteral("dod (6)"));
    dod->setText(1, QStringLiteral("1.3.6"));
    dod->setText(2, QStringLiteral(""));
    dod->setText(3, QStringLiteral(""));

    auto* internet = new QTreeWidgetItem(dod);
    internet->setText(0, QStringLiteral("internet (1)"));
    internet->setText(1, QStringLiteral("1.3.6.1"));
    internet->setText(2, QStringLiteral(""));
    internet->setText(3, QStringLiteral(""));

    // ── mgmt (2) ──
    auto* mgmt = new QTreeWidgetItem(internet);
    mgmt->setText(0, QStringLiteral("mgmt (2)"));
    mgmt->setText(1, QStringLiteral("1.3.6.1.2"));
    mgmt->setText(2, QStringLiteral(""));
    mgmt->setText(3, QStringLiteral(""));

    auto* mib2 = new QTreeWidgetItem(mgmt);
    mib2->setText(0, QStringLiteral("mib-2 (1)"));
    mib2->setText(1, QStringLiteral("1.3.6.1.2.1"));
    mib2->setText(2, QStringLiteral(""));
    mib2->setText(3, QStringLiteral(""));

    // ── system (1) ──
    auto* system = new QTreeWidgetItem(mib2);
    system->setText(0, QStringLiteral("system (1)"));
    system->setText(1, QStringLiteral("1.3.6.1.2.1.1"));
    system->setText(2, QStringLiteral(""));
    system->setText(3, QStringLiteral(""));
    addOidToTree(QStringLiteral("sysDescr"), QStringLiteral("1.3.6.1.2.1.1.1.0"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
                 QStringLiteral("A textual description of the entity."));
    addOidToTree(QStringLiteral("sysObjectID"), QStringLiteral("1.3.6.1.2.1.1.2.0"),
                 QStringLiteral("OBJECT IDENTIFIER"), QStringLiteral("read-only"),
                 QStringLiteral("The vendor's authoritative identification of the network management subsystem."));
    addOidToTree(QStringLiteral("sysUpTime"), QStringLiteral("1.3.6.1.2.1.1.3.0"),
                 QStringLiteral("TimeTicks"), QStringLiteral("read-only"),
                 QStringLiteral("The time since the network management portion of the system was last re-initialized."));
    addOidToTree(QStringLiteral("sysContact"), QStringLiteral("1.3.6.1.2.1.1.4.0"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
                 QStringLiteral("The textual identification of the contact person for this managed node."));
    addOidToTree(QStringLiteral("sysName"), QStringLiteral("1.3.6.1.2.1.1.5.0"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
                 QStringLiteral("An administratively-assigned name for this managed node."));
    addOidToTree(QStringLiteral("sysLocation"), QStringLiteral("1.3.6.1.2.1.1.6.0"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-write"),
                 QStringLiteral("The physical location of this node."));
    addOidToTree(QStringLiteral("sysServices"), QStringLiteral("1.3.6.1.2.1.1.7.0"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("A value which indicates the set of services that this entity may potentially offer."));

    // ── interfaces (2) ──
    auto* interfaces = new QTreeWidgetItem(mib2);
    interfaces->setText(0, QStringLiteral("interfaces (2)"));
    interfaces->setText(1, QStringLiteral("1.3.6.1.2.1.2"));
    interfaces->setText(2, QStringLiteral(""));
    interfaces->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("ifNumber"), QStringLiteral("1.3.6.1.2.1.2.1.0"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("The number of network interfaces present on this system."));

    auto* ifTable = new QTreeWidgetItem(interfaces);
    ifTable->setText(0, QStringLiteral("ifTable (2)"));
    ifTable->setText(1, QStringLiteral("1.3.6.1.2.1.2.2"));
    ifTable->setText(2, QStringLiteral(""));
    ifTable->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("ifIndex"), QStringLiteral("1.3.6.1.2.1.2.2.1.1"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("A unique value for each interface."));
    addOidToTree(QStringLiteral("ifDescr"), QStringLiteral("1.3.6.1.2.1.2.2.1.2"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
                 QStringLiteral("A textual string containing information about the interface."));
    addOidToTree(QStringLiteral("ifType"), QStringLiteral("1.3.6.1.2.1.2.2.1.3"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("The type of interface."));
    addOidToTree(QStringLiteral("ifMtu"), QStringLiteral("1.3.6.1.2.1.2.2.1.4"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("The size of the largest datagram which can be sent/received on the interface."));
    addOidToTree(QStringLiteral("ifSpeed"), QStringLiteral("1.3.6.1.2.1.2.2.1.5"),
                 QStringLiteral("Gauge32"), QStringLiteral("read-only"),
                 QStringLiteral("An estimate of the interface's current bandwidth in bits per second."));
    addOidToTree(QStringLiteral("ifPhysAddress"), QStringLiteral("1.3.6.1.2.1.2.2.1.6"),
                 QStringLiteral("OCTET STRING"), QStringLiteral("read-only"),
                 QStringLiteral("The interface's address at the protocol layer immediately below the network layer."));
    addOidToTree(QStringLiteral("ifAdminStatus"), QStringLiteral("1.3.6.1.2.1.2.2.1.7"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-write"),
                 QStringLiteral("The desired state of the interface."));
    addOidToTree(QStringLiteral("ifOperStatus"), QStringLiteral("1.3.6.1.2.1.2.2.1.8"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("The current operational state of the interface."));
    addOidToTree(QStringLiteral("ifInOctets"), QStringLiteral("1.3.6.1.2.1.2.2.1.10"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of octets received on the interface."));
    addOidToTree(QStringLiteral("ifOutOctets"), QStringLiteral("1.3.6.1.2.1.2.2.1.16"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of octets transmitted out of the interface."));

    // ── ip (4) ──
    auto* ip = new QTreeWidgetItem(mib2);
    ip->setText(0, QStringLiteral("ip (4)"));
    ip->setText(1, QStringLiteral("1.3.6.1.2.1.4"));
    ip->setText(2, QStringLiteral(""));
    ip->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("ipForwarding"), QStringLiteral("1.3.6.1.2.1.4.1.0"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-write"),
                 QStringLiteral("The indication of whether this entity is acting as an IP gateway."));
    addOidToTree(QStringLiteral("ipDefaultTTL"), QStringLiteral("1.3.6.1.2.1.4.2.0"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-write"),
                 QStringLiteral("The default value inserted into the Time-To-Live field of the IP header."));
    addOidToTree(QStringLiteral("ipInReceives"), QStringLiteral("1.3.6.1.2.1.4.3.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of input datagrams received from interfaces."));
    addOidToTree(QStringLiteral("ipInHdrErrors"), QStringLiteral("1.3.6.1.2.1.4.4.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of input datagrams discarded due to errors in their IP headers."));
    addOidToTree(QStringLiteral("ipInAddrErrors"), QStringLiteral("1.3.6.1.2.1.4.5.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of input datagrams discarded because the destination IP address was invalid."));
    addOidToTree(QStringLiteral("ipForwDatagrams"), QStringLiteral("1.3.6.1.2.1.4.6.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of input datagrams for which this entity was not their final IP destination."));
    addOidToTree(QStringLiteral("ipOutRequests"), QStringLiteral("1.3.6.1.2.1.4.9.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of IP datagrams which local IP user-protocols supplied to IP."));

    // ── icmp (5) ──
    auto* icmp = new QTreeWidgetItem(mib2);
    icmp->setText(0, QStringLiteral("icmp (5)"));
    icmp->setText(1, QStringLiteral("1.3.6.1.2.1.5"));
    icmp->setText(2, QStringLiteral(""));
    icmp->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("icmpInMsgs"), QStringLiteral("1.3.6.1.2.1.5.1.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of ICMP messages which the entity received."));
    addOidToTree(QStringLiteral("icmpInErrors"), QStringLiteral("1.3.6.1.2.1.5.2.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of ICMP messages which the entity received but had ICMP-specific errors."));
    addOidToTree(QStringLiteral("icmpOutMsgs"), QStringLiteral("1.3.6.1.2.1.5.14.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of ICMP messages which this entity attempted to send."));

    // ── tcp (6) ──
    auto* tcp = new QTreeWidgetItem(mib2);
    tcp->setText(0, QStringLiteral("tcp (6)"));
    tcp->setText(1, QStringLiteral("1.3.6.1.2.1.6"));
    tcp->setText(2, QStringLiteral(""));
    tcp->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("tcpRtoAlgorithm"), QStringLiteral("1.3.6.1.2.1.6.1.0"),
                 QStringLiteral("INTEGER"), QStringLiteral("read-only"),
                 QStringLiteral("The algorithm used to determine the timeout value used for retransmitting unacknowledged octets."));
    addOidToTree(QStringLiteral("tcpActiveOpens"), QStringLiteral("1.3.6.1.2.1.6.5.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of times TCP connections have made a direct transition to the SYN-SENT state."));
    addOidToTree(QStringLiteral("tcpPassiveOpens"), QStringLiteral("1.3.6.1.2.1.6.6.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The number of times TCP connections have made a direct transition to the SYN-RCVD state."));
    addOidToTree(QStringLiteral("tcpInSegs"), QStringLiteral("1.3.6.1.2.1.6.9.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of segments received, including those received in error."));

    // ── udp (7) ──
    auto* udp = new QTreeWidgetItem(mib2);
    udp->setText(0, QStringLiteral("udp (7)"));
    udp->setText(1, QStringLiteral("1.3.6.1.2.1.7"));
    udp->setText(2, QStringLiteral(""));
    udp->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("udpInDatagrams"), QStringLiteral("1.3.6.1.2.1.7.1.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of UDP datagrams delivered to UDP users."));
    addOidToTree(QStringLiteral("udpNoPorts"), QStringLiteral("1.3.6.1.2.1.7.2.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of received UDP datagrams for which there was no application at the destination port."));
    addOidToTree(QStringLiteral("udpOutDatagrams"), QStringLiteral("1.3.6.1.2.1.7.4.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of UDP datagrams sent from this entity."));

    // ── snmp (11) ──
    auto* snmpNode = new QTreeWidgetItem(mib2);
    snmpNode->setText(0, QStringLiteral("snmp (11)"));
    snmpNode->setText(1, QStringLiteral("1.3.6.1.2.1.11"));
    snmpNode->setText(2, QStringLiteral(""));
    snmpNode->setText(3, QStringLiteral(""));

    addOidToTree(QStringLiteral("snmpInPkts"), QStringLiteral("1.3.6.1.2.1.11.1.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of messages delivered to the SNMP entity from the transport service."));
    addOidToTree(QStringLiteral("snmpOutPkts"), QStringLiteral("1.3.6.1.2.1.11.2.0"),
                 QStringLiteral("Counter32"), QStringLiteral("read-only"),
                 QStringLiteral("The total number of SNMP messages which were passed from the SNMP protocol entity to the transport service."));

    m_oidTree->expandToDepth(2);
}

void MibWidget::addOidToTree(const QString& name, const QString& oid,
                              const QString& type, const QString& access,
                              const QString& description)
{
    // Find the parent by walking the OID hierarchy
    QStringList parts = oid.split('.');
    if (parts.size() < 2) return;

    // Walk up to the parent OID
    QStringList parentParts = parts.mid(0, parts.size() - 2);
    QString parentOid = parentParts.join('.');

    // Find parent node in tree
    QTreeWidgetItem* parent = findParentNode(m_oidTree->invisibleRootItem(), parentOid);
    if (!parent) {
        parent = m_oidTree->invisibleRootItem();
    }

    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, name);
    item->setText(1, oid);
    item->setText(2, type);
    item->setText(3, access);
    item->setToolTip(1, oid);
    item->setToolTip(0, description);
    item->setData(0, Qt::UserRole, description);
}

QTreeWidgetItem* MibWidget::findParentNode(QTreeWidgetItem* root, const QString& oid)
{
    if (!root) return nullptr;

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* child = root->child(i);
        if (child->text(1) == oid) {
            return child;
        }
        QTreeWidgetItem* found = findParentNode(child, oid);
        if (found) return found;
    }
    return nullptr;
}

// ─── Import MIB ──────────────────────────────────────────────────────────────

void MibWidget::onImportMib()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("导入 MIB 文件"),
        QString(),
        QStringLiteral("MIB 文件 (*.mib *.txt *.my);;所有文件 (*)")
    );

    if (files.isEmpty()) return;

    for (const QString& filePath : files) {
        if (m_loadedMibFiles.contains(filePath)) {
            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("MIB 文件已导入: %1").arg(filePath));
            continue;
        }
        loadMibFile(filePath);
    }
}

void MibWidget::loadMibFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"),
                             QStringLiteral("无法打开文件: %1").arg(filePath));
        Logger::instance().error("MIB", "Failed to open file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    QStringList lines = content.split('\n');

    QString currentName;
    QString currentOid;
    QString currentType;
    QString currentAccess;
    QString currentDescription;
    bool inObjectType = false;
    bool inDescription = false;
    QString descriptionBuffer;

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();

        // Skip comments and empty lines
        if (line.isEmpty() || line.startsWith(QStringLiteral("--"))) {
            if (inDescription && !line.isEmpty()) {
                descriptionBuffer += line.mid(2).trimmed() + " ";
            }
            continue;
        }

        // Detect OBJECT-TYPE definition
        if (line.contains(QStringLiteral("OBJECT-TYPE"), Qt::CaseInsensitive) ||
            line.contains(QStringLiteral("MODULE-IDENTITY"), Qt::CaseInsensitive) ||
            line.contains(QStringLiteral("OBJECT-IDENTITY"), Qt::CaseInsensitive)) {

            if (inObjectType && !currentName.isEmpty() && !currentOid.isEmpty()) {
                // Save previous entry
                if (!m_oidDatabase.contains(currentOid)) {
                    m_oidDatabase.insert(currentOid,
                        {currentName, currentOid, currentType, currentAccess, descriptionBuffer.trimmed()});
                    addOidToTree(currentName, currentOid, currentType, currentAccess, descriptionBuffer.trimmed());
                }
            }

            // Reset for new entry
            inObjectType = true;
            inDescription = false;
            currentName.clear();
            currentOid.clear();
            currentType = QStringLiteral("OBJECT IDENTIFIER");
            currentAccess.clear();
            descriptionBuffer.clear();

            // Extract name - usually the first word before OBJECT-TYPE
            static const QRegularExpression nameRe(QStringLiteral(R"(^(\w+)\s+OBJECT-TYPE)"));
            QRegularExpressionMatch nameMatch = nameRe.match(line);
            if (nameMatch.hasMatch()) {
                currentName = nameMatch.captured(1);
            }

            continue;
        }

        if (!inObjectType) continue;

        // Parse SYNTAX
        if (line.contains(QStringLiteral("SYNTAX"), Qt::CaseInsensitive)) {
            static const QRegularExpression syntaxRe(QStringLiteral(R"(SYNTAX\s+(.+?)(?:\s*\{|\s*\(|\s*$))"),
                                                     QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch syntaxMatch = syntaxRe.match(line);
            if (syntaxMatch.hasMatch()) {
                currentType = syntaxMatch.captured(1).trimmed();
            }
            continue;
        }

        // Parse ACCESS / MAX-ACCESS
        if (line.contains(QStringLiteral("ACCESS"), Qt::CaseInsensitive) ||
            line.contains(QStringLiteral("MAX-ACCESS"), Qt::CaseInsensitive)) {
            static const QRegularExpression accessRe(QStringLiteral(R"((?:MAX-)?ACCESS\s+(\S+))"),
                                                     QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch accessMatch = accessRe.match(line);
            if (accessMatch.hasMatch()) {
                currentAccess = accessMatch.captured(1).toLower();
            }
            continue;
        }

        // Parse DESCRIPTION
        if (line.contains(QStringLiteral("DESCRIPTION"), Qt::CaseInsensitive)) {
            inDescription = true;
            // Check if description continues on the same line
            QString afterDesc = line.mid(line.indexOf(QStringLiteral("DESCRIPTION"), 0, Qt::CaseInsensitive) + 11).trimmed();
            if (!afterDesc.isEmpty() && afterDesc != QStringLiteral("\"")) {
                descriptionBuffer = afterDesc;
            }
            continue;
        }

        // Parse ::= { ... } for OID
        if (line.contains(QStringLiteral("::="))) {
            static const QRegularExpression oidRe(QStringLiteral(R"(::=\s*\{\s*([^}]+)\s*\})"));
            QRegularExpressionMatch oidMatch = oidRe.match(line);
            if (oidMatch.hasMatch()) {
                QString oidRef = oidMatch.captured(1).trimmed();
                // Parse OID reference like "mib-2 1" or "system 1"
                QStringList oidParts = oidRef.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                if (!oidParts.isEmpty()) {
                    // Try to resolve the parent OID
                    QString parentOid = resolveOidDescription(oidParts.first());
                    if (parentOid.isEmpty()) {
                        parentOid = oidParts.first();
                    }
                    if (oidParts.size() >= 2) {
                        currentOid = parentOid + "." + oidParts.last();
                    }
                }
            }
            continue;
        }

        // Collect description lines
        if (inDescription) {
            QString cleanLine = line;
            if (cleanLine.startsWith('"')) cleanLine = cleanLine.mid(1);
            if (cleanLine.endsWith('"')) {
                cleanLine = cleanLine.left(cleanLine.length() - 1);
                inDescription = false;
            }
            descriptionBuffer += cleanLine + " ";
        }

        // Check for end of entry
        if (line == QStringLiteral("::=") || line.startsWith(QStringLiteral("::="))) {
            continue;
        }
    }

    // Save last entry
    if (inObjectType && !currentName.isEmpty() && !currentOid.isEmpty()) {
        if (!m_oidDatabase.contains(currentOid)) {
            m_oidDatabase.insert(currentOid,
                {currentName, currentOid, currentType, currentAccess, descriptionBuffer.trimmed()});
            addOidToTree(currentName, currentOid, currentType, currentAccess, descriptionBuffer.trimmed());
        }
    }

    m_loadedMibFiles.append(filePath);

    // Add to file list widget
    QFileInfo fi(filePath);
    m_mibFileList->addItem(fi.fileName());

    Logger::instance().info("MIB", "Loaded MIB file: " + filePath);
    QMessageBox::information(this, QStringLiteral("导入成功"),
                             QStringLiteral("MIB 文件已导入: %1").arg(fi.fileName()));
}

void MibWidget::parseMibLine(const QString& line)
{
    Q_UNUSED(line)
    // Parsing is done inline in loadMibFile()
}

QString MibWidget::resolveOidDescription(const QString& oid)
{
    // Try exact match in database
    for (auto it = m_oidDatabase.constBegin(); it != m_oidDatabase.constEnd(); ++it) {
        if (it.value().name.compare(oid, Qt::CaseInsensitive) == 0) {
            return it.value().oid;
        }
    }

    // Known MIB module parent OIDs
    static const QHash<QString, QString> knownParents = {
        {QStringLiteral("system"),       QStringLiteral("1.3.6.1.2.1.1")},
        {QStringLiteral("interfaces"),   QStringLiteral("1.3.6.1.2.1.2")},
        {QStringLiteral("at"),           QStringLiteral("1.3.6.1.2.1.3")},
        {QStringLiteral("ip"),           QStringLiteral("1.3.6.1.2.1.4")},
        {QStringLiteral("icmp"),         QStringLiteral("1.3.6.1.2.1.5")},
        {QStringLiteral("tcp"),          QStringLiteral("1.3.6.1.2.1.6")},
        {QStringLiteral("udp"),          QStringLiteral("1.3.6.1.2.1.7")},
        {QStringLiteral("egp"),          QStringLiteral("1.3.6.1.2.1.8")},
        {QStringLiteral("transmission"), QStringLiteral("1.3.6.1.2.1.10")},
        {QStringLiteral("snmp"),         QStringLiteral("1.3.6.1.2.1.11")},
        {QStringLiteral("mib-2"),        QStringLiteral("1.3.6.1.2.1")},
        {QStringLiteral("mgmt"),         QStringLiteral("1.3.6.1.2")},
        {QStringLiteral("internet"),     QStringLiteral("1.3.6.1")},
        {QStringLiteral("private"),      QStringLiteral("1.3.6.1.4")},
        {QStringLiteral("enterprises"),  QStringLiteral("1.3.6.1.4.1")},
        {QStringLiteral("iso"),          QStringLiteral("1")},
        {QStringLiteral("org"),          QStringLiteral("1.3")},
        {QStringLiteral("dod"),          QStringLiteral("1.3.6")},
        {QStringLiteral("ifMIB"),        QStringLiteral("1.3.6.1.2.1.31")},
        {QStringLiteral("ifTable"),      QStringLiteral("1.3.6.1.2.1.2.2")},
        {QStringLiteral("ifEntry"),      QStringLiteral("1.3.6.1.2.1.2.2.1")},
    };

    if (knownParents.contains(oid)) {
        return knownParents.value(oid);
    }

    return QString();
}

// ─── Delete MIB ──────────────────────────────────────────────────────────────

void MibWidget::onDeleteMib()
{
    int currentRow = m_mibFileList->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的 MIB 文件。"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除选中的 MIB 文件吗？\n注意: 已导入的 OID 节点不会被移除。"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (currentRow < m_loadedMibFiles.size()) {
            m_loadedMibFiles.removeAt(currentRow);
        }
        delete m_mibFileList->takeItem(currentRow);
        Logger::instance().info("MIB", QStringLiteral("MIB 文件已从列表中删除"));
    }
}

// ─── Search OID ──────────────────────────────────────────────────────────────

void MibWidget::onSearchOid()
{
    QString keyword = m_oidSearchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请输入搜索关键字。"));
        return;
    }

    // Collapse all first
    m_oidTree->collapseAll();

    // Search and highlight matching items
    QList<QTreeWidgetItem*> matchedItems;
    searchTreeItems(m_oidTree->invisibleRootItem(), keyword, matchedItems);

    if (matchedItems.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("搜索结果"),
                                 QStringLiteral("未找到匹配的 OID: %1").arg(keyword));
        return;
    }

    // Expand and select the first match
    for (QTreeWidgetItem* item : matchedItems) {
        // Expand all ancestors
        QTreeWidgetItem* parent = item->parent();
        while (parent) {
            parent->setExpanded(true);
            parent = parent->parent();
        }
        // Highlight - set background
        item->setBackground(0, QColor("#3C5A3C"));
        item->setBackground(1, QColor("#3C5A3C"));
        item->setBackground(2, QColor("#3C5A3C"));
        item->setBackground(3, QColor("#3C5A3C"));
    }

    m_oidTree->scrollToItem(matchedItems.first());
    m_oidTree->setCurrentItem(matchedItems.first());

    Logger::instance().info("MIB", QStringLiteral("搜索 OID: %1, 找到 %2 条结果")
                                      .arg(keyword).arg(matchedItems.size()));
}

void MibWidget::searchTreeItems(QTreeWidgetItem* root, const QString& keyword,
                                 QList<QTreeWidgetItem*>& results)
{
    if (!root) return;

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* child = root->child(i);
        // Check both name and OID columns
        if (child->text(0).contains(keyword, Qt::CaseInsensitive) ||
            child->text(1).contains(keyword, Qt::CaseInsensitive)) {
            results.append(child);
        }
        // Recurse into children
        searchTreeItems(child, keyword, results);
    }
}

// ─── Add Favorite ────────────────────────────────────────────────────────────

void MibWidget::onAddFavorite()
{
    QTreeWidgetItem* currentItem = m_oidTree->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在 OID 树中选择一个节点。"));
        return;
    }

    QString oid = currentItem->text(1);
    QString name = currentItem->text(0);

    if (oid.isEmpty() || name.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("所选节点不是有效的 OID 条目。"));
        return;
    }

    // Save to SQLite database
    if (DatabaseManager::instance().isOpen()) {
        QSqlQuery query(DatabaseManager::instance().database());
        query.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)"
        ));
        query.addBindValue(QStringLiteral("mib_favorite_") + oid);
        query.addBindValue(name);

        if (!query.exec()) {
            Logger::instance().error("MIB", "Failed to save favorite: " + query.lastError().text());
            QMessageBox::warning(this, QStringLiteral("收藏失败"),
                                 QStringLiteral("无法保存收藏: ") + query.lastError().text());
            return;
        }
    }

    Logger::instance().info("MIB", QStringLiteral("已收藏 OID: %1 (%2)").arg(name, oid));
    QMessageBox::information(this, QStringLiteral("收藏成功"),
                             QStringLiteral("OID 已添加到收藏夹: %1").arg(name));
}

// ─── OID Tree Item Click ────────────────────────────────────────────────────

void MibWidget::onOidTreeItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;

    QString name = item->text(0);
    QString oid = item->text(1);
    QString type = item->text(2);
    QString access = item->text(3);
    QString description = item->data(0, Qt::UserRole).toString();

    // If no description from tree, try database
    if (description.isEmpty() && m_oidDatabase.contains(oid)) {
        description = m_oidDatabase.value(oid).description;
    }

    m_detailArea->clear();
    m_detailArea->appendPlainText(QStringLiteral("═══════════════════════════════════"));
    m_detailArea->appendPlainText(QStringLiteral("  名称: ") + name);
    m_detailArea->appendPlainText(QStringLiteral("  OID:  ") + oid);
    m_detailArea->appendPlainText(QStringLiteral("  类型: ") + type);
    m_detailArea->appendPlainText(QStringLiteral("  访问: ") + access);
    m_detailArea->appendPlainText(QStringLiteral("═══════════════════════════════════"));

    if (!description.isEmpty()) {
        m_detailArea->appendPlainText(QString());
        m_detailArea->appendPlainText(QStringLiteral("描述:"));
        m_detailArea->appendPlainText(QStringLiteral("  ") + description);
    }
}

// ─── SNMP Get ────────────────────────────────────────────────────────────────

void MibWidget::onSnmpGet()
{
    QString deviceIp = m_deviceIpEdit->text().trimmed();
    QString community = m_communityEdit->text().trimmed();

    if (deviceIp.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入设备 IP 地址。"));
        return;
    }

    QTreeWidgetItem* currentItem = m_oidTree->currentItem();
    if (!currentItem || currentItem->text(1).isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先在 OID 树中选择一个 OID 节点。"));
        return;
    }

    QString oid = currentItem->text(1);
    m_snmpResultArea->clear();
    m_snmpResultArea->appendPlainText(QStringLiteral("正在执行 snmpget %1 %2...").arg(deviceIp, oid));

    if (m_snmpProcess) {
        m_snmpProcess->kill();
        m_snmpProcess->waitForFinished(3000);
        delete m_snmpProcess;
        m_snmpProcess = nullptr;
    }

    m_snmpProcess = new QProcess(this);
    connect(m_snmpProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_snmpProcess) {
            QString output = QString::fromUtf8(m_snmpProcess->readAllStandardOutput());
            m_snmpResultArea->appendPlainText(output.trimmed());
        }
    });
    connect(m_snmpProcess, &QProcess::readyReadStandardError, this, [this]() {
        if (m_snmpProcess) {
            QString err = QString::fromUtf8(m_snmpProcess->readAllStandardError());
            if (!err.trimmed().isEmpty()) {
                m_snmpResultArea->appendPlainText(QStringLiteral("[ERR] ") + err.trimmed());
            }
        }
    });
    connect(m_snmpProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            m_snmpResultArea->appendPlainText(QStringLiteral("snmpget 执行失败，退出码: %1").arg(exitCode));
            Logger::instance().error("MIB", QStringLiteral("snmpget 失败, 退出码: %1").arg(exitCode));
        } else {
            Logger::instance().info("MIB", QStringLiteral("snmpget 完成"));
        }
        if (m_snmpProcess) {
            m_snmpProcess->deleteLater();
            m_snmpProcess = nullptr;
        }
    });

    QStringList args;
    args << QStringLiteral("-v2c") << QStringLiteral("-c") << community << deviceIp << oid;
    m_snmpProcess->start(QStringLiteral("snmpget"), args);

    Logger::instance().info("MIB", QStringLiteral("执行 snmpget -v2c -c %1 %2 %3")
                                      .arg(community, deviceIp, oid));
}

// ─── SNMP Walk ───────────────────────────────────────────────────────────────

void MibWidget::onSnmpWalk()
{
    QString deviceIp = m_deviceIpEdit->text().trimmed();
    QString community = m_communityEdit->text().trimmed();

    if (deviceIp.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入设备 IP 地址。"));
        return;
    }

    QTreeWidgetItem* currentItem = m_oidTree->currentItem();
    if (!currentItem || currentItem->text(1).isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先在 OID 树中选择一个 OID 节点。"));
        return;
    }

    QString oid = currentItem->text(1);
    m_snmpResultArea->clear();
    m_snmpResultArea->appendPlainText(QStringLiteral("正在执行 snmpwalk %1 %2...").arg(deviceIp, oid));

    if (m_snmpProcess) {
        m_snmpProcess->kill();
        m_snmpProcess->waitForFinished(3000);
        delete m_snmpProcess;
        m_snmpProcess = nullptr;
    }

    m_snmpProcess = new QProcess(this);
    connect(m_snmpProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_snmpProcess) {
            QString output = QString::fromUtf8(m_snmpProcess->readAllStandardOutput());
            m_snmpResultArea->appendPlainText(output.trimmed());
        }
    });
    connect(m_snmpProcess, &QProcess::readyReadStandardError, this, [this]() {
        if (m_snmpProcess) {
            QString err = QString::fromUtf8(m_snmpProcess->readAllStandardError());
            if (!err.trimmed().isEmpty()) {
                m_snmpResultArea->appendPlainText(QStringLiteral("[ERR] ") + err.trimmed());
            }
        }
    });
    connect(m_snmpProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            m_snmpResultArea->appendPlainText(QStringLiteral("snmpwalk 执行失败，退出码: %1").arg(exitCode));
            Logger::instance().error("MIB", QStringLiteral("snmpwalk 失败, 退出码: %1").arg(exitCode));
        } else {
            Logger::instance().info("MIB", QStringLiteral("snmpwalk 完成"));
        }
        if (m_snmpProcess) {
            m_snmpProcess->deleteLater();
            m_snmpProcess = nullptr;
        }
    });

    QStringList args;
    args << QStringLiteral("-v2c") << QStringLiteral("-c") << community << deviceIp << oid;
    m_snmpProcess->start(QStringLiteral("snmpwalk"), args);

    Logger::instance().info("MIB", QStringLiteral("执行 snmpwalk -v2c -c %1 %2 %3")
                                      .arg(community, deviceIp, oid));
}

// ─── Export CSV ──────────────────────────────────────────────────────────────

void MibWidget::onExport()
{
    // Collect all leaf OIDs from the tree
    QVector<MibOidInfo> exportData;
    collectLeafOids(m_oidTree->invisibleRootItem(), exportData);

    if (exportData.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("没有可导出的 OID 数据。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出 OID 列表"),
        QStringLiteral("mib_oid_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        QStringLiteral("CSV 文件 (*.csv)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("无法写入文件: ") + filePath);
        Logger::instance().error("MIB", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF";
    stream << QStringLiteral("名称,OID,类型,访问,描述\n");

    for (const auto& info : exportData) {
        auto escapeCsv = [](const QString& s) -> QString {
            if (s.contains(',') || s.contains('"') || s.contains('\n')) {
                QString escaped = s;
                escaped.replace('"', QStringLiteral("\"\""));
                return '"' + escaped + '"';
            }
            return s;
        };

        stream << escapeCsv(info.name) << ","
               << escapeCsv(info.oid) << ","
               << escapeCsv(info.type) << ","
               << escapeCsv(info.access) << ","
               << escapeCsv(info.description) << "\n";
    }

    file.close();
    Logger::instance().info("MIB", "OID list exported to: " + filePath);
    QMessageBox::information(this, QStringLiteral("导出成功"),
                              QStringLiteral("已导出 %1 条 OID 记录到:\n%2")
                                  .arg(exportData.size()).arg(filePath));
}

void MibWidget::collectLeafOids(QTreeWidgetItem* root, QVector<MibOidInfo>& results)
{
    if (!root) return;

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* child = root->child(i);
        QString oid = child->text(1);
        QString name = child->text(0);
        QString type = child->text(2);
        QString access = child->text(3);

        // If it has a valid OID (leaf), add it
        if (!oid.isEmpty() && !name.isEmpty() && child->childCount() == 0) {
            QString description = child->data(0, Qt::UserRole).toString();
            if (description.isEmpty() && m_oidDatabase.contains(oid)) {
                description = m_oidDatabase.value(oid).description;
            }
            results.append({name, oid, type, access, description});
        }

        // Recurse
        collectLeafOids(child, results);
    }
}