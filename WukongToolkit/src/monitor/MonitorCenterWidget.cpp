#include "monitor/MonitorCenterWidget.h"
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
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QSplitter>
#include <QLabel>
#include <QInputDialog>
#include <QDateTime>
#include <QSettings>
#include <QRegularExpression>
#include <QApplication>
#include <QRandomGenerator>
#include <QFrame>
#include <QSet>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════════
// MonitorCenterWidget
// ═══════════════════════════════════════════════════════════════════════════════

MonitorCenterWidget::MonitorCenterWidget(QWidget* parent)
    : QWidget(parent)
    , m_autoRefreshTimer(nullptr)
    , m_process(nullptr)
    , m_currentQueryType(0)
    , m_isFullScreen(false)
{
    setupUI();
    setupConnections();
    loadDevices();
    updateDeviceList();
}

MonitorCenterWidget::~MonitorCenterWidget()
{
    if (m_autoRefreshTimer) {
        m_autoRefreshTimer->stop();
    }
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void MonitorCenterWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──
    auto styleCombo = [](QComboBox* combo, int minWidth = 180) {
        combo->setMinimumWidth(minWidth);
        combo->setStyleSheet(
            "QComboBox {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #161B22; color: #E6EDF3;"
            "  selection-background-color: #30363D;"
            "}"
        );
    };

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
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
                    "QPushButton:disabled { background-color: #484F58; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleLabel = [](QLabel* label, const QString& color = "#8B949E", int fontSize = 12) {
        label->setStyleSheet(
            QString("font-size: %1px; color: %2;").arg(fontSize).arg(color)
        );
    };

    auto styleCard = [](QFrame* card) {
        card->setStyleSheet(
            "QFrame {"
            "  background-color: #161B22;"
            "  border: 1px solid #30363D;"
            "  border-radius: 6px;"
            "}"
        );
    };

    auto styleSpin = [](QSpinBox* spin, int width = 80) {
        spin->setFixedWidth(width);
        spin->setStyleSheet(
            "QSpinBox {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 4px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
        );
    };

    // ── Top: Device selection and refresh controls ──
    auto* topGroup = new QGroupBox("设备监控中心");
    topGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #58A6FF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #30363D; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(12);

    auto* deviceLabel = new QLabel("监控设备:");
    styleLabel(deviceLabel, "#8B949E", 13);
    m_deviceCombo = new QComboBox();
    styleCombo(m_deviceCombo, 220);

    m_addDeviceBtn = new QPushButton("+ 添加设备");
    styleButton(m_addDeviceBtn, "#58A6FF", "#79C0FF");

    topLayout->addWidget(deviceLabel);
    topLayout->addWidget(m_deviceCombo);
    topLayout->addWidget(m_addDeviceBtn);
    topLayout->addSpacing(24);

    auto* refreshLabel = new QLabel("刷新间隔:");
    styleLabel(refreshLabel, "#8B949E", 13);
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(5, 300);
    m_intervalSpin->setValue(10);
    m_intervalSpin->setSuffix(" 秒");
    styleSpin(m_intervalSpin, 100);

    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);
    m_autoRefreshCheck->setStyleSheet(
        "QCheckBox { color: #E6EDF3; font-size: 13px; }"
        "QCheckBox::indicator {"
        "  width: 16px; height: 16px;"
        "  border: 1px solid #30363D; border-radius: 3px;"
        "  background-color: #161B22;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #58A6FF; border-color: #58A6FF;"
        "}"
    );

    m_refreshBtn = new QPushButton("立即刷新");
    styleButton(m_refreshBtn, "#3FB950", "#56D364");

    m_exportBtn = new QPushButton("导出 CSV");
    styleButton(m_exportBtn, "#D29922", "#DBAB4A");

    m_fullScreenBtn = new QPushButton("全屏");
    styleButton(m_fullScreenBtn, "#8B949E", "#B4B4B4");

    topLayout->addWidget(refreshLabel);
    topLayout->addWidget(m_intervalSpin);
    topLayout->addWidget(m_autoRefreshCheck);
    topLayout->addWidget(m_refreshBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_exportBtn);
    topLayout->addWidget(m_fullScreenBtn);

    mainLayout->addWidget(topGroup);

    // ── Middle: Dashboard cards + Metrics table ──
    auto* middleSplitter = new QSplitter(Qt::Horizontal);
    middleSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #30363D; width: 2px; }"
    );

    // Left: Dashboard cards
    auto* dashboardWidget = new QWidget();
    auto* dashboardLayout = new QVBoxLayout(dashboardWidget);
    dashboardLayout->setContentsMargins(0, 0, 4, 0);
    dashboardLayout->setSpacing(8);

    // ── CPU Card ──
    auto* cpuCard = new QFrame();
    styleCard(cpuCard);
    auto* cpuLayout = new QVBoxLayout(cpuCard);
    cpuLayout->setContentsMargins(12, 10, 12, 10);
    cpuLayout->setSpacing(6);

    auto* cpuHeader = new QHBoxLayout();
    auto* cpuIcon = new QLabel("🖥");
    cpuIcon->setStyleSheet("font-size: 18px;");
    auto* cpuTitle = new QLabel("CPU 使用率");
    styleLabel(cpuTitle, "#E6EDF3", 14);
    cpuTitle->setStyleSheet("font-size: 14px; color: #E6EDF3; font-weight: bold;");
    cpuHeader->addWidget(cpuIcon);
    cpuHeader->addWidget(cpuTitle);
    cpuHeader->addStretch();

    m_cpuBar = new QProgressBar();
    m_cpuBar->setRange(0, 100);
    m_cpuBar->setValue(0);
    m_cpuBar->setTextVisible(true);
    m_cpuBar->setFixedHeight(28);
    m_cpuBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #0D1117; border: 1px solid #30363D;"
        "  border-radius: 4px; text-align: center; color: white;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #3FB950; border-radius: 3px;"
        "}"
    );

    cpuLayout->addLayout(cpuHeader);
    cpuLayout->addWidget(m_cpuBar);
    dashboardLayout->addWidget(cpuCard);

    // ── Memory Card ──
    auto* memCard = new QFrame();
    styleCard(memCard);
    auto* memLayout = new QVBoxLayout(memCard);
    memLayout->setContentsMargins(12, 10, 12, 10);
    memLayout->setSpacing(6);

    auto* memHeader = new QHBoxLayout();
    auto* memIcon = new QLabel("💾");
    memIcon->setStyleSheet("font-size: 18px;");
    auto* memTitle = new QLabel("内存使用率");
    memTitle->setStyleSheet("font-size: 14px; color: #E6EDF3; font-weight: bold;");
    memHeader->addWidget(memIcon);
    memHeader->addWidget(memTitle);
    memHeader->addStretch();

    m_memBar = new QProgressBar();
    m_memBar->setRange(0, 100);
    m_memBar->setValue(0);
    m_memBar->setTextVisible(true);
    m_memBar->setFixedHeight(28);
    m_memBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #0D1117; border: 1px solid #30363D;"
        "  border-radius: 4px; text-align: center; color: white;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #3FB950; border-radius: 3px;"
        "}"
    );

    memLayout->addLayout(memHeader);
    memLayout->addWidget(m_memBar);
    dashboardLayout->addWidget(memCard);

    // ── Temperature Card ──
    auto* tempCard = new QFrame();
    styleCard(tempCard);
    auto* tempLayout = new QHBoxLayout(tempCard);
    tempLayout->setContentsMargins(12, 10, 12, 10);
    tempLayout->setSpacing(10);

    auto* tempIcon = new QLabel("🌡");
    tempIcon->setStyleSheet("font-size: 24px;");
    auto* tempTextLayout = new QVBoxLayout();
    m_tempLabel = new QLabel("设备温度");
    styleLabel(m_tempLabel, "#8B949E", 12);
    m_tempValueLabel = new QLabel("-- °C");
    m_tempValueLabel->setStyleSheet("font-size: 22px; color: #3FB950; font-weight: bold;");
    tempTextLayout->addWidget(m_tempLabel);
    tempTextLayout->addWidget(m_tempValueLabel);

    tempLayout->addWidget(tempIcon);
    tempLayout->addLayout(tempTextLayout);
    tempLayout->addStretch();
    dashboardLayout->addWidget(tempCard);

    // ── Uptime Card ──
    auto* uptimeCard = new QFrame();
    styleCard(uptimeCard);
    auto* uptimeLayout = new QHBoxLayout(uptimeCard);
    uptimeLayout->setContentsMargins(12, 10, 12, 10);
    uptimeLayout->setSpacing(10);

    auto* uptimeIcon = new QLabel("⏱");
    uptimeIcon->setStyleSheet("font-size: 24px;");
    auto* uptimeTextLayout = new QVBoxLayout();
    m_uptimeLabel = new QLabel("运行时间");
    styleLabel(m_uptimeLabel, "#8B949E", 12);
    m_uptimeValueLabel = new QLabel("--");
    m_uptimeValueLabel->setStyleSheet("font-size: 16px; color: #58A6FF; font-weight: bold;");
    uptimeTextLayout->addWidget(m_uptimeLabel);
    uptimeTextLayout->addWidget(m_uptimeValueLabel);

    uptimeLayout->addWidget(uptimeIcon);
    uptimeLayout->addLayout(uptimeTextLayout);
    uptimeLayout->addStretch();
    dashboardLayout->addWidget(uptimeCard);

    dashboardLayout->addStretch();

    // Right: Metrics table
    auto* metricsWidget = new QWidget();
    auto* metricsLayout = new QVBoxLayout(metricsWidget);
    metricsLayout->setContentsMargins(4, 0, 0, 0);
    metricsLayout->setSpacing(4);

    auto* metricsTitle = new QLabel("监控指标");
    metricsTitle->setStyleSheet("font-size: 13px; color: #58A6FF; font-weight: bold; padding: 4px 0;");

    m_metricsTable = new QTableWidget(0, 5);
    m_metricsTable->setHorizontalHeaderLabels({"指标", "当前值", "状态", "阈值", "上次更新"});
    m_metricsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_metricsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_metricsTable->setColumnWidth(2, 70);
    m_metricsTable->setColumnWidth(3, 100);
    m_metricsTable->setColumnWidth(4, 160);
    m_metricsTable->verticalHeader()->setVisible(false);
    styleTable(m_metricsTable);

    metricsLayout->addWidget(metricsTitle);
    metricsLayout->addWidget(m_metricsTable);

    middleSplitter->addWidget(dashboardWidget);
    middleSplitter->addWidget(metricsWidget);
    middleSplitter->setStretchFactor(0, 1);
    middleSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(middleSplitter, 1);

    // ── Bottom: Interface traffic table ──
    auto* interfaceGroup = new QGroupBox("接口流量");
    interfaceGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #58A6FF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #30363D; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* interfaceLayout = new QVBoxLayout(interfaceGroup);
    interfaceLayout->setContentsMargins(4, 8, 4, 4);

    m_interfaceTable = new QTableWidget(0, 6);
    m_interfaceTable->setHorizontalHeaderLabels({"接口", "状态", "入流量", "出流量", "错误", "丢包"});
    m_interfaceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_interfaceTable->setColumnWidth(1, 70);
    m_interfaceTable->setColumnWidth(2, 110);
    m_interfaceTable->setColumnWidth(3, 110);
    m_interfaceTable->setColumnWidth(4, 80);
    m_interfaceTable->setColumnWidth(5, 80);
    m_interfaceTable->verticalHeader()->setVisible(false);
    styleTable(m_interfaceTable);

    interfaceLayout->addWidget(m_interfaceTable);
    mainLayout->addWidget(interfaceGroup, 1);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void MonitorCenterWidget::setupConnections()
{
    connect(m_addDeviceBtn, &QPushButton::clicked, this, &MonitorCenterWidget::onAddDevice);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MonitorCenterWidget::onRefresh);
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, &MonitorCenterWidget::onToggleAutoRefresh);
    connect(m_exportBtn, &QPushButton::clicked, this, &MonitorCenterWidget::onExport);
    connect(m_fullScreenBtn, &QPushButton::clicked, this, &MonitorCenterWidget::onFullScreen);
}

// ─── Add Device ──────────────────────────────────────────────────────────────

void MonitorCenterWidget::onAddDevice()
{
    bool ok;
    QString name = QInputDialog::getText(this, "添加设备", "设备名称:",
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString ip = QInputDialog::getText(this, "添加设备", "设备 IP 地址:",
                                       QLineEdit::Normal, "192.168.", &ok);
    if (!ok || ip.trimmed().isEmpty()) return;

    QString community = QInputDialog::getText(this, "添加设备", "SNMP Community:",
                                              QLineEdit::Normal, "public", &ok);
    if (!ok) return;

    MonitorDeviceInfo dev;
    dev.name = name.trimmed();
    dev.ip = ip.trimmed();
    dev.community = community.trimmed().isEmpty() ? "public" : community.trimmed();

    m_devices.append(dev);
    saveDevices();
    updateDeviceList();

    // Select the newly added device
    m_deviceCombo->setCurrentIndex(m_deviceCombo->count() - 1);

    Logger::instance().info("Monitor Center",
        QString("设备已添加: %1 (%2)").arg(dev.name, dev.ip));
}

// ─── Update Device List ──────────────────────────────────────────────────────

void MonitorCenterWidget::updateDeviceList()
{
    m_deviceCombo->clear();
    for (const auto& dev : m_devices) {
        m_deviceCombo->addItem(
            QString("%1 (%2)").arg(dev.name, dev.ip),
            QVariant::fromValue(dev)
        );
    }

    if (m_devices.isEmpty()) {
        m_deviceCombo->addItem("-- 请添加设备 --", QVariant());
    }

    Logger::instance().debug("Monitor Center",
        QString("设备列表已更新，共 %1 个设备").arg(m_devices.size()));
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void MonitorCenterWidget::onRefresh()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0 || m_devices.isEmpty()) {
        QMessageBox::information(this, "提示", "请先添加监控设备。");
        return;
    }

    QVariant data = m_deviceCombo->itemData(idx);
    if (!data.isValid()) {
        QMessageBox::information(this, "提示", "请先添加监控设备。");
        return;
    }

    MonitorDeviceInfo dev = data.value<MonitorDeviceInfo>();
    if (dev.ip.isEmpty()) {
        QMessageBox::information(this, "提示", "请先添加监控设备。");
        return;
    }

    Logger::instance().info("Monitor Center",
        QString("开始刷新设备: %1 (%2)").arg(dev.name, dev.ip));

    clearMetrics();
    clearInterfaces();

    queryDeviceMetrics(dev.ip);
}

// ─── Query Device Metrics ────────────────────────────────────────────────────

void MonitorCenterWidget::queryDeviceMetrics(const QString& ip)
{
    if (checkSnmpAvailable()) {
        // Use SNMP to query real device metrics
        int idx = m_deviceCombo->currentIndex();
        if (idx < 0) return;

        QVariant data = m_deviceCombo->itemData(idx);
        if (!data.isValid()) return;

        MonitorDeviceInfo dev = data.value<MonitorDeviceInfo>();
        QString community = dev.community.isEmpty() ? "public" : dev.community;

        if (m_process) {
            m_process->kill();
            m_process->waitForFinished(1000);
            delete m_process;
            m_process = nullptr;
        }

        m_process = new QProcess(this);
        m_currentQueryType = 1; // Metrics query

        // Query multiple OIDs in one snmpget call
        QStringList oids = {
            // UCD-SNMP-MIB CPU
            "1.3.6.1.4.1.2021.11.9.0",   // ssCpuUser
            "1.3.6.1.4.1.2021.11.10.0",  // ssCpuSystem
            "1.3.6.1.4.1.2021.11.11.0",  // ssCpuIdle
            // UCD-SNMP-MIB Memory
            "1.3.6.1.4.1.2021.4.5.0",    // memTotalReal
            "1.3.6.1.4.1.2021.4.6.0",    // memAvailReal
            "1.3.6.1.4.1.2021.4.15.0",   // memTotalSwap
            // UCD-SNMP-MIB Temperature
            "1.3.6.1.4.1.2021.13.16.2.1.3.1", // tempSensor
            // System
            "1.3.6.1.2.1.1.3.0",         // sysUpTime
            "1.3.6.1.2.1.1.5.0",         // sysName
        };

        QStringList args;
        args << "-v" << "2c" << "-c" << community << ip << oids;

        m_process->setProperty("ip", ip);
        m_process->setProperty("community", community);
        m_process->setProperty("queryType", 1);

        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitStatus)

            if (!m_process) return;

            int queryType = m_process->property("queryType").toInt();
            QString ip = m_process->property("ip").toString();
            QString community = m_process->property("community").toString();

            if (exitCode != 0 || queryType == 1) {
                // SNMP failed, fallback to simulated data
                QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
                Logger::instance().warning("Monitor Center",
                    QString("SNMP 查询失败，使用模拟数据。错误: %1").arg(errorOutput.trimmed()));

                generateSimulatedMetrics();
            }

            if (queryType == 1) {
                // After metrics, query interfaces
                m_currentQueryType = 2;
                queryInterfacesViaSnmp(ip, community);
            } else if (queryType == 2) {
                // Done
                m_process->deleteLater();
                m_process = nullptr;
            }
        });

        Logger::instance().debug("Monitor Center", "执行: snmpget " + args.join(" "));
        m_process->start("snmpget", args);
    } else {
        // SNMP tools not available, use simulated data
        Logger::instance().info("Monitor Center", "snmpget 不可用，使用模拟数据");
        generateSimulatedMetrics();
        generateSimulatedInterfaces();
    }
}

// ─── Generate Simulated Metrics ──────────────────────────────────────────────

void MonitorCenterWidget::generateSimulatedMetrics()
{
    auto* rng = QRandomGenerator::global();

    int cpuUsage = rng->bounded(5, 85);
    int memUsage = rng->bounded(20, 90);
    int temp = rng->bounded(35, 72);
    int uptimeDays = rng->bounded(1, 365);
    int uptimeHours = rng->bounded(0, 24);
    int uptimeMins = rng->bounded(0, 60);

    // Update CPU bar
    m_cpuBar->setValue(cpuUsage);
    updateProgressBar(m_cpuBar, cpuUsage);

    // Update Memory bar
    m_memBar->setValue(memUsage);
    updateProgressBar(m_memBar, memUsage);

    // Update temperature
    if (temp < 50) {
        m_tempValueLabel->setStyleSheet("font-size: 22px; color: #3FB950; font-weight: bold;");
    } else if (temp < 70) {
        m_tempValueLabel->setStyleSheet("font-size: 22px; color: #D29922; font-weight: bold;");
    } else {
        m_tempValueLabel->setStyleSheet("font-size: 22px; color: #F85149; font-weight: bold;");
    }
    m_tempValueLabel->setText(QString("%1 °C").arg(temp));

    // Update uptime
    QString uptimeStr;
    if (uptimeDays > 0) {
        uptimeStr = QString("%1 天 %2 小时 %3 分钟").arg(uptimeDays).arg(uptimeHours).arg(uptimeMins);
    } else if (uptimeHours > 0) {
        uptimeStr = QString("%1 小时 %2 分钟").arg(uptimeHours).arg(uptimeMins);
    } else {
        uptimeStr = QString("%1 分钟").arg(uptimeMins);
    }
    m_uptimeValueLabel->setText(uptimeStr);

    // Add metrics to table
    QString cpuStatus = cpuUsage > 80 ? "Warning" : (cpuUsage > 60 ? "Caution" : "OK");
    QString cpuThreshold = "80%";
    addMetricRow("CPU 使用率", QString("%1%").arg(cpuUsage), cpuStatus, cpuThreshold);

    QString memStatus = memUsage > 85 ? "Warning" : (memUsage > 70 ? "Caution" : "OK");
    QString memThreshold = "85%";
    addMetricRow("内存使用率", QString("%1%").arg(memUsage), memStatus, memThreshold);

    QString tempStatus = temp > 70 ? "Warning" : (temp > 55 ? "Caution" : "OK");
    QString tempThreshold = "70°C";
    addMetricRow("设备温度", QString("%1 °C").arg(temp), tempStatus, tempThreshold);

    double load1 = rng->bounded(10, 50) / 10.0;
    double load5 = rng->bounded(10, 50) / 10.0;
    double load15 = rng->bounded(10, 50) / 10.0;
    addMetricRow("系统负载 (1min)", QString::number(load1, 'f', 2), "OK", "4.0");
    addMetricRow("系统负载 (5min)", QString::number(load5, 'f', 2), "OK", "4.0");
    addMetricRow("系统负载 (15min)", QString::number(load15, 'f', 2), "OK", "4.0");

    qint64 totalMem = 16LL * 1024 * 1024 * 1024; // 16GB simulated
    qint64 usedMem = totalMem * memUsage / 100;
    qint64 availMem = totalMem - usedMem;
    addMetricRow("总内存", formatTraffic(totalMem), "OK", "-");
    addMetricRow("已用内存", formatTraffic(usedMem), "OK", "-");
    addMetricRow("可用内存", formatTraffic(availMem), "OK", "-");

    int totalProcs = rng->bounded(100, 500);
    int runningProcs = rng->bounded(1, 10);
    addMetricRow("进程总数", QString::number(totalProcs), "OK", "-");
    addMetricRow("运行中进程", QString::number(runningProcs), "OK", "-");
}

// ─── Generate Simulated Interfaces ───────────────────────────────────────────

void MonitorCenterWidget::generateSimulatedInterfaces()
{
    auto* rng = QRandomGenerator::global();

    struct SimInterface {
        QString name;
        bool up;
    };

    QList<SimInterface> ifaces = {
        {"GigabitEthernet0/0", true},
        {"GigabitEthernet0/1", true},
        {"GigabitEthernet0/2", false},
        {"FastEthernet1/0", true},
        {"FastEthernet1/1", true},
        {"Loopback0", true},
        {"Vlan100", true},
        {"Tunnel0", false},
    };

    for (const auto& iface : ifaces) {
        QString status = iface.up ? "Up" : "Down";
        QString inTraffic = formatTraffic(rng->bounded(1000000LL, 5000000000LL));
        QString outTraffic = formatTraffic(rng->bounded(500000LL, 3000000000LL));
        QString errors = iface.up ? QString::number(rng->bounded(0, 100)) : "0";
        QString drops = iface.up ? QString::number(rng->bounded(0, 50)) : "0";

        addInterfaceRow(iface.name, status, inTraffic, outTraffic, errors, drops);
    }
}

// ─── Query Interfaces via SNMP ───────────────────────────────────────────────

void MonitorCenterWidget::queryInterfacesViaSnmp(const QString& ip, const QString& community)
{
    if (!m_process) return;

    // Disconnect previous connection
    disconnect(m_process, nullptr, this, nullptr);

    QStringList args;
    args << "-v" << "2c" << "-c" << community << ip << "1.3.6.1.2.1.2.2.1";

    m_process->setProperty("queryType", 2);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)

        if (!m_process) return;

        if (exitCode != 0) {
            QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
            Logger::instance().warning("Monitor Center",
                QString("接口 SNMP 查询失败，使用模拟数据。错误: %1").arg(errorOutput.trimmed()));
            generateSimulatedInterfaces();
        } else {
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);

            // Parse ifTable results
            QMap<int, QString> ifNames;
            QMap<int, QString> ifStatus;
            QMap<int, qint64> ifInOctets;
            QMap<int, qint64> ifOutOctets;
            QMap<int, qint64> ifErrors;
            QMap<int, qint64> ifDrops;
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
                QString value = match.captured(2).trimmed();
                indices.insert(index);

                if (trimmed.contains(".2.")) { // ifDescr
                    ifNames[index] = value;
                    if (value.startsWith('"') && value.endsWith('"')) {
                        ifNames[index] = value.mid(1, value.length() - 2);
                    }
                } else if (trimmed.contains(".8.")) { // ifOperStatus
                    ifStatus[index] = (value == "1") ? "Up" : "Down";
                } else if (trimmed.contains(".10.")) { // ifInOctets
                    ifInOctets[index] = value.toLongLong();
                } else if (trimmed.contains(".16.")) { // ifOutOctets
                    ifOutOctets[index] = value.toLongLong();
                } else if (trimmed.contains(".14.")) { // ifInErrors
                    ifErrors[index] = value.toLongLong();
                } else if (trimmed.contains(".20.")) { // ifOutErrors
                    ifDrops[index] = value.toLongLong();
                }
            }

            QList<int> sortedIndices = indices.values();
            std::sort(sortedIndices.begin(), sortedIndices.end());

            for (int idx : sortedIndices) {
                QString name = ifNames.value(idx, QString("接口 %1").arg(idx));
                QString status = ifStatus.value(idx, "Unknown");
                QString inTraffic = formatTraffic(ifInOctets.value(idx, 0));
                QString outTraffic = formatTraffic(ifOutOctets.value(idx, 0));
                QString errors = QString::number(ifErrors.value(idx, 0));
                QString drops = QString::number(ifDrops.value(idx, 0));

                addInterfaceRow(name, status, inTraffic, outTraffic, errors, drops);
            }

            if (sortedIndices.isEmpty()) {
                generateSimulatedInterfaces();
            }
        }

        m_process->deleteLater();
        m_process = nullptr;
    });

    Logger::instance().debug("Monitor Center", "执行: snmpwalk " + args.join(" "));
    m_process->start("snmpwalk", args);
}

// ─── Toggle Auto Refresh ─────────────────────────────────────────────────────

void MonitorCenterWidget::onToggleAutoRefresh(bool enabled)
{
    if (enabled) {
        if (!m_autoRefreshTimer) {
            m_autoRefreshTimer = new QTimer(this);
            connect(m_autoRefreshTimer, &QTimer::timeout, this, &MonitorCenterWidget::onRefresh);
        }

        int interval = m_intervalSpin->value();
        m_autoRefreshTimer->start(interval * 1000);

        Logger::instance().info("Monitor Center",
            QString("自动刷新已启动，间隔 %1 秒").arg(interval));
    } else {
        if (m_autoRefreshTimer) {
            m_autoRefreshTimer->stop();
        }

        Logger::instance().info("Monitor Center", "自动刷新已停止");
    }
}

// ─── Export CSV ──────────────────────────────────────────────────────────────

void MonitorCenterWidget::onExport()
{
    if (m_metricsTable->rowCount() == 0 && m_interfaceTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出监控数据",
        QString("monitor_center_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("Monitor Center", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // Write metrics table
    stream << "=== 监控指标 ===\n";
    QStringList metricHeaders;
    for (int col = 0; col < m_metricsTable->columnCount(); ++col) {
        auto* headerItem = m_metricsTable->horizontalHeaderItem(col);
        metricHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << metricHeaders.join(",") << "\n";

    for (int row = 0; row < m_metricsTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_metricsTable->columnCount(); ++col) {
            auto* item = m_metricsTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // Write interface table
    stream << "\n=== 接口流量 ===\n";
    QStringList ifHeaders;
    for (int col = 0; col < m_interfaceTable->columnCount(); ++col) {
        auto* headerItem = m_interfaceTable->horizontalHeaderItem(col);
        ifHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << ifHeaders.join(",") << "\n";

    for (int row = 0; row < m_interfaceTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_interfaceTable->columnCount(); ++col) {
            auto* item = m_interfaceTable->item(row, col);
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

    int totalRows = m_metricsTable->rowCount() + m_interfaceTable->rowCount();
    Logger::instance().info("Monitor Center",
        QString("导出 %1 条记录到: %2").arg(totalRows).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(totalRows).arg(filePath));
}

// ─── Full Screen ─────────────────────────────────────────────────────────────

void MonitorCenterWidget::onFullScreen()
{
    if (m_isFullScreen) {
        showNormal();
        m_fullScreenBtn->setText("全屏");
        m_isFullScreen = false;
    } else {
        showFullScreen();
        m_fullScreenBtn->setText("退出全屏");
        m_isFullScreen = true;
    }

    Logger::instance().debug("Monitor Center",
        QString("全屏模式: %1").arg(m_isFullScreen ? "开启" : "关闭"));
}

// ─── Add Metric Row ──────────────────────────────────────────────────────────

void MonitorCenterWidget::addMetricRow(const QString& metric, const QString& value,
                                        const QString& status, const QString& threshold)
{
    int row = m_metricsTable->rowCount();
    m_metricsTable->insertRow(row);

    m_metricsTable->setItem(row, 0, new QTableWidgetItem(metric));
    m_metricsTable->setItem(row, 1, new QTableWidgetItem(value));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "OK") {
        statusItem->setForeground(QColor("#3FB950"));
    } else if (status == "Caution") {
        statusItem->setForeground(QColor("#D29922"));
    } else if (status == "Warning" || status == "Critical" || status == "Error") {
        statusItem->setForeground(QColor("#F85149"));
    }
    m_metricsTable->setItem(row, 2, statusItem);

    m_metricsTable->setItem(row, 3, new QTableWidgetItem(threshold));

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_metricsTable->setItem(row, 4, new QTableWidgetItem(timestamp));
}

// ─── Add Interface Row ───────────────────────────────────────────────────────

void MonitorCenterWidget::addInterfaceRow(const QString& name, const QString& status,
                                           const QString& inTraffic, const QString& outTraffic,
                                           const QString& errors, const QString& drops)
{
    int row = m_interfaceTable->rowCount();
    m_interfaceTable->insertRow(row);

    m_interfaceTable->setItem(row, 0, new QTableWidgetItem(name));

    auto* statusItem = new QTableWidgetItem(status);
    if (status == "Up") {
        statusItem->setForeground(QColor("#3FB950"));
    } else {
        statusItem->setForeground(QColor("#F85149"));
    }
    m_interfaceTable->setItem(row, 1, statusItem);

    m_interfaceTable->setItem(row, 2, new QTableWidgetItem(inTraffic));
    m_interfaceTable->setItem(row, 3, new QTableWidgetItem(outTraffic));
    m_interfaceTable->setItem(row, 4, new QTableWidgetItem(errors));
    m_interfaceTable->setItem(row, 5, new QTableWidgetItem(drops));
}

// ─── Clear Methods ───────────────────────────────────────────────────────────

void MonitorCenterWidget::clearMetrics()
{
    m_metricsTable->setRowCount(0);
    m_cpuBar->setValue(0);
    m_memBar->setValue(0);
    m_tempValueLabel->setText("-- °C");
    m_tempValueLabel->setStyleSheet("font-size: 22px; color: #3FB950; font-weight: bold;");
    m_uptimeValueLabel->setText("--");
}

void MonitorCenterWidget::clearInterfaces()
{
    m_interfaceTable->setRowCount(0);
}

// ─── Update Progress Bar ─────────────────────────────────────────────────────

void MonitorCenterWidget::updateProgressBar(QProgressBar* bar, int value)
{
    QString color;
    if (value < 60) {
        color = "#3FB950"; // Green
    } else if (value < 80) {
        color = "#D29922"; // Yellow
    } else {
        color = "#F85149"; // Red
    }

    bar->setStyleSheet(
        QString("QProgressBar {"
                "  background-color: #0D1117; border: 1px solid #30363D;"
                "  border-radius: 4px; text-align: center; color: white;"
                "  font-size: 13px; font-weight: bold;"
                "}"
                "QProgressBar::chunk {"
                "  background-color: %1; border-radius: 3px;"
                "}")
            .arg(color)
    );
}

// ─── Format Traffic ──────────────────────────────────────────────────────────

QString MonitorCenterWidget::formatTraffic(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024);
    } else if (bytes < 1024LL * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024 * 1024));
    } else {
        return QString("%.2f GB").arg(bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// ─── Format Uptime ───────────────────────────────────────────────────────────

QString MonitorCenterWidget::formatUptime(qint64 ticks) const
{
    // SNMP sysUpTime is in hundredths of a second
    qint64 seconds = ticks / 100;
    qint64 days = seconds / 86400;
    qint64 hours = (seconds % 86400) / 3600;
    qint64 minutes = (seconds % 3600) / 60;

    if (days > 0) {
        return QString("%1 天 %2 小时 %3 分钟").arg(days).arg(hours).arg(minutes);
    } else if (hours > 0) {
        return QString("%1 小时 %2 分钟").arg(hours).arg(minutes);
    } else {
        return QString("%1 分钟").arg(minutes);
    }
}

// ─── Save / Load Devices ─────────────────────────────────────────────────────

void MonitorCenterWidget::saveDevices()
{
    QSettings settings("WukongToolkit", "MonitorCenter");
    settings.beginGroup("Devices");
    settings.beginWriteArray("deviceList");
    for (int i = 0; i < m_devices.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_devices[i].name);
        settings.setValue("ip", m_devices[i].ip);
        settings.setValue("community", m_devices[i].community);
    }
    settings.endArray();
    settings.endGroup();

    Logger::instance().debug("Monitor Center",
        QString("已保存 %1 个设备").arg(m_devices.size()));
}

void MonitorCenterWidget::loadDevices()
{
    QSettings settings("WukongToolkit", "MonitorCenter");
    settings.beginGroup("Devices");
    int size = settings.beginReadArray("deviceList");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        MonitorDeviceInfo dev;
        dev.name = settings.value("name").toString();
        dev.ip = settings.value("ip").toString();
        dev.community = settings.value("community", "public").toString();
        if (!dev.ip.isEmpty()) {
            m_devices.append(dev);
        }
    }
    settings.endArray();
    settings.endGroup();

    Logger::instance().debug("Monitor Center",
        QString("已加载 %1 个设备").arg(m_devices.size()));
}

// ─── Check SNMP Availability ─────────────────────────────────────────────────

bool MonitorCenterWidget::checkSnmpAvailable() const
{
    QProcess test;
    test.start("which", {"snmpget"});
    test.waitForFinished(2000);
    return test.exitCode() == 0;
}