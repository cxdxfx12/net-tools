#include "planner/PlannerWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QSpinBox>
#include <QLabel>
#include <QFile>
#include <QRegularExpression>
#include <QtMath>

// ============================================================================
// 内部 IP 工具函数
// ============================================================================
static quint32 ipToUint32(const QString& ip)
{
    QStringList parts = ip.split('.');
    if (parts.size() != 4) return 0;
    quint32 val = 0;
    for (int i = 0; i < 4; ++i) {
        bool ok = false;
        int octet = parts[i].toInt(&ok);
        if (!ok || octet < 0 || octet > 255) return 0;
        val = (val << 8) | static_cast<quint32>(octet);
    }
    return val;
}

static QString uint32ToIp(quint32 val)
{
    return QString("%1.%2.%3.%4")
        .arg((val >> 24) & 0xFF)
        .arg((val >> 16) & 0xFF)
        .arg((val >> 8) & 0xFF)
        .arg(val & 0xFF);
}

static quint32 prefixToMask(int prefix)
{
    if (prefix <= 0) return 0;
    if (prefix >= 32) return 0xFFFFFFFF;
    return ~0u << (32 - prefix);
}

static QString maskToString(quint32 mask)
{
    return uint32ToIp(mask);
}

static int requiredPrefix(int hostCount)
{
    // hostCount includes network + broadcast, so we need at least hostCount + 2 addresses
    int needed = hostCount + 2;
    // find smallest power of 2 >= needed
    int size = 1;
    int bits = 0;
    while (size < needed) {
        size <<= 1;
        ++bits;
    }
    return 32 - bits;
}

// ============================================================================
// PlannerWidget 实现
// ============================================================================
PlannerWidget::PlannerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

PlannerWidget::~PlannerWidget()
{
}

// ============================================================================
// UI 构建
// ============================================================================
void PlannerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // ========== 输入区域 ==========
    auto* inputGroup = new QGroupBox("子网规划参数");
    auto* inputLayout = new QFormLayout(inputGroup);

    m_projectNameEdit = new QLineEdit();
    m_projectNameEdit->setPlaceholderText("例如: 办公楼网络规划");

    m_baseAddrEdit = new QLineEdit();
    m_baseAddrEdit->setPlaceholderText("例如: 10.0.0.0/16");
    m_baseAddrEdit->setText("10.0.0.0/16");

    m_subnetCountSpin = new QSpinBox();
    m_subnetCountSpin->setRange(1, 100);
    m_subnetCountSpin->setValue(4);
    m_subnetCountSpin->setSuffix(" 个");

    m_hostCountSpin = new QSpinBox();
    m_hostCountSpin->setRange(1, 65536);
    m_hostCountSpin->setValue(254);
    m_hostCountSpin->setSuffix(" 台/子网");

    m_planBtn = new QPushButton("开始规划");

    inputLayout->addRow("项目名称:", m_projectNameEdit);
    inputLayout->addRow("基地址 (CIDR):", m_baseAddrEdit);
    inputLayout->addRow("子网数量:", m_subnetCountSpin);
    inputLayout->addRow("每子网主机数:", m_hostCountSpin);
    inputLayout->addRow("", m_planBtn);

    mainLayout->addWidget(inputGroup);

    // ========== 规划结果表格 ==========
    auto* resultGroup = new QGroupBox("规划结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);

    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(7);
    QStringList headers = {"部门/VLAN", "子网", "掩码", "网关", "DHCP 范围", "主机数", "利用率"};
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setColumnWidth(0, 100);
    m_resultTable->setColumnWidth(1, 150);
    m_resultTable->setColumnWidth(2, 150);
    m_resultTable->setColumnWidth(3, 150);
    m_resultTable->setColumnWidth(4, 200);
    m_resultTable->setColumnWidth(5, 80);
    m_resultTable->setColumnWidth(6, 80);

    resultLayout->addWidget(m_resultTable);
    mainLayout->addWidget(resultGroup);

    // ========== 底部按钮区 ==========
    auto* btnLayout = new QHBoxLayout();
    m_exportBtn = new QPushButton("导出 CSV");
    m_analyzeBtn = new QPushButton("利用率分析");
    btnLayout->addStretch();
    btnLayout->addWidget(m_analyzeBtn);
    btnLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(btnLayout);
}

// ============================================================================
// 信号连接
// ============================================================================
void PlannerWidget::setupConnections()
{
    connect(m_planBtn, &QPushButton::clicked, this, &PlannerWidget::onPlan);
    connect(m_exportBtn, &QPushButton::clicked, this, &PlannerWidget::onExport);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &PlannerWidget::onAnalyzeUtilization);
}

// ============================================================================
// 子网规划 (VLSM 算法)
// ============================================================================
void PlannerWidget::onPlan()
{
    QString baseAddr = m_baseAddrEdit->text().trimmed();
    int subnetCount = m_subnetCountSpin->value();
    int hostsPerSubnet = m_hostCountSpin->value();
    QString projectName = m_projectNameEdit->text().trimmed();

    // 解析基地址
    QRegularExpression cidrRe("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(\\d{1,2})$");
    auto match = cidrRe.match(baseAddr);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "输入错误", "基地址格式无效，请使用 CIDR 格式，例如: 10.0.0.0/16");
        return;
    }

    QString baseIp = match.captured(1);
    int basePrefix = match.captured(2).toInt();
    if (basePrefix < 0 || basePrefix > 32) {
        QMessageBox::warning(this, "输入错误", "前缀长度必须在 0-32 之间。");
        return;
    }

    quint32 baseIpUint = ipToUint32(baseIp);
    quint32 baseMask = prefixToMask(basePrefix);
    quint32 networkAddr = baseIpUint & baseMask;
    quint32 totalRange = ~baseMask;  // total addresses in the base network

    // 计算每个子网所需地址数 (含网络地址和广播地址)
    // 按 2 的幂次对齐
    struct SubnetReq {
        int index;
        int hosts;
        int needed;       // hosts + 2, rounded up to power of 2
        int prefixLen;
    };

    QList<SubnetReq> reqs;
    for (int i = 0; i < subnetCount; ++i) {
        SubnetReq r;
        r.index = i;
        r.hosts = hostsPerSubnet;
        r.prefixLen = requiredPrefix(r.hosts);
        int size = 1 << (32 - r.prefixLen);
        r.needed = size;
        reqs.append(r);
    }

    // 从大到小排序
    std::sort(reqs.begin(), reqs.end(), [](const SubnetReq& a, const SubnetReq& b) {
        return a.needed > b.needed;
    });

    // 分配地址块
    struct SubnetResult {
        int vlanId;
        int index;
        quint32 network;
        int prefixLen;
        quint32 mask;
        int hosts;
        int totalSize;
    };

    QList<SubnetResult> results;
    quint32 current = networkAddr;
    bool overflow = false;

    for (const auto& req : reqs) {
        SubnetResult sr;
        sr.vlanId = 10 + req.index;
        sr.index = req.index;
        sr.network = current;
        sr.prefixLen = req.prefixLen;
        sr.mask = prefixToMask(req.prefixLen);
        sr.hosts = req.hosts;
        sr.totalSize = req.needed;

        if (current + req.needed > networkAddr + totalRange + 1) {
            overflow = true;
            break;
        }

        current += req.needed;
        results.append(sr);
    }

    if (overflow) {
        QMessageBox::warning(this, "地址空间不足",
            QString("基地址 %1 无法容纳 %2 个子网（每子网 %3 台主机）。\n"
                    "请减少子网数量或主机数，或使用更大的基地址。")
                .arg(baseAddr)
                .arg(subnetCount)
                .arg(hostsPerSubnet));
        return;
    }

    // 按原始 index 排序输出
    std::sort(results.begin(), results.end(), [](const SubnetResult& a, const SubnetResult& b) {
        return a.index < b.index;
    });

    // 填充表格
    m_resultTable->setRowCount(0);
    for (const auto& sr : results) {
        int row = m_resultTable->rowCount();
        m_resultTable->insertRow(row);

        QString netStr = uint32ToIp(sr.network);
        QString maskStr = maskToString(sr.mask);
        QString gateway = calculateGateway(netStr, maskStr);
        QString dhcpRange = calculateDHCPRange(netStr, maskStr);
        int usableHosts = sr.totalSize - 2;
        double utilization = (usableHosts > 0) ? (sr.hosts * 100.0 / usableHosts) : 0;

        // VLAN ID
        m_resultTable->setItem(row, 0, new QTableWidgetItem(QString("VLAN %1").arg(sr.vlanId)));

        // 子网
        m_resultTable->setItem(row, 1, new QTableWidgetItem(
            QString("%1/%2").arg(netStr).arg(sr.prefixLen)));

        // 掩码
        m_resultTable->setItem(row, 2, new QTableWidgetItem(maskStr));

        // 网关
        m_resultTable->setItem(row, 3, new QTableWidgetItem(gateway));

        // DHCP 范围
        m_resultTable->setItem(row, 4, new QTableWidgetItem(dhcpRange));

        // 主机数
        m_resultTable->setItem(row, 5, new QTableWidgetItem(QString::number(sr.hosts)));

        // 利用率
        auto* utilItem = new QTableWidgetItem(QString("%1%").arg(utilization, 0, 'f', 1));
        if (utilization > 80) {
            utilItem->setForeground(QColor(200, 50, 0));
        } else if (utilization > 50) {
            utilItem->setForeground(QColor(200, 150, 0));
        } else {
            utilItem->setForeground(QColor(0, 150, 0));
        }
        m_resultTable->setItem(row, 6, utilItem);
    }

    Logger::instance().info("PlannerWidget",
        QString("子网规划完成: %1, 基地址 %2, %3 个子网")
            .arg(projectName.isEmpty() ? "未命名" : projectName)
            .arg(baseAddr)
            .arg(subnetCount));
}

// ============================================================================
// 计算网关地址 (子网第一个可用 IP)
// ============================================================================
QString PlannerWidget::calculateGateway(const QString& network, const QString& mask)
{
    Q_UNUSED(mask)
    quint32 net = ipToUint32(network);
    return uint32ToIp(net + 1);
}

// ============================================================================
// 计算 DHCP 范围 (.10 - .200)
// ============================================================================
QString PlannerWidget::calculateDHCPRange(const QString& network, const QString& mask)
{
    Q_UNUSED(mask)
    quint32 net = ipToUint32(network);
    quint32 start = net + 10;
    quint32 end = net + 200;
    return QString("%1 - %2").arg(uint32ToIp(start), uint32ToIp(end));
}

// ============================================================================
// 导出 CSV
// ============================================================================
void PlannerWidget::onExport()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据，请先进行子网规划。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV", "subnet_plan.csv",
                                                     "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    // BOM for UTF-8
    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "部门/VLAN,子网,掩码,网关,DHCP范围,主机数,利用率\n";

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            auto* item = m_resultTable->item(row, col);
            QString val = item ? item->text() : "-";
            if (val.contains(',') || val.contains('\n') || val.contains('"')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            cols << val;
        }
        out << cols.join(',') << "\n";
    }

    file.close();
    Logger::instance().info("PlannerWidget",
                            QString("子网规划已导出到: %1").arg(filePath));
    QMessageBox::information(this, "导出成功",
                              QString("已导出子网规划到:\n%1").arg(filePath));
}

// ============================================================================
// 利用率分析
// ============================================================================
void PlannerWidget::onAnalyzeUtilization()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "利用率分析", "没有可分析的数据，请先进行子网规划。");
        return;
    }

    int totalSubnets = m_resultTable->rowCount();
    int highUsage = 0;   // > 80%
    int mediumUsage = 0; // 50-80%
    int lowUsage = 0;    // < 50%
    double totalUtil = 0;

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        auto* utilItem = m_resultTable->item(row, 6);
        if (!utilItem) continue;
        QString utilStr = utilItem->text();
        utilStr.remove('%');
        bool ok = false;
        double util = utilStr.toDouble(&ok);
        if (!ok) continue;

        totalUtil += util;
        if (util > 80) {
            ++highUsage;
        } else if (util > 50) {
            ++mediumUsage;
        } else {
            ++lowUsage;
        }
    }

    double avgUtil = totalSubnets > 0 ? totalUtil / totalSubnets : 0;

    QString analysis;
    analysis += QString("总子网数: %1\n\n").arg(totalSubnets);
    analysis += QString("平均利用率: %1%\n\n").arg(avgUtil, 0, 'f', 1);
    analysis += QString("利用率分布:\n");
    analysis += QString("  高利用率 (>80%%):  %1 个子网\n").arg(highUsage);
    analysis += QString("  中等利用率 (50-80%%):  %1 个子网\n").arg(mediumUsage);
    analysis += QString("  低利用率 (<50%%):  %1 个子网\n").arg(lowUsage);

    if (highUsage > 0) {
        analysis += QString("\n⚠ 建议: 有 %1 个子网利用率超过 80%，建议预留更多地址空间。").arg(highUsage);
    }
    if (lowUsage > totalSubnets / 2) {
        analysis += QString("\n⚠ 建议: 超过半数子网利用率偏低，可考虑减少子网规模。");
    }

    QMessageBox::information(this, "利用率分析", analysis);
    Logger::instance().info("PlannerWidget",
        QString("利用率分析完成: 平均 %1%, 高 %2, 中 %3, 低 %4")
            .arg(avgUtil, 0, 'f', 1)
            .arg(highUsage)
            .arg(mediumUsage)
            .arg(lowUsage));
}