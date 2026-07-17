#include "snmp/SNMPMonitorWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QDateTime>
#include <QRegularExpression>
#include <QApplication>
#include <QSet>
#include <QColor>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════════
// SNMPMonitorWidget
// ═══════════════════════════════════════════════════════════════════════════════

SNMPMonitorWidget::SNMPMonitorWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollTimer(nullptr)
    , m_isMonitoring(false)
    , m_process(nullptr)
    , m_currentQueryType(0)
{
    setupUI();
    setupConnections();
}

SNMPMonitorWidget::~SNMPMonitorWidget()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void SNMPMonitorWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──
    auto styleLineEdit = [](QLineEdit* edit, const QString& placeholder, int minWidth = 130) {
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

    auto styleCombo = [](QComboBox* combo, int minWidth = 100) {
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

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
            "QTableWidget {"
            "  background-color: #1E1F22; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; font-size: 12px;"
            "  gridline-color: #2C2D30;"
            "}"
            "QTableWidget::item { padding: 3px 6px; }"
            "QTableWidget::item:selected { background-color: #3C3F41; }"
            "QHeaderView::section {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  border: none; border-bottom: 2px solid #3C3F41;"
            "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
            "}"
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString("QPushButton {"
                    "  background-color: %1; color: white;"
                    "  border: none; padding: 8px 20px; border-radius: 4px;"
                    "  font-size: 13px; font-weight: bold;"
                    "}"
                    "QPushButton:hover { background-color: %2; }"
                    "QPushButton:disabled { background-color: #5C5C5C; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleLabel = [](QLabel* label) {
        label->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    };

    auto styleSpin = [](QSpinBox* spin, int width = 80) {
        spin->setFixedWidth(width);
        spin->setStyleSheet(
            "QSpinBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        );
    };

    // ── Top: Configuration area ──
    auto* configGroup = new QGroupBox("SNMP 监控配置");
    auto* configLayout = new QHBoxLayout(configGroup);
    configLayout->setSpacing(12);

    // Device IP
    auto* ipLayout = new QVBoxLayout();
    auto* ipLabel = new QLabel("设备 IP:");
    styleLabel(ipLabel);
    m_ipEdit = new QLineEdit();
    styleLineEdit(m_ipEdit, "例如: 192.168.1.1", 150);
    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(m_ipEdit);
    configLayout->addLayout(ipLayout);

    // SNMP Version
    auto* versionLayout = new QVBoxLayout();
    auto* versionLabel = new QLabel("SNMP 版本:");
    styleLabel(versionLabel);
    m_versionCombo = new QComboBox();
    m_versionCombo->addItem("v1", "1");
    m_versionCombo->addItem("v2c", "2c");
    m_versionCombo->addItem("v3", "3");
    styleCombo(m_versionCombo, 90);
    versionLayout->addWidget(versionLabel);
    versionLayout->addWidget(m_versionCombo);
    configLayout->addLayout(versionLayout);

    // Community / User
    auto* communityLayout = new QVBoxLayout();
    auto* communityLabel = new QLabel("Community/用户:");
    styleLabel(communityLabel);
    m_communityEdit = new QLineEdit();
    m_communityEdit->setText("public");
    styleLineEdit(m_communityEdit, "public", 130);
    communityLayout->addWidget(communityLabel);
    communityLayout->addWidget(m_communityEdit);
    configLayout->addLayout(communityLayout);

    // Monitor Type
    auto* typeLayout = new QVBoxLayout();
    auto* typeLabel = new QLabel("监控类型:");
    styleLabel(typeLabel);
    m_monitorTypeCombo = new QComboBox();
    m_monitorTypeCombo->addItem("System Info", "system");
    m_monitorTypeCombo->addItem("Interface", "interface");
    m_monitorTypeCombo->addItem("CPU", "cpu");
    m_monitorTypeCombo->addItem("Memory", "memory");
    m_monitorTypeCombo->addItem("Temperature", "temperature");
    m_monitorTypeCombo->addItem("Custom OID", "custom");
    styleCombo(m_monitorTypeCombo, 130);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_monitorTypeCombo);
    configLayout->addLayout(typeLayout);

    // Custom OID
    auto* oidLayout = new QVBoxLayout();
    auto* oidLabel = new QLabel("自定义 OID:");
    styleLabel(oidLabel);
    m_customOidEdit = new QLineEdit();
    styleLineEdit(m_customOidEdit, "例如: 1.3.6.1.2.1.1.1.0", 180);
    m_customOidEdit->setEnabled(false);
    oidLayout->addWidget(oidLabel);
    oidLayout->addWidget(m_customOidEdit);
    configLayout->addLayout(oidLayout);

    configLayout->addStretch();
    mainLayout->addWidget(configGroup);

    // ── Middle: Tabbed results area ──
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #3C3F41; background-color: #1E1F22;"
        "}"
        "QTabBar::tab {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  padding: 6px 16px; border: 1px solid #3C3F41;"
        "  border-bottom: none; font-size: 12px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #1E1F22; color: #409EFF;"
        "  border-bottom: 2px solid #409EFF;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: #2C2D30; color: #DCDCDC;"
        "}"
    );

    // Tab 0: System Info
    m_systemTable = new QTableWidget(0, 4);
    m_systemTable->setHorizontalHeaderLabels({"指标", "当前值", "状态", "上次更新"});
    m_systemTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_systemTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_systemTable->setColumnWidth(2, 80);
    m_systemTable->setColumnWidth(3, 160);
    styleTable(m_systemTable);
    m_tabWidget->addTab(m_systemTable, "系统信息");

    // Tab 1: Interface Traffic
    m_interfaceTable = new QTableWidget(0, 7);
    m_interfaceTable->setHorizontalHeaderLabels({"接口", "状态", "入流量", "出流量", "入包", "出包", "错误"});
    m_interfaceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_interfaceTable->setColumnWidth(1, 70);
    m_interfaceTable->setColumnWidth(2, 110);
    m_interfaceTable->setColumnWidth(3, 110);
    m_interfaceTable->setColumnWidth(4, 90);
    m_interfaceTable->setColumnWidth(5, 90);
    m_interfaceTable->setColumnWidth(6, 80);
    styleTable(m_interfaceTable);
    m_tabWidget->addTab(m_interfaceTable, "接口流量");

    // Tab 2: CPU / Memory
    m_cpuMemTable = new QTableWidget(0, 4);
    m_cpuMemTable->setHorizontalHeaderLabels({"指标", "当前值", "状态", "上次更新"});
    m_cpuMemTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_cpuMemTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_cpuMemTable->setColumnWidth(2, 80);
    m_cpuMemTable->setColumnWidth(3, 160);
    styleTable(m_cpuMemTable);
    m_tabWidget->addTab(m_cpuMemTable, "CPU/内存");

    mainLayout->addWidget(m_tabWidget, 1);

    // ── Bottom: Control area ──
    auto* controlGroup = new QGroupBox("监控控制");
    auto* controlLayout = new QHBoxLayout(controlGroup);
    controlLayout->setSpacing(12);

    // Start button
    m_startBtn = new QPushButton("开始监控");
    styleButton(m_startBtn, "#409EFF", "#66B1FF");

    // Stop button
    m_stopBtn = new QPushButton("停止监控");
    styleButton(m_stopBtn, "#F56C6C", "#F78989");
    m_stopBtn->setEnabled(false);

    // Poll interval
    auto* intervalLabel = new QLabel("轮询间隔:");
    styleLabel(intervalLabel);
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(1, 300);
    m_intervalSpin->setValue(10);
    m_intervalSpin->setSuffix(" 秒");
    styleSpin(m_intervalSpin, 100);

    // Auto refresh
    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);
    m_autoRefreshCheck->setStyleSheet(
        "QCheckBox { color: #DCDCDC; font-size: 13px; }"
        "QCheckBox::indicator {"
        "  width: 16px; height: 16px;"
        "  border: 1px solid #3C3F41; border-radius: 3px;"
        "  background-color: #25262B;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #409EFF; border-color: #409EFF;"
        "}"
    );

    // Export button
    m_exportBtn = new QPushButton("导出 CSV");
    styleButton(m_exportBtn, "#67C23A", "#85CE61");

    controlLayout->addWidget(m_startBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addSpacing(24);
    controlLayout->addWidget(intervalLabel);
    controlLayout->addWidget(m_intervalSpin);
    controlLayout->addWidget(m_autoRefreshCheck);
    controlLayout->addStretch();
    controlLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(controlGroup);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void SNMPMonitorWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &SNMPMonitorWidget::onStartMonitor);
    connect(m_stopBtn, &QPushButton::clicked, this, &SNMPMonitorWidget::onStopMonitor);
    connect(m_exportBtn, &QPushButton::clicked, this, &SNMPMonitorWidget::onExport);
    connect(m_monitorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SNMPMonitorWidget::onMonitorTypeChanged);
}

// ─── Monitor Type Changed ────────────────────────────────────────────────────

void SNMPMonitorWidget::onMonitorTypeChanged(int /*index*/)
{
    QString type = m_monitorTypeCombo->currentData().toString();
    m_customOidEdit->setEnabled(type == "custom");

    // Switch to appropriate tab
    if (type == "system") {
        m_tabWidget->setCurrentIndex(0);
    } else if (type == "interface") {
        m_tabWidget->setCurrentIndex(1);
    } else if (type == "cpu" || type == "memory" || type == "temperature") {
        m_tabWidget->setCurrentIndex(2);
    } else if (type == "custom") {
        m_tabWidget->setCurrentIndex(0);
    }
}

// ─── Start / Stop ────────────────────────────────────────────────────────────

void SNMPMonitorWidget::onStartMonitor()
{
    QString ip = m_ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入设备 IP 地址。");
        return;
    }

    // Check for custom OID
    QString type = m_monitorTypeCombo->currentData().toString();
    if (type == "custom" && m_customOidEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入自定义 OID。");
        return;
    }

    m_isMonitoring = true;
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_ipEdit->setEnabled(false);
    m_versionCombo->setEnabled(false);
    m_communityEdit->setEnabled(false);
    m_monitorTypeCombo->setEnabled(false);
    m_customOidEdit->setEnabled(false);
    m_intervalSpin->setEnabled(false);

    // Clear previous interface counters
    m_prevInterfaceCounters.clear();

    if (!m_pollTimer) {
        m_pollTimer = new QTimer(this);
        connect(m_pollTimer, &QTimer::timeout, this, &SNMPMonitorWidget::onPoll);
    }

    Logger::instance().info("SNMP Monitor",
        QString("开始监控 %1 [%2]").arg(ip, type));

    // Execute first poll immediately
    onPoll();

    // Start timer
    m_pollTimer->start(m_intervalSpin->value() * 1000);
}

void SNMPMonitorWidget::onStopMonitor()
{
    m_isMonitoring = false;

    if (m_pollTimer) {
        m_pollTimer->stop();
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }

    m_currentQueryType = 0;

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_ipEdit->setEnabled(true);
    m_versionCombo->setEnabled(true);
    m_communityEdit->setEnabled(true);
    m_monitorTypeCombo->setEnabled(true);
    if (m_monitorTypeCombo->currentData().toString() == "custom") {
        m_customOidEdit->setEnabled(true);
    }
    m_intervalSpin->setEnabled(true);

    Logger::instance().info("SNMP Monitor", "监控已停止");
}

// ─── Poll ────────────────────────────────────────────────────────────────────

void SNMPMonitorWidget::onPoll()
{
    if (!m_isMonitoring) return;

    // Skip if a query is already in progress
    if (m_process && m_process->state() != QProcess::NotRunning) {
        return;
    }

    QString ip = m_ipEdit->text().trimmed();
    QString community = m_communityEdit->text().trimmed();
    QString type = m_monitorTypeCombo->currentData().toString();

    m_currentQueryType = 0;

    if (type == "system") {
        m_currentQueryType = 1;
        querySystemInfo(ip, community);
    } else if (type == "interface") {
        m_currentQueryType = 2;
        queryInterfaces(ip, community);
    } else if (type == "cpu") {
        m_currentQueryType = 3;
        queryCPU(ip, community);
    } else if (type == "memory") {
        m_currentQueryType = 4;
        queryMemory(ip, community);
    } else if (type == "temperature") {
        m_currentQueryType = 5;
        queryTemperature(ip, community);
    } else if (type == "custom") {
        m_currentQueryType = 6;
        QString oid = m_customOidEdit->text().trimmed();
        queryCustomOid(ip, community, oid);
    }
}

// ─── Build SNMP command arguments ────────────────────────────────────────────

QString SNMPMonitorWidget::buildCommunityArgs(const QString& ip, const QString& community) const
{
    QString version = m_versionCombo->currentData().toString();
    QString result;

    if (version == "3") {
        result = QString("-v 3 -u %1 -l noAuthNoPriv %2").arg(community, ip);
    } else {
        result = QString("-v %1 -c %2 %3").arg(version, community, ip);
    }

    return result;
}

// ─── Query: System Info ──────────────────────────────────────────────────────

void SNMPMonitorWidget::querySystemInfo(const QString& ip, const QString& community)
{
    clearMetricsTable(m_systemTable);

    m_systemTable->setRowCount(0);

    QStringList oids = {
        "1.3.6.1.2.1.1.1.0",   // sysDescr
        "1.3.6.1.2.1.1.5.0",   // sysName
        "1.3.6.1.2.1.1.3.0",   // sysUpTime
        "1.3.6.1.2.1.1.4.0",   // sysContact
        "1.3.6.1.2.1.1.6.0",   // sysLocation
        "1.3.6.1.2.1.1.2.0",   // sysObjectID
    };

    QStringList oidNames = {
        "系统描述 (sysDescr)",
        "系统名称 (sysName)",
        "运行时间 (sysUpTime)",
        "系统联系人 (sysContact)",
        "系统位置 (sysLocation)",
        "系统 OID (sysObjectID)",
    };

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);
    QString cmd = QString("snmpget %1 %2").arg(args, oids.join(" "));

    Logger::instance().debug("SNMP Monitor", "执行: " + cmd);

    // Store OID names for parsing
    m_process->setProperty("oidNames", QVariant::fromValue(oidNames));
    m_process->setProperty("queryType", 1);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oids;

    m_process->start("snmpget", argList);
}

// ─── Query: Interfaces ───────────────────────────────────────────────────────

void SNMPMonitorWidget::queryInterfaces(const QString& ip, const QString& community)
{
    m_interfaceTable->setRowCount(0);

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);
    QString oid = "1.3.6.1.2.1.2.2.1"; // ifTable subtree

    Logger::instance().debug("SNMP Monitor", "执行: snmpwalk " + args + " " + oid);

    m_process->setProperty("queryType", 2);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oid;

    m_process->start("snmpwalk", argList);
}

// ─── Query: CPU ──────────────────────────────────────────────────────────────

void SNMPMonitorWidget::queryCPU(const QString& ip, const QString& community)
{
    clearMetricsTable(m_cpuMemTable);
    m_cpuMemTable->setRowCount(0);

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);
    QString oid = "1.3.6.1.4.1.2021.11"; // UCD-SNMP-MIB systemStats

    Logger::instance().debug("SNMP Monitor", "执行: snmpwalk " + args + " " + oid);

    m_process->setProperty("queryType", 3);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oid;

    m_process->start("snmpwalk", argList);
}

// ─── Query: Memory ───────────────────────────────────────────────────────────

void SNMPMonitorWidget::queryMemory(const QString& ip, const QString& community)
{
    clearMetricsTable(m_cpuMemTable);
    m_cpuMemTable->setRowCount(0);

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);
    QString oid = "1.3.6.1.4.1.2021.4"; // UCD-SNMP-MIB memory

    Logger::instance().debug("SNMP Monitor", "执行: snmpwalk " + args + " " + oid);

    m_process->setProperty("queryType", 4);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oid;

    m_process->start("snmpwalk", argList);
}

// ─── Query: Temperature ──────────────────────────────────────────────────────

void SNMPMonitorWidget::queryTemperature(const QString& ip, const QString& community)
{
    clearMetricsTable(m_cpuMemTable);
    m_cpuMemTable->setRowCount(0);

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);
    QString oid = "1.3.6.1.4.1.2021.13.16.2.1.3"; // UCD-SNMP-MIB temperature sensors

    Logger::instance().debug("SNMP Monitor", "执行: snmpwalk " + args + " " + oid);

    m_process->setProperty("queryType", 5);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oid;

    m_process->start("snmpwalk", argList);
}

// ─── Query: Custom OID ───────────────────────────────────────────────────────

void SNMPMonitorWidget::queryCustomOid(const QString& ip, const QString& community, const QString& oid)
{
    clearMetricsTable(m_systemTable);
    m_systemTable->setRowCount(0);

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SNMPMonitorWidget::onProcessFinished);

    QString args = buildCommunityArgs(ip, community);

    Logger::instance().debug("SNMP Monitor", "执行: snmpwalk " + args + " " + oid);

    m_process->setProperty("queryType", 6);
    m_process->setProperty("customOid", oid);

    QStringList argList = args.split(' ', Qt::SkipEmptyParts);
    argList << oid;

    m_process->start("snmpwalk", argList);
}

// ─── Process Finished ────────────────────────────────────────────────────────

void SNMPMonitorWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (!m_process) return;

    int queryType = m_process->property("queryType").toInt();
    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromUtf8(data);

    if (exitCode != 0) {
        QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
        QString errorMsg = QString("SNMP 查询失败 (退出码: %1)").arg(exitCode);
        if (!errorOutput.isEmpty()) {
            errorMsg += "\n" + errorOutput.trimmed();
        }
        Logger::instance().error("SNMP Monitor", errorMsg);

        // Add error row to the appropriate table
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        if (queryType == 2) {
            int row = m_interfaceTable->rowCount();
            m_interfaceTable->insertRow(row);
            m_interfaceTable->setItem(row, 0, new QTableWidgetItem("查询失败"));
            m_interfaceTable->setItem(row, 1, new QTableWidgetItem("Error"));
            m_interfaceTable->setItem(row, 2, new QTableWidgetItem(errorMsg));
        } else {
            QTableWidget* targetTable = (queryType >= 3 && queryType <= 5) ? m_cpuMemTable : m_systemTable;
            addMetricRowTo(targetTable, "查询失败", errorMsg, "Error");
        }
    } else {
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        switch (queryType) {
        case 1: // System Info
            {
                // Parse snmpget output
                QStringList oidNames = m_process->property("oidNames").toStringList();
                for (int i = 0; i < lines.size() && i < oidNames.size(); ++i) {
                    QString line = lines[i].trimmed();
                    static QRegularExpression re(R"((.+?)\s*=\s*\w+\s*:\s*(.+)$)");
                    QRegularExpressionMatch match = re.match(line);
                    if (match.hasMatch()) {
                        QString value = match.captured(2).trimmed();
                        if (value.startsWith('"') && value.endsWith('"')) {
                            value = value.mid(1, value.length() - 2);
                        }
                        addMetricRowTo(m_systemTable, oidNames[i], value, "OK");
                    } else {
                        addMetricRowTo(m_systemTable, oidNames[i], line, "OK");
                    }
                }
            }
            break;

        case 2: // Interfaces
            {
                // Parse snmpwalk output for ifTable
                // Build maps: ifIndex -> {col2->value, col8->value, col10->value, col16->value}
                QMap<int, QString> ifNames;
                QMap<int, QString> ifStatus;
                QMap<int, qint64> ifInOctets;
                QMap<int, qint64> ifOutOctets;
                QMap<int, qint64> ifInPkts;
                QMap<int, qint64> ifOutPkts;
                QMap<int, qint64> ifErrors;
                QSet<int> indices;

                static QRegularExpression ifRe(
                    R"(.*\.(\d+)\s*=\s*(\w+)\s*:\s*(.+)$)"
                );

                for (const QString& line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;

                    QRegularExpressionMatch match = ifRe.match(trimmed);
                    if (!match.hasMatch()) continue;

                    int index = match.captured(1).toInt();
                    QString type = match.captured(2);
                    QString value = match.captured(3).trimmed();
                    if (value.startsWith('"') && value.endsWith('"')) {
                        value = value.mid(1, value.length() - 2);
                    }
                    indices.insert(index);

                    if (trimmed.contains(".2.") || trimmed.contains("ifDescr")) {
                        ifNames[index] = value;
                    } else if (trimmed.contains(".8.") || trimmed.contains("ifOperStatus")) {
                        ifStatus[index] = (value == "1") ? "Up" : "Down";
                    } else if (trimmed.contains(".10.") || trimmed.contains("ifInOctets")) {
                        ifInOctets[index] = value.toLongLong();
                    } else if (trimmed.contains(".16.") || trimmed.contains("ifOutOctets")) {
                        ifOutOctets[index] = value.toLongLong();
                    } else if (trimmed.contains(".11.") || trimmed.contains("ifInUcastPkts")) {
                        ifInPkts[index] = value.toLongLong();
                    } else if (trimmed.contains(".17.") || trimmed.contains("ifOutUcastPkts")) {
                        ifOutPkts[index] = value.toLongLong();
                    } else if (trimmed.contains(".14.") || trimmed.contains("ifInErrors")) {
                        ifErrors[index] = value.toLongLong();
                    }
                }

                // Calculate traffic rates if we have previous data
                QList<int> sortedIndices = indices.values();
                std::sort(sortedIndices.begin(), sortedIndices.end());

                for (int idx : sortedIndices) {
                    QString name = ifNames.value(idx, QString("接口 %1").arg(idx));
                    QString status = ifStatus.value(idx, "Unknown");
                    qint64 inOct = ifInOctets.value(idx, 0);
                    qint64 outOct = ifOutOctets.value(idx, 0);
                    qint64 inPkt = ifInPkts.value(idx, 0);
                    qint64 outPkt = ifOutPkts.value(idx, 0);
                    qint64 errVal = ifErrors.value(idx, 0);

                    QString inTraffic = formatTraffic(inOct);
                    QString outTraffic = formatTraffic(outOct);
                    QString inPktsStr = QString::number(inPkt);
                    QString outPktsStr = QString::number(outPkt);
                    QString errorsStr = QString::number(errVal);

                    addInterfaceRow(name, status, inTraffic, outTraffic,
                                    inPktsStr, outPktsStr, errorsStr);
                }

                if (sortedIndices.isEmpty()) {
                    int row = m_interfaceTable->rowCount();
                    m_interfaceTable->insertRow(row);
                    m_interfaceTable->setItem(row, 0, new QTableWidgetItem("无接口数据"));
                }
            }
            break;

        case 3: // CPU
            {
                // UCD-SNMP-MIB systemStats OIDs
                QMap<QString, QString> cpuMap;
                for (const QString& line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;
                    static QRegularExpression cpuRe(R"(.*\.(\d+)\.0\s*=\s*\w+\s*:\s*(.+)$)");
                    QRegularExpressionMatch match = cpuRe.match(trimmed);
                    if (match.hasMatch()) {
                        int subId = match.captured(1).toInt();
                        QString value = match.captured(2).trimmed();
                        if (value.startsWith('"') && value.endsWith('"')) {
                            value = value.mid(1, value.length() - 2);
                        }
                        cpuMap[QString::number(subId)] = value;
                    }
                }

                if (cpuMap.contains("9")) {
                    addMetricRowTo(m_cpuMemTable, "CPU 用户态 (%)", cpuMap["9"], "OK");
                }
                if (cpuMap.contains("10")) {
                    addMetricRowTo(m_cpuMemTable, "CPU 系统态 (%)", cpuMap["10"], "OK");
                }
                if (cpuMap.contains("11")) {
                    addMetricRowTo(m_cpuMemTable, "CPU 空闲 (%)", cpuMap["11"], "OK");
                }
                if (cpuMap.contains("1")) {
                    addMetricRowTo(m_cpuMemTable, "CPU 1分钟负载", cpuMap["1"], "OK");
                }

                if (cpuMap.isEmpty()) {
                    addMetricRowTo(m_cpuMemTable, "CPU 数据", "无数据返回", "Warning");
                }
            }
            break;

        case 4: // Memory
            {
                QMap<QString, QString> memMap;
                for (const QString& line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;
                    static QRegularExpression memRe(R"(.*\.(\d+)\.0\s*=\s*\w+\s*:\s*(.+)$)");
                    QRegularExpressionMatch match = memRe.match(trimmed);
                    if (match.hasMatch()) {
                        int subId = match.captured(1).toInt();
                        QString value = match.captured(2).trimmed();
                        if (value.startsWith('"') && value.endsWith('"')) {
                            value = value.mid(1, value.length() - 2);
                        }
                        memMap[QString::number(subId)] = value;
                    }
                }

                if (memMap.contains("5")) {
                    qint64 totalKb = memMap["5"].toLongLong();
                    addMetricRowTo(m_cpuMemTable, "内存总量",
                                   QString("%1 MB").arg(totalKb / 1024), "OK");
                }
                if (memMap.contains("6")) {
                    qint64 availKb = memMap["6"].toLongLong();
                    addMetricRowTo(m_cpuMemTable, "可用内存",
                                   QString("%1 MB").arg(availKb / 1024), "OK");
                }
                if (memMap.contains("5") && memMap.contains("6")) {
                    qint64 totalKb = memMap["5"].toLongLong();
                    qint64 availKb = memMap["6"].toLongLong();
                    if (totalKb > 0) {
                        qint64 usedKb = totalKb - availKb;
                        int usedPercent = static_cast<int>(usedKb * 100 / totalKb);
                        QString status = (usedPercent > 90) ? "Warning" : "OK";
                        addMetricRowTo(m_cpuMemTable, "内存使用率",
                                       QString("%1%").arg(usedPercent), status);
                    }
                }
                if (memMap.contains("3")) {
                    qint64 swapTotalKb = memMap["3"].toLongLong();
                    addMetricRowTo(m_cpuMemTable, "Swap 总量",
                                   QString("%1 MB").arg(swapTotalKb / 1024), "OK");
                }
                if (memMap.contains("4")) {
                    qint64 swapAvailKb = memMap["4"].toLongLong();
                    addMetricRowTo(m_cpuMemTable, "Swap 可用",
                                   QString("%1 MB").arg(swapAvailKb / 1024), "OK");
                }

                if (memMap.isEmpty()) {
                    addMetricRowTo(m_cpuMemTable, "内存数据", "无数据返回", "Warning");
                }
            }
            break;

        case 5: // Temperature
            {
                static QRegularExpression tempRe(R"(.*\.(\d+)\s*=\s*\w+\s*:\s*(.+)$)");
                bool found = false;

                for (const QString& line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;
                    QRegularExpressionMatch match = tempRe.match(trimmed);
                    if (match.hasMatch()) {
                        int sensorIndex = match.captured(1).toInt();
                        QString value = match.captured(2).trimmed();
                        if (value.startsWith('"') && value.endsWith('"')) {
                            value = value.mid(1, value.length() - 2);
                        }
                        QString status = "OK";
                        int tempVal = value.toInt();
                        if (tempVal > 80) status = "Warning";
                        if (tempVal > 95) status = "Critical";
                        addMetricRowTo(m_cpuMemTable,
                                       QString("温度传感器 #%1").arg(sensorIndex),
                                       QString("%1 °C").arg(value), status);
                        found = true;
                    }
                }

                if (!found) {
                    addMetricRowTo(m_cpuMemTable, "温度数据", "无数据返回", "Warning");
                }
            }
            break;

        case 6: // Custom OID
            {
                QString customOid = m_process->property("customOid").toString();
                for (const QString& line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;
                    static QRegularExpression customRe(R"((.+?)\s*=\s*\w+\s*:\s*(.+)$)");
                    QRegularExpressionMatch match = customRe.match(trimmed);
                    if (match.hasMatch()) {
                        QString oidStr = match.captured(1).trimmed();
                        QString value = match.captured(2).trimmed();
                        if (value.startsWith('"') && value.endsWith('"')) {
                            value = value.mid(1, value.length() - 2);
                        }
                        addMetricRowTo(m_systemTable, oidStr, value, "OK");
                    } else {
                        addMetricRowTo(m_systemTable, customOid, trimmed, "OK");
                    }
                }

                if (lines.isEmpty()) {
                    addMetricRowTo(m_systemTable, customOid, "无数据返回", "Warning");
                }
            }
            break;
        }
    }

    m_process->deleteLater();
    m_process = nullptr;

    Logger::instance().info("SNMP Monitor",
        QString("查询完成 (类型: %1, 退出码: %2)").arg(queryType).arg(exitCode));
}

// ─── Add Metric Row ──────────────────────────────────────────────────────────

void SNMPMonitorWidget::addMetricRow(const QString& metric, const QString& value, const QString& status)
{
    QTableWidget* targetTable = m_systemTable;
    // Determine target table based on current query type
    int queryType = m_currentQueryType;
    if (queryType >= 3 && queryType <= 5) {
        targetTable = m_cpuMemTable;
    }
    addMetricRowTo(targetTable, metric, value, status);
}

void SNMPMonitorWidget::addMetricRowTo(QTableWidget* table, const QString& metric,
                                        const QString& value, const QString& status)
{
    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row, 0, new QTableWidgetItem(metric));
    table->setItem(row, 1, new QTableWidgetItem(value));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "OK") {
        statusItem->setForeground(QColor("#67C23A"));
    } else if (status == "Warning") {
        statusItem->setForeground(QColor("#E6A23C"));
    } else if (status == "Critical" || status == "Error") {
        statusItem->setForeground(QColor("#F56C6C"));
    }
    table->setItem(row, 2, statusItem);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    table->setItem(row, 3, new QTableWidgetItem(timestamp));
}

// ─── Add Interface Row ───────────────────────────────────────────────────────

void SNMPMonitorWidget::addInterfaceRow(const QString& name, const QString& status,
                                         const QString& inTraffic, const QString& outTraffic,
                                         const QString& inPkts, const QString& outPkts,
                                         const QString& errors)
{
    int row = m_interfaceTable->rowCount();
    m_interfaceTable->insertRow(row);

    m_interfaceTable->setItem(row, 0, new QTableWidgetItem(name));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "Up") {
        statusItem->setForeground(QColor("#67C23A"));
    } else {
        statusItem->setForeground(QColor("#F56C6C"));
    }
    m_interfaceTable->setItem(row, 1, statusItem);

    m_interfaceTable->setItem(row, 2, new QTableWidgetItem(inTraffic));
    m_interfaceTable->setItem(row, 3, new QTableWidgetItem(outTraffic));
    m_interfaceTable->setItem(row, 4, new QTableWidgetItem(inPkts));
    m_interfaceTable->setItem(row, 5, new QTableWidgetItem(outPkts));
    m_interfaceTable->setItem(row, 6, new QTableWidgetItem(errors));
}

// ─── Clear Metrics Table ─────────────────────────────────────────────────────

void SNMPMonitorWidget::clearMetricsTable(QTableWidget* table)
{
    table->setRowCount(0);
}

// ─── Format Traffic ──────────────────────────────────────────────────────────

QString SNMPMonitorWidget::formatTraffic(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024);
    } else if (bytes < 1024LL * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024 * 1024));
    } else {
        return QString("%1 GB").arg(bytes / (1024LL * 1024 * 1024));
    }
}

// ─── Parse SNMP Line ─────────────────────────────────────────────────────────

void SNMPMonitorWidget::parseSnmpLine(const QString& line, QMap<QString, QString>& resultMap)
{
    static QRegularExpression re(R"((.+?)\s*=\s*\w+\s*:\s*(.+)$)");
    QRegularExpressionMatch match = re.match(line.trimmed());
    if (match.hasMatch()) {
        QString oid = match.captured(1).trimmed();
        QString value = match.captured(2).trimmed();
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        resultMap[oid] = value;
    }
}

// ─── Extract Last OID Component ──────────────────────────────────────────────

int SNMPMonitorWidget::extractLastOidComponent(const QString& oidStr) const
{
    QStringList parts = oidStr.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return 0;
    return parts.last().toInt();
}

// ─── Export ──────────────────────────────────────────────────────────────────

void SNMPMonitorWidget::onExport()
{
    // Determine which table to export based on current tab
    QTableWidget* currentTable = nullptr;
    QString tabName;
    int tabIndex = m_tabWidget->currentIndex();

    if (tabIndex == 0) {
        currentTable = m_systemTable;
        tabName = "系统信息";
    } else if (tabIndex == 1) {
        currentTable = m_interfaceTable;
        tabName = "接口流量";
    } else {
        currentTable = m_cpuMemTable;
        tabName = "CPU内存";
    }

    if (!currentTable || currentTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出监控数据",
        QString("snmp_monitor_%1_%2.csv")
            .arg(tabName)
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("SNMP Monitor", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // Write header
    QStringList headers;
    for (int col = 0; col < currentTable->columnCount(); ++col) {
        auto* headerItem = currentTable->horizontalHeaderItem(col);
        headers << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << headers.join(",") << "\n";

    // Write data
    for (int row = 0; row < currentTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < currentTable->columnCount(); ++col) {
            auto* item = currentTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    file.close();
    Logger::instance().info("SNMP Monitor",
        QString("导出 %1 条记录到: %2").arg(currentTable->rowCount()).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(currentTable->rowCount()).arg(filePath));
}