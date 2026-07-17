#include "ipv6/IPv6Widget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTabWidget>
#include <QHostAddress>
#include <QSplitter>
#include <QRegularExpression>

IPv6Widget::IPv6Widget(QWidget* parent)
    : QWidget(parent)
    , m_process(new QProcess(this))
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();

    Logger::instance().info(QStringLiteral("IPv6"), QStringLiteral("IPv6 Widget initialized"));
}

void IPv6Widget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    auto* tabWidget = new QTabWidget();

    // ── Tab 1: Diagnostics ──
    auto* diagTab = new QWidget();
    auto* diagLayout = new QVBoxLayout(diagTab);

    auto* diagGroup = new QGroupBox(QStringLiteral("IPv6 诊断"));
    auto* diagForm = new QFormLayout(diagGroup);
    m_ipv6Input = new QLineEdit();
    m_ipv6Input->setPlaceholderText(QStringLiteral("输入 IPv6 地址或域名"));
    diagForm->addRow(QStringLiteral("目标地址:"), m_ipv6Input);

    auto* btnRow = new QHBoxLayout();
    m_pingBtn = new QPushButton(QStringLiteral("Ping6"));
    m_traceBtn = new QPushButton(QStringLiteral("Traceroute6"));
    btnRow->addWidget(m_pingBtn);
    btnRow->addWidget(m_traceBtn);
    btnRow->addStretch();
    diagForm->addRow(btnRow);

    m_healthResult = new QPlainTextEdit();
    m_healthResult->setReadOnly(true);
    m_healthResult->setMaximumBlockCount(5000);
    diagForm->addRow(QStringLiteral("输出:"), m_healthResult);

    diagLayout->addWidget(diagGroup);
    tabWidget->addTab(diagTab, QStringLiteral("诊断"));

    // ── Tab 2: Address Calculator ──
    auto* calcTab = new QWidget();
    auto* calcLayout = new QVBoxLayout(calcTab);

    auto* calcGroup = new QGroupBox(QStringLiteral("IPv6 地址计算"));
    auto* calcForm = new QFormLayout(calcGroup);
    auto* calcInput = new QLineEdit();
    calcInput->setPlaceholderText(QStringLiteral("如 2001:db8::1"));
    m_prefixSpin = new QSpinBox();
    m_prefixSpin->setRange(0, 128);
    m_prefixSpin->setValue(64);
    auto* calcBtn = new QPushButton(QStringLiteral("计算"));
    calcForm->addRow(QStringLiteral("IPv6 地址:"), calcInput);
    calcForm->addRow(QStringLiteral("前缀长度:"), m_prefixSpin);
    calcForm->addRow(calcBtn);

    m_networkAddr = new QLineEdit();
    m_networkAddr->setReadOnly(true);
    m_broadcastAddr = new QLineEdit();
    m_broadcastAddr->setReadOnly(true);
    m_rangeAddr = new QLineEdit();
    m_rangeAddr->setReadOnly(true);
    m_hostCount = new QLineEdit();
    m_hostCount->setReadOnly(true);
    calcForm->addRow(QStringLiteral("网络地址:"), m_networkAddr);
    calcForm->addRow(QStringLiteral("广播地址:"), m_broadcastAddr);
    calcForm->addRow(QStringLiteral("可用范围:"), m_rangeAddr);
    calcForm->addRow(QStringLiteral("主机数:"), m_hostCount);

    calcLayout->addWidget(calcGroup);
    calcLayout->addStretch();
    tabWidget->addTab(calcTab, QStringLiteral("地址规划"));

    connect(calcInput, &QLineEdit::textChanged, this, [this, calcInput]() {
        m_ipv6Input->setText(calcInput->text());
    });
    connect(calcBtn, &QPushButton::clicked, this, &IPv6Widget::onCalculate);

    // ── Tab 3: NDP Table ──
    auto* ndpTab = new QWidget();
    auto* ndpLayout = new QVBoxLayout(ndpTab);
    m_ndpTable = new QTableWidget();
    m_ndpTable->setColumnCount(4);
    m_ndpTable->setHorizontalHeaderLabels({QStringLiteral("IPv6 地址"), QStringLiteral("MAC 地址"), QStringLiteral("接口"), QStringLiteral("状态")});
    m_ndpTable->horizontalHeader()->setStretchLastSection(true);
    m_ndpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ndpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto* ndpRefreshBtn = new QPushButton(QStringLiteral("刷新 NDP 表"));
    ndpLayout->addWidget(ndpRefreshBtn);
    ndpLayout->addWidget(m_ndpTable);
    tabWidget->addTab(ndpTab, QStringLiteral("邻居"));

    connect(ndpRefreshBtn, &QPushButton::clicked, this, &IPv6Widget::onNDPRefresh);

    // ── Tab 4: Route Table ──
    auto* routeTab = new QWidget();
    auto* routeLayout = new QVBoxLayout(routeTab);
    m_routeTable = new QTableWidget();
    m_routeTable->setColumnCount(4);
    m_routeTable->setHorizontalHeaderLabels({QStringLiteral("目标网络"), QStringLiteral("下一跳"), QStringLiteral("接口"), QStringLiteral("度量值")});
    m_routeTable->horizontalHeader()->setStretchLastSection(true);
    m_routeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_routeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto* routeRefreshBtn = new QPushButton(QStringLiteral("刷新路由表"));
    routeLayout->addWidget(routeRefreshBtn);
    routeLayout->addWidget(m_routeTable);
    tabWidget->addTab(routeTab, QStringLiteral("路由"));

    connect(routeRefreshBtn, &QPushButton::clicked, this, &IPv6Widget::onRouteRefresh);

    // ── Tab 5: Health Check ──
    auto* healthTab = new QWidget();
    auto* healthLayout = new QVBoxLayout(healthTab);

    auto* healthGroup = new QGroupBox(QStringLiteral("双栈检测"));
    auto* healthForm = new QFormLayout(healthGroup);
    m_ipv4Status = new QLabel(QStringLiteral("未检测"));
    m_ipv6Status = new QLabel(QStringLiteral("未检测"));
    m_dnsStatus = new QLabel(QStringLiteral("未检测"));
    healthForm->addRow(QStringLiteral("IPv4 连通性:"), m_ipv4Status);
    healthForm->addRow(QStringLiteral("IPv6 连通性:"), m_ipv6Status);
    healthForm->addRow(QStringLiteral("DNS 解析:"), m_dnsStatus);

    m_healthBtn = new QPushButton(QStringLiteral("开始健康检查"));
    healthForm->addRow(m_healthBtn);

    auto* healthResultOut = new QPlainTextEdit();
    healthResultOut->setReadOnly(true);
    healthResultOut->setMaximumBlockCount(5000);
    healthForm->addRow(QStringLiteral("详细结果:"), healthResultOut);

    healthLayout->addWidget(healthGroup);
    tabWidget->addTab(healthTab, QStringLiteral("健康检查"));

    connect(m_healthBtn, &QPushButton::clicked, this, [this, healthResultOut]() {
        m_healthResult = healthResultOut;
        onHealthCheck();
    });

    mainLayout->addWidget(tabWidget);

    // ── Bottom: Export ──
    auto* bottomLayout = new QHBoxLayout();
    m_exportBtn = new QPushButton(QStringLiteral("导出报告"));
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(bottomLayout);

    // Style
    setStyleSheet(QStringLiteral(
        "QWidget { background-color: #0D1117; color: #E6EDF3; font-size: 13px; }"
        "QGroupBox { border: 1px solid #30363D; border-radius: 4px; margin-top: 8px; padding-top: 16px; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QTableWidget { background-color: #161B22; alternate-background-color: #2B2D31; border: 1px solid #30363D; gridline-color: #30363D; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background-color: #2B2D31; color: #E6EDF3; border: 1px solid #30363D; padding: 4px; }"
        "QLineEdit, QSpinBox, QPlainTextEdit { background-color: #161B22; border: 1px solid #30363D; border-radius: 3px; padding: 4px; color: #E6EDF3; }"
        "QPushButton { background-color: #30363D; border: 1px solid #555; border-radius: 3px; padding: 5px 15px; color: #E6EDF3; }"
        "QPushButton:hover { background-color: #4A4D51; }"
        "QPushButton:pressed { background-color: #2B2D31; }"
        "QTabWidget::pane { border: 1px solid #30363D; background-color: #0D1117; }"
        "QTabBar::tab { background-color: #161B22; color: #E6EDF3; padding: 6px 15px; border: 1px solid #30363D; }"
        "QTabBar::tab:selected { background-color: #30363D; }"
    ));

    // Pre-populate NDP mock data
    onNDPRefresh();
    onRouteRefresh();
}

void IPv6Widget::setupConnections()
{
    connect(m_pingBtn, &QPushButton::clicked, this, &IPv6Widget::onPing6);
    connect(m_traceBtn, &QPushButton::clicked, this, &IPv6Widget::onTrace6);
    connect(m_exportBtn, &QPushButton::clicked, this, &IPv6Widget::onExport);
}

void IPv6Widget::onPing6()
{
    QString target = m_ipv6Input->text().trimmed();
    if (target.isEmpty()) {
        m_healthResult->appendPlainText(QStringLiteral("[错误] 请输入目标地址"));
        return;
    }

    m_healthResult->clear();
    m_healthResult->appendPlainText(QStringLiteral("=== Ping6 %1 ===").arg(target));
    m_pingBtn->setEnabled(false);

    m_process->disconnect();
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        m_healthResult->appendPlainText(m_process->readAllStandardOutput());
        m_healthResult->appendPlainText(m_process->readAllStandardError());
        m_healthResult->appendPlainText(QStringLiteral("--- 完成 (exit: %1) ---").arg(exitCode));
        m_pingBtn->setEnabled(true);
    });

    m_process->start(QStringLiteral("ping6"), {QStringLiteral("-c"), QStringLiteral("4"), target});
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("Ping6: %1").arg(target));
}

void IPv6Widget::onTrace6()
{
    QString target = m_ipv6Input->text().trimmed();
    if (target.isEmpty()) {
        m_healthResult->appendPlainText(QStringLiteral("[错误] 请输入目标地址"));
        return;
    }

    m_healthResult->clear();
    m_healthResult->appendPlainText(QStringLiteral("=== Traceroute6 %1 ===").arg(target));
    m_traceBtn->setEnabled(false);

    m_process->disconnect();
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        m_healthResult->appendPlainText(m_process->readAllStandardOutput());
        m_healthResult->appendPlainText(m_process->readAllStandardError());
        m_healthResult->appendPlainText(QStringLiteral("--- 完成 (exit: %1) ---").arg(exitCode));
        m_traceBtn->setEnabled(true);
    });

    m_process->start(QStringLiteral("traceroute6"), {target});
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("Traceroute6: %1").arg(target));
}

void IPv6Widget::onCalculate()
{
    QString addr = m_ipv6Input->text().trimmed();
    int prefix = m_prefixSpin->value();

    QHostAddress hostAddr(addr);
    if (hostAddr.isNull() || hostAddr.protocol() != QAbstractSocket::IPv6Protocol) {
        m_networkAddr->setText(QStringLiteral("无效的 IPv6 地址"));
        return;
    }

    Q_IPV6ADDR ipv6 = hostAddr.toIPv6Address();
    quint8* octets = ipv6.c;

    // Calculate network address
    Q_IPV6ADDR netAddr = {};
    int fullBytes = prefix / 8;
    int remBits = prefix % 8;

    for (int i = 0; i < fullBytes && i < 16; i++) {
        netAddr.c[i] = octets[i];
    }
    if (remBits > 0 && fullBytes < 16) {
        quint8 mask = 0xFF << (8 - remBits);
        netAddr.c[fullBytes] = octets[fullBytes] & mask;
    }

    QHostAddress netHost(netAddr);
    m_networkAddr->setText(netHost.toString());

    // Broadcast = all ones in host portion
    Q_IPV6ADDR bcastAddr = {};
    for (int i = 0; i < fullBytes && i < 16; i++) {
        bcastAddr.c[i] = octets[i];
    }
    if (remBits > 0 && fullBytes < 16) {
        quint8 mask = 0xFF << (8 - remBits);
        bcastAddr.c[fullBytes] = octets[fullBytes] & mask;
        bcastAddr.c[fullBytes] |= (0xFF >> remBits);
    }
    for (int i = fullBytes + (remBits > 0 ? 1 : 0); i < 16; i++) {
        bcastAddr.c[i] = 0xFF;
    }

    QHostAddress bcastHost(bcastAddr);
    m_broadcastAddr->setText(bcastHost.toString());

    // Host count
    int hostBits = 128 - prefix;
    QString hostCountStr;
    if (hostBits <= 10) {
        hostCountStr = QString::number(1ULL << hostBits);
    } else {
        hostCountStr = QStringLiteral("2^%1 (≈ %2)").arg(hostBits).arg(QString::number(1ULL << (hostBits > 60 ? 60 : hostBits), 'e', 0));
    }
    m_hostCount->setText(hostCountStr);

    // Range
    Q_IPV6ADDR firstAddr = netAddr;
    if (hostBits > 0) {
        firstAddr.c[15] = 1;
    }
    QHostAddress firstHost(firstAddr);
    m_rangeAddr->setText(QStringLiteral("%1 - %2").arg(firstHost.toString(), bcastHost.toString()));

    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("IPv6 calc: %1/%2").arg(addr).arg(prefix));
}

void IPv6Widget::onHealthCheck()
{
    m_healthResult->clear();
    m_healthResult->appendPlainText(QStringLiteral("=== IPv6 健康检查 ===\n"));

    // IPv4 check
    m_healthResult->appendPlainText(QStringLiteral("[1] IPv4 连通性检查..."));
    m_ipv4Status->setText(QStringLiteral("检查中..."));
    QProcess ipv4Proc;
    ipv4Proc.start(QStringLiteral("ping"), {QStringLiteral("-c"), QStringLiteral("2"), QStringLiteral("-W"), QStringLiteral("2"), QStringLiteral("8.8.8.8")});
    ipv4Proc.waitForFinished(5000);
    if (ipv4Proc.exitCode() == 0) {
        m_ipv4Status->setText(QStringLiteral("正常"));
        m_ipv4Status->setStyleSheet(QStringLiteral("color: #3FB950; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  IPv4 连通性: 正常"));
    } else {
        m_ipv4Status->setText(QStringLiteral("异常"));
        m_ipv4Status->setStyleSheet(QStringLiteral("color: #F85149; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  IPv4 连通性: 异常"));
    }

    // IPv6 check
    m_healthResult->appendPlainText(QStringLiteral("\n[2] IPv6 连通性检查..."));
    m_ipv6Status->setText(QStringLiteral("检查中..."));
    QProcess ipv6Proc;
    ipv6Proc.start(QStringLiteral("ping6"), {QStringLiteral("-c"), QStringLiteral("2"), QStringLiteral("-W"), QStringLiteral("2"), QStringLiteral("2001:4860:4860::8888")});
    ipv6Proc.waitForFinished(5000);
    if (ipv6Proc.exitCode() == 0) {
        m_ipv6Status->setText(QStringLiteral("正常"));
        m_ipv6Status->setStyleSheet(QStringLiteral("color: #3FB950; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  IPv6 连通性: 正常"));
    } else {
        m_ipv6Status->setText(QStringLiteral("异常"));
        m_ipv6Status->setStyleSheet(QStringLiteral("color: #F85149; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  IPv6 连通性: 异常"));
    }

    // DNS check
    m_healthResult->appendPlainText(QStringLiteral("\n[3] DNS AAAA 解析检查..."));
    m_dnsStatus->setText(QStringLiteral("检查中..."));
    QProcess dnsProc;
    dnsProc.start(QStringLiteral("nslookup"), {QStringLiteral("-type=AAAA"), QStringLiteral("google.com")});
    dnsProc.waitForFinished(5000);
    QString dnsOutput = dnsProc.readAllStandardOutput();
    if (dnsOutput.contains(QStringLiteral("IPv6")) || dnsOutput.contains(QStringLiteral("AAAA"))) {
        m_dnsStatus->setText(QStringLiteral("正常"));
        m_dnsStatus->setStyleSheet(QStringLiteral("color: #3FB950; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  DNS AAAA 解析: 正常"));
    } else {
        m_dnsStatus->setText(QStringLiteral("异常"));
        m_dnsStatus->setStyleSheet(QStringLiteral("color: #F85149; font-weight: bold;"));
        m_healthResult->appendPlainText(QStringLiteral("  DNS AAAA 解析: 异常"));
    }

    m_healthResult->appendPlainText(QStringLiteral("\n=== 健康检查完成 ==="));
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("IPv6 health check completed"));
}

void IPv6Widget::onExport()
{
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出 IPv6 报告"), QStringLiteral("ipv6_report.csv"), QStringLiteral("CSV (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法打开文件"));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QStringLiteral("\xEF\xBB\xBF"); // BOM

    out << QStringLiteral("IPv6 管理报告\n\n");

    out << QStringLiteral("NDP 邻居表\n");
    out << QStringLiteral("IPv6 地址,MAC 地址,接口,状态\n");
    for (int r = 0; r < m_ndpTable->rowCount(); r++) {
        out << m_ndpTable->item(r, 0)->text() << QStringLiteral(",")
            << m_ndpTable->item(r, 1)->text() << QStringLiteral(",")
            << m_ndpTable->item(r, 2)->text() << QStringLiteral(",")
            << m_ndpTable->item(r, 3)->text() << QStringLiteral("\n");
    }

    out << QStringLiteral("\n路由表\n");
    out << QStringLiteral("目标网络,下一跳,接口,度量值\n");
    for (int r = 0; r < m_routeTable->rowCount(); r++) {
        out << m_routeTable->item(r, 0)->text() << QStringLiteral(",")
            << m_routeTable->item(r, 1)->text() << QStringLiteral(",")
            << m_routeTable->item(r, 2)->text() << QStringLiteral(",")
            << m_routeTable->item(r, 3)->text() << QStringLiteral("\n");
    }

    file.close();
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("IPv6 report exported: %1").arg(filePath));
}

void IPv6Widget::onNDPRefresh()
{
    QProcess proc;
    proc.start(QStringLiteral("ndp"), {QStringLiteral("-a")});
    proc.waitForFinished(3000);
    QString output = proc.readAllStandardOutput();

    if (!output.isEmpty()) {
        parseNDPOutput(output);
    } else {
        // Mock data
        m_ndpTable->setRowCount(0);
        struct MockNDP { QString ipv6, mac, iface, state; };
        QList<MockNDP> mock = {
            {QStringLiteral("fe80::1:2ff:fe03:405"), QStringLiteral("00:11:22:33:44:55"), QStringLiteral("en0"), QStringLiteral("REACHABLE")},
            {QStringLiteral("fe80::aabb:ccff:fed0:1234"), QStringLiteral("aa:bb:cc:dd:ee:ff"), QStringLiteral("en0"), QStringLiteral("STALE")},
            {QStringLiteral("fe80::cafe:babe:0000:0001"), QStringLiteral("ca:fe:ba:be:00:01"), QStringLiteral("en1"), QStringLiteral("REACHABLE")},
            {QStringLiteral("2001:db8::1"), QStringLiteral("11:22:33:aa:bb:cc"), QStringLiteral("en0"), QStringLiteral("PERMANENT")},
            {QStringLiteral("2001:db8::2"), QStringLiteral("22:33:44:bb:cc:dd"), QStringLiteral("en0"), QStringLiteral("REACHABLE")},
            {QStringLiteral("2001:db8::100"), QStringLiteral("33:44:55:cc:dd:ee"), QStringLiteral("en1"), QStringLiteral("STALE")},
            {QStringLiteral("fd00::1"), QStringLiteral("44:55:66:dd:ee:ff"), QStringLiteral("bridge0"), QStringLiteral("REACHABLE")},
            {QStringLiteral("fd00::2"), QStringLiteral("55:66:77:ee:ff:00"), QStringLiteral("bridge0"), QStringLiteral("PROBE")},
        };
        for (const auto& e : mock) {
            int r = m_ndpTable->rowCount();
            m_ndpTable->insertRow(r);
            m_ndpTable->setItem(r, 0, new QTableWidgetItem(e.ipv6));
            m_ndpTable->setItem(r, 1, new QTableWidgetItem(e.mac));
            m_ndpTable->setItem(r, 2, new QTableWidgetItem(e.iface));
            auto* stateItem = new QTableWidgetItem(e.state);
            if (e.state == QStringLiteral("REACHABLE")) stateItem->setForeground(QColor(QStringLiteral("#3FB950")));
            else if (e.state == QStringLiteral("STALE")) stateItem->setForeground(QColor(QStringLiteral("#D29922")));
            else if (e.state == QStringLiteral("PROBE")) stateItem->setForeground(QColor(QStringLiteral("#58A6FF")));
            m_ndpTable->setItem(r, 3, stateItem);
        }
    }
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("NDP table refreshed"));
}

void IPv6Widget::onRouteRefresh()
{
    QProcess proc;
    proc.start(QStringLiteral("netstat"), {QStringLiteral("-rn"), QStringLiteral("-f"), QStringLiteral("inet6")});
    proc.waitForFinished(3000);
    QString output = proc.readAllStandardOutput();

    if (!output.isEmpty()) {
        parseRouteOutput(output);
    } else {
        // Mock data
        m_routeTable->setRowCount(0);
        struct MockRoute { QString dest, nexthop, iface, metric; };
        QList<MockRoute> mock = {
            {QStringLiteral("::1"), QStringLiteral("::1"), QStringLiteral("lo0"), QStringLiteral("0")},
            {QStringLiteral("fe80::%lo0/64"), QStringLiteral("link#1"), QStringLiteral("lo0"), QStringLiteral("0")},
            {QStringLiteral("fe80::%en0/64"), QStringLiteral("link#2"), QStringLiteral("en0"), QStringLiteral("0")},
            {QStringLiteral("2001:db8::/64"), QStringLiteral("link#2"), QStringLiteral("en0"), QStringLiteral("0")},
            {QStringLiteral("fd00::/64"), QStringLiteral("link#3"), QStringLiteral("bridge0"), QStringLiteral("0")},
            {QStringLiteral("default"), QStringLiteral("fe80::1%en0"), QStringLiteral("en0"), QStringLiteral("100")},
            {QStringLiteral("2001:db8:1::/48"), QStringLiteral("2001:db8::1"), QStringLiteral("en0"), QStringLiteral("200")},
            {QStringLiteral("2001:db8:2::/48"), QStringLiteral("2001:db8::1"), QStringLiteral("en0"), QStringLiteral("200")},
        };
        for (const auto& e : mock) {
            int r = m_routeTable->rowCount();
            m_routeTable->insertRow(r);
            m_routeTable->setItem(r, 0, new QTableWidgetItem(e.dest));
            m_routeTable->setItem(r, 1, new QTableWidgetItem(e.nexthop));
            m_routeTable->setItem(r, 2, new QTableWidgetItem(e.iface));
            m_routeTable->setItem(r, 3, new QTableWidgetItem(e.metric));
        }
    }
    Logger::instance().info(QStringLiteral("IPv6"),QStringLiteral("Route table refreshed"));
}

void IPv6Widget::parseNDPOutput(const QString& output)
{
    m_ndpTable->setRowCount(0);
    QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")));
        if (parts.size() >= 4) {
            int r = m_ndpTable->rowCount();
            m_ndpTable->insertRow(r);
            m_ndpTable->setItem(r, 0, new QTableWidgetItem(parts[0]));
            m_ndpTable->setItem(r, 1, new QTableWidgetItem(parts.size() > 1 ? parts[1] : QStringLiteral("-")));
            m_ndpTable->setItem(r, 2, new QTableWidgetItem(parts.size() > 2 ? parts[2] : QStringLiteral("-")));
            m_ndpTable->setItem(r, 3, new QTableWidgetItem(parts.size() > 3 ? parts[3] : QStringLiteral("-")));
        }
    }
}

void IPv6Widget::parseRouteOutput(const QString& output)
{
    m_routeTable->setRowCount(0);
    QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty() || line.startsWith(QLatin1String("Internet")) || line.startsWith(QLatin1String("Destination"))) continue;
        QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")));
        if (parts.size() >= 4) {
            int r = m_routeTable->rowCount();
            m_routeTable->insertRow(r);
            m_routeTable->setItem(r, 0, new QTableWidgetItem(parts[0]));
            m_routeTable->setItem(r, 1, new QTableWidgetItem(parts[1]));
            m_routeTable->setItem(r, 2, new QTableWidgetItem(parts.size() > 3 ? parts[3] : QStringLiteral("-")));
            m_routeTable->setItem(r, 3, new QTableWidgetItem(parts.size() > 4 ? parts[4] : QStringLiteral("-")));
        }
    }
}