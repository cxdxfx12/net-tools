#include "performance/PerformanceWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QPen>
#include <QBrush>
#include <QSplitter>
#include <QLabel>
#include <QDateTime>
#include <QRandomGenerator>
#include <QFrame>
#include <QApplication>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
// TrendChartWidget
// ═══════════════════════════════════════════════════════════════════════════════

TrendChartWidget::TrendChartWidget(ChartType type, QWidget* parent)
    : QWidget(parent)
    , m_chartType(type)
{
    setMinimumSize(280, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrendChartWidget::setData(const QList<PerfDataPoint>& data)
{
    m_data = data;
    update();
}

void TrendChartWidget::setTitle(const QString& title)
{
    m_title = title;
    update();
}

QColor TrendChartWidget::lineColor() const
{
    switch (m_chartType) {
    case CPU:        return QColor("#409EFF");
    case Memory:     return QColor("#67C23A");
    case Bandwidth:  return QColor("#E6A23C");
    case Temperature:return QColor("#F56C6C");
    }
    return QColor("#409EFF");
}

QColor TrendChartWidget::fillColor() const
{
    QColor c = lineColor();
    c.setAlpha(40);
    return c;
}

QString TrendChartWidget::valueLabel(double value) const
{
    switch (m_chartType) {
    case CPU:        return QString("%1%").arg(value, 0, 'f', 1);
    case Memory:     return QString("%1%").arg(value, 0, 'f', 1);
    case Bandwidth:  return QString("%1 Mbps").arg(value, 0, 'f', 1);
    case Temperature:return QString("%1°C").arg(value, 0, 'f', 1);
    }
    return QString::number(value, 'f', 1);
}

double TrendChartWidget::maxValue() const
{
    double max = 0;
    for (const auto& p : m_data) {
        double v = 0;
        switch (m_chartType) {
        case CPU:         v = p.cpu; break;
        case Memory:      v = p.mem; break;
        case Bandwidth:   v = p.bandwidth; break;
        case Temperature: v = p.temp; break;
        }
        if (v > max) max = v;
    }
    if (max <= 0) max = 100;
    // 上方留 15% 余量
    return max * 1.15;
}

double TrendChartWidget::minValue() const
{
    if (m_data.isEmpty()) return 0;
    double min = 1e18;
    for (const auto& p : m_data) {
        double v = 0;
        switch (m_chartType) {
        case CPU:         v = p.cpu; break;
        case Memory:      v = p.mem; break;
        case Bandwidth:   v = p.bandwidth; break;
        case Temperature: v = p.temp; break;
        }
        if (v < min) min = v;
    }
    if (min >= 1e17) min = 0;
    return min * 0.9;
}

void TrendChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    // ── 背景 ──
    painter.fillRect(rect(), QColor("#1E1F22"));
    painter.setPen(QPen(QColor("#3C3F41"), 1));
    painter.drawRect(0, 0, w - 1, h - 1);

    // ── 内边距 ──
    const int padLeft   = 48;
    const int padRight  = 16;
    const int padTop    = 28;
    const int padBottom = 28;
    int plotW = w - padLeft - padRight;
    int plotH = h - padTop - padBottom;

    if (plotW <= 0 || plotH <= 0) return;

    // ── 标题 ──
    painter.setPen(QColor("#8C8C8C"));
    QFont titleFont = painter.font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRect(padLeft, 4, plotW, 20), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // ── 网格线 + Y轴标签 ──
    double vMin = minValue();
    double vMax = maxValue();
    if (vMax - vMin < 0.001) vMax = vMin + 1;

    painter.setPen(QPen(QColor("#2C2D30"), 1, Qt::DashLine));
    QFont labelFont = painter.font();
    labelFont.setPointSize(8);
    painter.setFont(labelFont);

    int gridLines = 4;
    for (int i = 0; i <= gridLines; ++i) {
        double ratio = 1.0 - (double)i / gridLines;
        int y = padTop + (int)(i * plotH / (double)gridLines);
        painter.drawLine(padLeft, y, w - padRight, y);

        double val = vMin + ratio * (vMax - vMin);
        painter.setPen(QColor("#8C8C8C"));
        painter.drawText(QRect(0, y - 8, padLeft - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         valueLabel(val));
        painter.setPen(QPen(QColor("#2C2D30"), 1, Qt::DashLine));
    }

    if (m_data.size() < 2) {
        painter.setPen(QColor("#8C8C8C"));
        painter.drawText(QRect(padLeft, padTop, plotW, plotH),
                         Qt::AlignCenter, "暂无数据");
        return;
    }

    // ── X轴时间标签 ──
    painter.setPen(QColor("#8C8C8C"));
    int xLabelCount = qMin(5, m_data.size());
    for (int i = 0; i < xLabelCount; ++i) {
        int idx = (int)(i * (m_data.size() - 1) / (double)(xLabelCount - 1));
        if (idx < 0) idx = 0;
        if (idx >= m_data.size()) idx = m_data.size() - 1;
        int x = padLeft + (int)(idx * plotW / (double)(m_data.size() - 1));
        QString label = m_data[idx].timestamp.toString("MM-dd");
        painter.drawText(QRect(x - 30, h - padBottom + 4, 60, 16),
                         Qt::AlignCenter, label);
    }

    // ── 计算折线路径 ──
    auto dataValue = [this](const PerfDataPoint& p) -> double {
        switch (m_chartType) {
        case CPU:         return p.cpu;
        case Memory:      return p.mem;
        case Bandwidth:   return p.bandwidth;
        case Temperature: return p.temp;
        }
        return 0;
    };

    QPainterPath linePath;
    QPainterPath fillPath;
    bool first = true;

    for (int i = 0; i < m_data.size(); ++i) {
        double x = padLeft + (double)i * plotW / (m_data.size() - 1);
        double val = dataValue(m_data[i]);
        double y = padTop + plotH - (val - vMin) / (vMax - vMin) * plotH;
        y = qBound((double)padTop, y, (double)(padTop + plotH));

        if (first) {
            linePath.moveTo(x, y);
            fillPath.moveTo(x, padTop + plotH);
            fillPath.lineTo(x, y);
            first = false;
        } else {
            linePath.lineTo(x, y);
            fillPath.lineTo(x, y);
        }
    }

    // 填充区域闭合
    double lastX = padLeft + plotW;
    fillPath.lineTo(lastX, padTop + plotH);
    fillPath.closeSubpath();

    // ── 绘制填充 ──
    painter.setPen(Qt::NoPen);
    painter.setBrush(fillColor());
    painter.drawPath(fillPath);

    // ── 绘制折线 ──
    QPen linePen(lineColor(), 2);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(linePath);

    // ── 绘制数据点 ──
    if (m_data.size() <= 60) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(lineColor());
        for (int i = 0; i < m_data.size(); ++i) {
            double x = padLeft + (double)i * plotW / (m_data.size() - 1);
            double val = dataValue(m_data[i]);
            double y = padTop + plotH - (val - vMin) / (vMax - vMin) * plotH;
            y = qBound((double)padTop, y, (double)(padTop + plotH));
            painter.drawEllipse(QPointF(x, y), 2.5, 2.5);
        }
    }

    // ── 最新值标签 ──
    if (!m_data.isEmpty()) {
        double lastVal = dataValue(m_data.last());
        double lastY = padTop + plotH - (lastVal - vMin) / (vMax - vMin) * plotH;
        lastY = qBound((double)padTop, lastY, (double)(padTop + plotH));

        QString lastLabel = valueLabel(lastVal);
        QFont valFont = painter.font();
        valFont.setPointSize(9);
        valFont.setBold(true);
        painter.setFont(valFont);

        QFontMetrics fm(valFont);
        int labelW = fm.horizontalAdvance(lastLabel) + 10;
        int labelH = 18;
        int labelX = (int)lastX - labelW - 4;
        int labelY = (int)lastY - labelH - 4;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#25262B"));
        painter.drawRoundedRect(labelX, labelY, labelW, labelH, 3, 3);

        painter.setPen(lineColor());
        painter.drawText(QRect(labelX, labelY, labelW, labelH),
                         Qt::AlignCenter, lastLabel);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// PerformanceWidget
// ═══════════════════════════════════════════════════════════════════════════════

PerformanceWidget::PerformanceWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();
    generateMockData();
}

PerformanceWidget::~PerformanceWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void PerformanceWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 通用样式 lambda ──
    auto styleCombo = [](QComboBox* combo, int minWidth = 160) {
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

    // ── 顶部控制栏 ──
    auto* topGroup = new QGroupBox("性能管理中心");
    topGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(12);

    auto* deviceLabel = new QLabel("设备选择:");
    deviceLabel->setStyleSheet("font-size: 13px; color: #8C8C8C;");
    m_deviceCombo = new QComboBox();
    styleCombo(m_deviceCombo, 200);
    m_deviceCombo->addItems({
        "Core-Switch-01 (192.168.1.1)",
        "Core-Router-01 (192.168.1.254)",
        "Access-Switch-02 (10.0.1.1)",
        "Firewall-01 (192.168.1.253)",
        "Server-Rack-A (10.0.2.100)",
        "WLC-01 (192.168.1.250)"
    });

    auto* timeLabel = new QLabel("时间范围:");
    timeLabel->setStyleSheet("font-size: 13px; color: #8C8C8C;");
    m_timeRangeCombo = new QComboBox();
    styleCombo(m_timeRangeCombo, 100);
    m_timeRangeCombo->addItems({"24H", "7D", "30D", "90D"});
    m_timeRangeCombo->setCurrentIndex(2); // 默认 30D

    m_exportBtn = new QPushButton("导出 CSV");
    styleButton(m_exportBtn, "#E6A23C", "#EBB563");

    topLayout->addWidget(deviceLabel);
    topLayout->addWidget(m_deviceCombo);
    topLayout->addWidget(timeLabel);
    topLayout->addWidget(m_timeRangeCombo);
    topLayout->addStretch();
    topLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(topGroup);

    // ── 中间：左右分栏 ──
    auto* bodySplitter = new QSplitter(Qt::Horizontal);
    bodySplitter->setStyleSheet(
        "QSplitter::handle { background-color: #3C3F41; width: 2px; }"
    );

    // ── 左侧：2x2 趋势图网格 ──
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(6);

    auto* chartsGrid = new QGridLayout();
    chartsGrid->setSpacing(6);

    m_cpuTrendWidget = new TrendChartWidget(TrendChartWidget::CPU);
    m_cpuTrendWidget->setTitle("CPU 使用率趋势");
    m_memoryTrendWidget = new TrendChartWidget(TrendChartWidget::Memory);
    m_memoryTrendWidget->setTitle("内存使用率趋势");
    m_bandwidthTrendWidget = new TrendChartWidget(TrendChartWidget::Bandwidth);
    m_bandwidthTrendWidget->setTitle("接口带宽趋势");
    m_temperatureTrendWidget = new TrendChartWidget(TrendChartWidget::Temperature);
    m_temperatureTrendWidget->setTitle("设备温度趋势");

    chartsGrid->addWidget(m_cpuTrendWidget, 0, 0);
    chartsGrid->addWidget(m_memoryTrendWidget, 0, 1);
    chartsGrid->addWidget(m_bandwidthTrendWidget, 1, 0);
    chartsGrid->addWidget(m_temperatureTrendWidget, 1, 1);

    leftLayout->addLayout(chartsGrid);

    // ── 右侧：Top N + 历史表格 ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(6);

    // Top N
    auto* topNGroup = new QGroupBox("Top N 设备排行");
    topNGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #E6A23C; font-size: 12px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* topNLayout = new QVBoxLayout(topNGroup);
    topNLayout->setContentsMargins(4, 4, 4, 4);

    m_topNTable = new QTableWidget(0, 5);
    m_topNTable->setHorizontalHeaderLabels({"设备名", "CPU%", "内存%", "温度", "带宽利用率"});
    m_topNTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_topNTable->setColumnWidth(1, 70);
    m_topNTable->setColumnWidth(2, 70);
    m_topNTable->setColumnWidth(3, 80);
    m_topNTable->setColumnWidth(4, 90);
    m_topNTable->verticalHeader()->setVisible(false);
    styleTable(m_topNTable);

    topNLayout->addWidget(m_topNTable);

    // 历史数据
    auto* historyGroup = new QGroupBox("历史数据");
    historyGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 12px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
        "  padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
        "}"
    );
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setContentsMargins(4, 4, 4, 4);

    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels({"时间", "CPU%", "内存%", "温度", "带宽"});
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_historyTable->setColumnWidth(1, 70);
    m_historyTable->setColumnWidth(2, 70);
    m_historyTable->setColumnWidth(3, 80);
    m_historyTable->setColumnWidth(4, 90);
    m_historyTable->verticalHeader()->setVisible(false);
    styleTable(m_historyTable);

    historyLayout->addWidget(m_historyTable);

    rightLayout->addWidget(topNGroup, 1);
    rightLayout->addWidget(historyGroup, 2);

    bodySplitter->addWidget(leftWidget);
    bodySplitter->addWidget(rightWidget);
    bodySplitter->setStretchFactor(0, 3);
    bodySplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(bodySplitter, 1);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void PerformanceWidget::setupConnections()
{
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onRefresh(); });
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onRefresh(); });
    connect(m_exportBtn, &QPushButton::clicked, this, &PerformanceWidget::onExport);
}

// ─── Generate Mock Data ──────────────────────────────────────────────────────

void PerformanceWidget::generateMockData()
{
    m_data.clear();

    auto* rng = QRandomGenerator::global();
    QDateTime now = QDateTime::currentDateTime();

    // 生成 30 天趋势数据，每小时一个点
    double cpuBase = rng->bounded(20, 50);
    double memBase = rng->bounded(30, 60);
    double tempBase = rng->bounded(38, 50);
    double bwBase = rng->bounded(200, 600);

    double cpuTrend = cpuBase;
    double memTrend = memBase;
    double tempTrend = tempBase;
    double bwTrend = bwBase;

    double cpuDir = (rng->bounded(0, 2) == 0) ? 1.0 : -1.0;
    double memDir = (rng->bounded(0, 2) == 0) ? 1.0 : -1.0;
    double tempDir = (rng->bounded(0, 2) == 0) ? 1.0 : -1.0;
    double bwDir = (rng->bounded(0, 2) == 0) ? 1.0 : -1.0;

    for (int day = 30; day >= 0; --day) {
        for (int hour = 0; hour < 24; ++hour) {
            PerfDataPoint pt;
            pt.timestamp = now.addDays(-day).addSecs(-hour * 3600);

            // 带随机游走的趋势
            cpuTrend += cpuDir * rng->bounded(8) + (rng->bounded(100) - 50) / 20.0;
            cpuTrend = qBound(5.0, cpuTrend, 95.0);
            if (rng->bounded(100) < 15) cpuDir = -cpuDir;

            memTrend += memDir * rng->bounded(6) + (rng->bounded(100) - 50) / 25.0;
            memTrend = qBound(10.0, memTrend, 95.0);
            if (rng->bounded(100) < 12) memDir = -memDir;

            tempTrend += tempDir * rng->bounded(3) + (rng->bounded(100) - 50) / 30.0;
            tempTrend = qBound(30.0, tempTrend, 78.0);
            if (rng->bounded(100) < 10) tempDir = -tempDir;

            bwTrend += bwDir * rng->bounded(50) + (rng->bounded(100) - 50) / 10.0;
            bwTrend = qBound(50.0, bwTrend, 950.0);
            if (rng->bounded(100) < 12) bwDir = -bwDir;

            pt.cpu = cpuTrend;
            pt.mem = memTrend;
            pt.temp = tempTrend;
            pt.bandwidth = bwTrend;
            m_data.append(pt);
        }
    }

    // 按时间排序
    std::sort(m_data.begin(), m_data.end(),
              [](const PerfDataPoint& a, const PerfDataPoint& b) {
                  return a.timestamp < b.timestamp;
              });

    Logger::instance().info("Performance Center",
        QString("已生成 %1 条模拟性能数据").arg(m_data.size()));

    updateTopN();
    onRefresh();
}

// ─── Update Top N ────────────────────────────────────────────────────────────

void PerformanceWidget::updateTopN()
{
    auto* rng = QRandomGenerator::global();
    m_topNTable->setRowCount(0);

    struct TopNEntry {
        QString name;
        double cpu;
        double mem;
        double temp;
        double bandwidth;
    };

    QList<TopNEntry> entries = {
        {"Core-Switch-01",  rng->bounded(30, 80) + rng->generateDouble(), 0, 0, 0},
        {"Core-Router-01",  rng->bounded(20, 70) + rng->generateDouble(), 0, 0, 0},
        {"Access-Switch-02",rng->bounded(10, 50) + rng->generateDouble(), 0, 0, 0},
        {"Firewall-01",     rng->bounded(25, 75) + rng->generateDouble(), 0, 0, 0},
        {"Server-Rack-A",   rng->bounded(40, 90) + rng->generateDouble(), 0, 0, 0},
        {"WLC-01",          rng->bounded(15, 55) + rng->generateDouble(), 0, 0, 0},
        {"Dist-Switch-03",  rng->bounded(10, 45) + rng->generateDouble(), 0, 0, 0},
        {"Edge-Router-02",  rng->bounded(20, 65) + rng->generateDouble(), 0, 0, 0},
    };

    for (auto& e : entries) {
        e.mem = rng->bounded(25, 85) + rng->generateDouble();
        e.temp = rng->bounded(35, 72) + rng->generateDouble();
        e.bandwidth = rng->bounded(30, 95) + rng->generateDouble();
    }

    // 按 CPU 降序排列
    std::sort(entries.begin(), entries.end(),
              [](const TopNEntry& a, const TopNEntry& b) {
                  return a.cpu > b.cpu;
              });

    for (const auto& e : entries) {
        int row = m_topNTable->rowCount();
        m_topNTable->insertRow(row);

        m_topNTable->setItem(row, 0, new QTableWidgetItem(e.name));

        auto makeItem = [](double val, const QString& suffix) -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(QString("%1%2").arg(val, 0, 'f', 1).arg(suffix));
            item->setTextAlignment(Qt::AlignCenter);
            if (val > 80)      item->setForeground(QColor("#F56C6C"));
            else if (val > 60) item->setForeground(QColor("#E6A23C"));
            else               item->setForeground(QColor("#67C23A"));
            return item;
        };

        m_topNTable->setItem(row, 1, makeItem(e.cpu, "%"));
        m_topNTable->setItem(row, 2, makeItem(e.mem, "%"));
        m_topNTable->setItem(row, 3, makeItem(e.temp, "°C"));
        m_topNTable->setItem(row, 4, makeItem(e.bandwidth, "%"));
    }
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void PerformanceWidget::onRefresh()
{
    QString timeRange = m_timeRangeCombo->currentText();
    int hours = 24;
    if (timeRange == "7D")  hours = 7 * 24;
    if (timeRange == "30D") hours = 30 * 24;
    if (timeRange == "90D") hours = 90 * 24;

    // 按时间范围筛选数据
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-hours * 3600);
    QList<PerfDataPoint> filtered;
    for (const auto& p : m_data) {
        if (p.timestamp >= cutoff) {
            filtered.append(p);
        }
    }

    // 如果数据点太多，均匀采样
    int maxPoints = 200;
    if (filtered.size() > maxPoints) {
        QList<PerfDataPoint> sampled;
        double step = (double)(filtered.size() - 1) / (maxPoints - 1);
        for (int i = 0; i < maxPoints; ++i) {
            int idx = qMin((int)(i * step), filtered.size() - 1);
            sampled.append(filtered[idx]);
        }
        filtered = sampled;
    }

    // 更新趋势图
    m_cpuTrendWidget->setData(filtered);
    m_memoryTrendWidget->setData(filtered);
    m_bandwidthTrendWidget->setData(filtered);
    m_temperatureTrendWidget->setData(filtered);

    // 更新历史数据表格
    m_historyTable->setRowCount(0);
    int step = qMax(1, filtered.size() / 50);
    for (int i = 0; i < filtered.size(); i += step) {
        int row = m_historyTable->rowCount();
        if (row >= 50) break;
        m_historyTable->insertRow(row);

        const auto& p = filtered[i];
        m_historyTable->setItem(row, 0, new QTableWidgetItem(
            p.timestamp.toString("yyyy-MM-dd hh:mm")));

        auto makeItem = [](double val, const QString& suffix) -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(QString("%1%2").arg(val, 0, 'f', 1).arg(suffix));
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        m_historyTable->setItem(row, 1, makeItem(p.cpu, "%"));
        m_historyTable->setItem(row, 2, makeItem(p.mem, "%"));
        m_historyTable->setItem(row, 3, makeItem(p.temp, "°C"));
        m_historyTable->setItem(row, 4, makeItem(p.bandwidth, " Mbps"));
    }

    Logger::instance().debug("Performance Center",
        QString("刷新性能数据，时间范围: %1，数据点: %2")
            .arg(timeRange).arg(filtered.size()));
}

// ─── Export CSV ──────────────────────────────────────────────────────────────

void PerformanceWidget::onExport()
{
    if (m_historyTable->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出性能数据",
        QString("performance_center_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("Performance Center", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // 写入 Top N 表格
    stream << "=== Top N 设备排行 ===\n";
    QStringList topNHeaders;
    for (int col = 0; col < m_topNTable->columnCount(); ++col) {
        auto* headerItem = m_topNTable->horizontalHeaderItem(col);
        topNHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << topNHeaders.join(",") << "\n";

    for (int row = 0; row < m_topNTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_topNTable->columnCount(); ++col) {
            auto* item = m_topNTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            rowData << cell;
        }
        stream << rowData.join(",") << "\n";
    }

    // 写入历史数据表格
    stream << "\n=== 历史数据 ===\n";
    QStringList historyHeaders;
    for (int col = 0; col < m_historyTable->columnCount(); ++col) {
        auto* headerItem = m_historyTable->horizontalHeaderItem(col);
        historyHeaders << (headerItem ? headerItem->text() : QString("列%1").arg(col + 1));
    }
    stream << historyHeaders.join(",") << "\n";

    for (int row = 0; row < m_historyTable->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < m_historyTable->columnCount(); ++col) {
            auto* item = m_historyTable->item(row, col);
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

    int totalRows = m_topNTable->rowCount() + m_historyTable->rowCount();
    Logger::instance().info("Performance Center",
        QString("导出 %1 条记录到: %2").arg(totalRows).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(totalRows).arg(filePath));
}

// ─── 趋势绘制适配方法 ────────────────────────────────────────────────────────

void PerformanceWidget::paintCPUTrend()
{
    m_cpuTrendWidget->update();
}

void PerformanceWidget::paintMemoryTrend()
{
    m_memoryTrendWidget->update();
}

void PerformanceWidget::paintBandwidthTrend()
{
    m_bandwidthTrendWidget->update();
}

void PerformanceWidget::paintTemperatureTrend()
{
    m_temperatureTrendWidget->update();
}