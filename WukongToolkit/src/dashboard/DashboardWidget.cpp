#include "dashboard/DashboardWidget.h"
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
#include <QListWidget>
#include <QScrollBar>
#include <QFrame>
#include <QDateTime>
#include <QRandomGenerator>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFont>
#include <QLinearGradient>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
// HealthPieChart
// ═══════════════════════════════════════════════════════════════════════════════

HealthPieChart::HealthPieChart(QWidget* parent)
    : QWidget(parent)
    , m_healthy(0)
    , m_warning(0)
    , m_danger(0)
{
    setMinimumSize(200, 180);
    
}

void HealthPieChart::setData(int healthy, int warning, int danger)
{
    m_healthy = healthy;
    m_warning = warning;
    m_danger = danger;
    update();
}

void HealthPieChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int total = m_healthy + m_warning + m_danger;
    if (total == 0) return;

    int w = width();
    int h = height();
    int side = qMin(w, h) - 20;
    int cx = w / 2;
    int cy = h / 2;
    int outerRadius = side / 2;
    int innerRadius = outerRadius * 0.55;

    // Draw donut ring
    QRectF outerRect(cx - outerRadius, cy - outerRadius, outerRadius * 2, outerRadius * 2);
    QRectF innerRect(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);

    int startAngle = 90 * 16; // 12 o'clock

    // Healthy - green
    if (m_healthy > 0) {
        int spanAngle = static_cast<int>(360.0 * m_healthy / total * 16);
        QPainterPath path;
        path.arcMoveTo(outerRect, startAngle / 16.0);
        path.arcTo(outerRect, startAngle / 16.0, spanAngle / 16.0);
        path.arcTo(innerRect, (startAngle + spanAngle) / 16.0, -spanAngle / 16.0);
        path.closeSubpath();
        painter.setBrush(QColor("#3FB950"));
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);
        startAngle += spanAngle;
    }

    // Warning - yellow
    if (m_warning > 0) {
        int spanAngle = static_cast<int>(360.0 * m_warning / total * 16);
        QPainterPath path;
        path.arcMoveTo(outerRect, startAngle / 16.0);
        path.arcTo(outerRect, startAngle / 16.0, spanAngle / 16.0);
        path.arcTo(innerRect, (startAngle + spanAngle) / 16.0, -spanAngle / 16.0);
        path.closeSubpath();
        painter.setBrush(QColor("#D29922"));
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);
        startAngle += spanAngle;
    }

    // Danger - red
    if (m_danger > 0) {
        int spanAngle = static_cast<int>(360.0 * m_danger / total * 16);
        QPainterPath path;
        path.arcMoveTo(outerRect, startAngle / 16.0);
        path.arcTo(outerRect, startAngle / 16.0, spanAngle / 16.0);
        path.arcTo(innerRect, (startAngle + spanAngle) / 16.0, -spanAngle / 16.0);
        path.closeSubpath();
        painter.setBrush(QColor("#F85149"));
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);
    }

    // Center text
    painter.setPen(QColor("#E6EDF3"));
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRectF(cx - 50, cy - 14, 100, 20), Qt::AlignCenter, "健康度");

    font.setPixelSize(18);
    painter.setFont(font);
    painter.setPen(QColor("#3FB950"));
    double healthPct = total > 0 ? (100.0 * m_healthy / total) : 0;
    painter.drawText(QRectF(cx - 50, cy + 6, 100, 24), Qt::AlignCenter,
                     QString("%1%").arg(static_cast<int>(healthPct)));

    // Legend
    font.setPixelSize(11);
    font.setBold(false);
    painter.setFont(font);

    int legendY = h - 16;
    int legendX = cx - 90;

    painter.setPen(QColor("#3FB950"));
    painter.drawText(QRectF(legendX, legendY, 60, 14), Qt::AlignHCenter,
                     QString("健康 %1").arg(m_healthy));
    painter.setPen(QColor("#D29922"));
    painter.drawText(QRectF(legendX + 60, legendY, 60, 14), Qt::AlignHCenter,
                     QString("警告 %1").arg(m_warning));
    painter.setPen(QColor("#F85149"));
    painter.drawText(QRectF(legendX + 120, legendY, 60, 14), Qt::AlignHCenter,
                     QString("危险 %1").arg(m_danger));
}

// ═══════════════════════════════════════════════════════════════════════════════
// AlertTrendChart
// ═══════════════════════════════════════════════════════════════════════════════

AlertTrendChart::AlertTrendChart(QWidget* parent)
    : QWidget(parent)
    , m_maxValue(0)
{
    setMinimumSize(200, 180);
    
}

void AlertTrendChart::setData(const QList<int>& data)
{
    m_data = data;
    m_maxValue = 0;
    for (int v : m_data) {
        if (v > m_maxValue) m_maxValue = v;
    }
    if (m_maxValue == 0) m_maxValue = 10;
    // Add 20% headroom
    m_maxValue = static_cast<int>(m_maxValue * 1.2);
    if (m_maxValue < 5) m_maxValue = 5;
    update();
}

void AlertTrendChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int marginLeft = 35;
    int marginRight = 15;
    int marginTop = 20;
    int marginBottom = 30;

    int plotW = w - marginLeft - marginRight;
    int plotH = h - marginTop - marginBottom;

    if (plotW <= 0 || plotH <= 0) return;

    // Background
    painter.fillRect(rect(), QColor("#0D1117"));

    // Grid lines
    painter.setPen(QPen(QColor("#21262D"), 1, Qt::DashLine));
    for (int i = 0; i <= 4; ++i) {
        int y = marginTop + plotH * i / 4;
        painter.drawLine(marginLeft, y, w - marginRight, y);

        // Y-axis labels
        painter.setPen(QColor("#8B949E"));
        QFont font = painter.font();
        font.setPixelSize(10);
        painter.setFont(font);
        int val = m_maxValue * (4 - i) / 4;
        painter.drawText(QRectF(0, y - 7, marginLeft - 4, 14),
                         Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
        painter.setPen(QPen(QColor("#21262D"), 1, Qt::DashLine));
    }

    if (m_data.isEmpty()) {
        painter.setPen(QColor("#8B949E"));
        QFont font = painter.font();
        font.setPixelSize(12);
        painter.setFont(font);
        painter.drawText(QRectF(marginLeft, marginTop, plotW, plotH),
                         Qt::AlignCenter, "无数据");
        return;
    }

    int n = m_data.size();
    if (n < 2) return;

    double stepX = static_cast<double>(plotW) / (n - 1);

    // X-axis labels
    painter.setPen(QColor("#8B949E"));
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);
    for (int i = 0; i < n; i += qMax(1, n / 6)) {
        int x = marginLeft + static_cast<int>(i * stepX);
        painter.drawText(QRectF(x - 20, h - marginBottom + 2, 40, 14),
                         Qt::AlignHCenter, QString::number(i + 1));
    }

    // Draw filled area under line
    QPainterPath areaPath;
    double x0 = marginLeft;
    double yBase = marginTop + plotH;
    areaPath.moveTo(x0, yBase);

    for (int i = 0; i < n; ++i) {
        double x = marginLeft + i * stepX;
        double y = marginTop + plotH - (static_cast<double>(m_data[i]) / m_maxValue * plotH);
        areaPath.lineTo(x, y);
    }
    double xEnd = marginLeft + (n - 1) * stepX;
    areaPath.lineTo(xEnd, yBase);
    areaPath.closeSubpath();

    QLinearGradient areaGrad(0, marginTop, 0, marginTop + plotH);
    areaGrad.setColorAt(0, QColor(64, 158, 255, 80));
    areaGrad.setColorAt(1, QColor(64, 158, 255, 10));
    painter.setBrush(areaGrad);
    painter.setPen(Qt::NoPen);
    painter.drawPath(areaPath);

    // Draw line
    QPainterPath linePath;
    for (int i = 0; i < n; ++i) {
        double x = marginLeft + i * stepX;
        double y = marginTop + plotH - (static_cast<double>(m_data[i]) / m_maxValue * plotH);
        if (i == 0)
            linePath.moveTo(x, y);
        else
            linePath.lineTo(x, y);
    }
    painter.setPen(QPen(QColor("#58A6FF"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(linePath);

    // Draw dots
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < n; ++i) {
        double x = marginLeft + i * stepX;
        double y = marginTop + plotH - (static_cast<double>(m_data[i]) / m_maxValue * plotH);
        painter.setBrush(QColor("#58A6FF"));
        painter.drawEllipse(QPointF(x, y), 3, 3);
    }

    // Title
    painter.setPen(QColor("#58A6FF"));
    QFont titleFont = painter.font();
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(marginLeft, 2, plotW, 16), Qt::AlignLeft, "告警趋势 (24h)");
}

// ═══════════════════════════════════════════════════════════════════════════════
// TrafficAreaChart
// ═══════════════════════════════════════════════════════════════════════════════

TrafficAreaChart::TrafficAreaChart(QWidget* parent)
    : QWidget(parent)
    , m_maxValue(0)
{
    setMinimumSize(200, 180);
    
}

void TrafficAreaChart::setData(const QList<double>& inData, const QList<double>& outData)
{
    m_inData = inData;
    m_outData = outData;
    m_maxValue = 0;
    for (double v : m_inData) {
        if (v > m_maxValue) m_maxValue = v;
    }
    for (double v : m_outData) {
        if (v > m_maxValue) m_maxValue = v;
    }
    if (m_maxValue == 0) m_maxValue = 100;
    m_maxValue *= 1.2;
    update();
}

void TrafficAreaChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int marginLeft = 40;
    int marginRight = 15;
    int marginTop = 20;
    int marginBottom = 30;

    int plotW = w - marginLeft - marginRight;
    int plotH = h - marginTop - marginBottom;

    if (plotW <= 0 || plotH <= 0) return;

    // Background
    painter.fillRect(rect(), QColor("#0D1117"));

    // Grid lines
    painter.setPen(QPen(QColor("#21262D"), 1, Qt::DashLine));
    for (int i = 0; i <= 4; ++i) {
        int y = marginTop + plotH * i / 4;
        painter.drawLine(marginLeft, y, w - marginRight, y);

        painter.setPen(QColor("#8B949E"));
        QFont font = painter.font();
        font.setPixelSize(10);
        painter.setFont(font);
        double val = m_maxValue * (4 - i) / 4;
        QString label;
        if (val >= 1000) {
            label = QString("%1G").arg(val / 1000, 0, 'f', 1);
        } else {
            label = QString("%1M").arg(static_cast<int>(val));
        }
        painter.drawText(QRectF(0, y - 7, marginLeft - 4, 14),
                         Qt::AlignRight | Qt::AlignVCenter, label);
        painter.setPen(QPen(QColor("#21262D"), 1, Qt::DashLine));
    }

    if (m_inData.isEmpty() && m_outData.isEmpty()) {
        painter.setPen(QColor("#8B949E"));
        QFont font = painter.font();
        font.setPixelSize(12);
        painter.setFont(font);
        painter.drawText(QRectF(marginLeft, marginTop, plotW, plotH),
                         Qt::AlignCenter, "无数据");
        return;
    }

    int n = qMax(m_inData.size(), m_outData.size());
    if (n < 2) return;

    double stepX = static_cast<double>(plotW) / (n - 1);

    // X-axis labels
    painter.setPen(QColor("#8B949E"));
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);
    for (int i = 0; i < n; i += qMax(1, n / 6)) {
        int x = marginLeft + static_cast<int>(i * stepX);
        painter.drawText(QRectF(x - 20, h - marginBottom + 2, 40, 14),
                         Qt::AlignHCenter, QString::number(i + 1));
    }

    // Draw inbound area
    if (m_inData.size() >= 2) {
        QPainterPath inArea;
        double yBase = marginTop + plotH;
        inArea.moveTo(marginLeft, yBase);
        for (int i = 0; i < m_inData.size(); ++i) {
            double x = marginLeft + i * stepX;
            double y = marginTop + plotH - (m_inData[i] / m_maxValue * plotH);
            inArea.lineTo(x, y);
        }
        inArea.lineTo(marginLeft + (m_inData.size() - 1) * stepX, yBase);
        inArea.closeSubpath();

        QLinearGradient inGrad(0, marginTop, 0, marginTop + plotH);
        inGrad.setColorAt(0, QColor(64, 158, 255, 100));
        inGrad.setColorAt(1, QColor(64, 158, 255, 10));
        painter.setBrush(inGrad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(inArea);
    }

    // Draw outbound area
    if (m_outData.size() >= 2) {
        QPainterPath outArea;
        double yBase = marginTop + plotH;
        outArea.moveTo(marginLeft, yBase);
        for (int i = 0; i < m_outData.size(); ++i) {
            double x = marginLeft + i * stepX;
            double y = marginTop + plotH - (m_outData[i] / m_maxValue * plotH);
            outArea.lineTo(x, y);
        }
        outArea.lineTo(marginLeft + (m_outData.size() - 1) * stepX, yBase);
        outArea.closeSubpath();

        QLinearGradient outGrad(0, marginTop, 0, marginTop + plotH);
        outGrad.setColorAt(0, QColor(103, 194, 58, 100));
        outGrad.setColorAt(1, QColor(103, 194, 58, 10));
        painter.setBrush(outGrad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(outArea);
    }

    // Draw inbound line
    if (m_inData.size() >= 2) {
        QPainterPath inLine;
        for (int i = 0; i < m_inData.size(); ++i) {
            double x = marginLeft + i * stepX;
            double y = marginTop + plotH - (m_inData[i] / m_maxValue * plotH);
            if (i == 0)
                inLine.moveTo(x, y);
            else
                inLine.lineTo(x, y);
        }
        painter.setPen(QPen(QColor("#58A6FF"), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(inLine);
    }

    // Draw outbound line
    if (m_outData.size() >= 2) {
        QPainterPath outLine;
        for (int i = 0; i < m_outData.size(); ++i) {
            double x = marginLeft + i * stepX;
            double y = marginTop + plotH - (m_outData[i] / m_maxValue * plotH);
            if (i == 0)
                outLine.moveTo(x, y);
            else
                outLine.lineTo(x, y);
        }
        painter.setPen(QPen(QColor("#3FB950"), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(outLine);
    }

    // Title
    painter.setPen(QColor("#58A6FF"));
    QFont titleFont = painter.font();
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(marginLeft, 2, plotW, 16), Qt::AlignLeft, "全网流量趋势");

    // Legend
    painter.setPen(QColor("#58A6FF"));
    QFont legendFont = painter.font();
    legendFont.setPixelSize(10);
    legendFont.setBold(false);
    painter.setFont(legendFont);
    painter.drawText(QRectF(marginLeft + plotW - 120, 2, 50, 16), Qt::AlignRight, "入流量");
    painter.setPen(QColor("#3FB950"));
    painter.drawText(QRectF(marginLeft + plotW - 60, 2, 50, 16), Qt::AlignRight, "出流量");
}

// ═══════════════════════════════════════════════════════════════════════════════
// DashboardWidget
// ═══════════════════════════════════════════════════════════════════════════════

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
{
    setupUI();
    setupConnections();
    generateMockData();
    updateDeviceStatus();
}

DashboardWidget::~DashboardWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void DashboardWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──

    auto styleCard = [](QFrame* card, const QString& accentColor) {
        card->setStyleSheet(
            QString("QFrame { border-top: 3px solid %1; }").arg(accentColor)
        );
    };

    auto configTable = [](QTableWidget* table) {
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString("QPushButton { background-color: %1; color: white; border: none; }"
                    "QPushButton:hover { background-color: %2; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto makeStatusCard = [&](const QString& icon, const QString& title, const QString& accentColor) -> QLabel* {
        auto* card = new QFrame();
        styleCard(card, accentColor);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(4);

        auto* header = new QHBoxLayout();
        auto* iconLabel = new QLabel(icon);
        iconLabel->setStyleSheet("font-size: 16px; background: transparent; border: none;");
        auto* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(
            QString("font-size: 11px; color: #8B949E; background: transparent; border: none;"));
        header->addWidget(iconLabel);
        header->addWidget(titleLabel);
        header->addStretch();

        auto* valueLabel = new QLabel("--");
        valueLabel->setStyleSheet(
            QString("font-size: 24px; color: %1; font-weight: bold; background: transparent; border: none;")
                .arg(accentColor));
        valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        cardLayout->addLayout(header);
        cardLayout->addWidget(valueLabel);

        return valueLabel;
    };

    // ═══════════════════════════════════════════════════════════════════════════
    // 顶部: 全局状态卡片 (6个)
    // ═══════════════════════════════════════════════════════════════════════════

    auto* topGroup = new QGroupBox("Dashboard");
    topGroup;
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(8);

    m_onlineDevicesLabel = makeStatusCard("🟢", "在线设备", "#3FB950");
    topLayout->addWidget(m_onlineDevicesLabel->parentWidget());

    m_totalDevicesLabel = makeStatusCard("📡", "总设备", "#58A6FF");
    topLayout->addWidget(m_totalDevicesLabel->parentWidget());

    m_alertCountLabel = makeStatusCard("🔔", "告警数", "#F85149");
    topLayout->addWidget(m_alertCountLabel->parentWidget());

    m_cpuAvgLabel = makeStatusCard("🖥", "CPU% 均值", "#D29922");
    topLayout->addWidget(m_cpuAvgLabel->parentWidget());

    m_memAvgLabel = makeStatusCard("💾", "内存% 均值", "#8B949E");
    topLayout->addWidget(m_memAvgLabel->parentWidget());

    m_bandwidthAvgLabel = makeStatusCard("🌐", "带宽利用率", "#3FB950");
    topLayout->addWidget(m_bandwidthAvgLabel->parentWidget());

    mainLayout->addWidget(topGroup);

    // ═══════════════════════════════════════════════════════════════════════════
    // 中部: 图表区 (三栏)
    // ═══════════════════════════════════════════════════════════════════════════

    auto* chartGroup = new QGroupBox("监控图表");
    chartGroup;
    auto* chartLayout = new QHBoxLayout(chartGroup);
    chartLayout->setSpacing(8);

    // 健康度饼图
    auto* healthBox = new QGroupBox();
    auto* healthLayout = new QVBoxLayout(healthBox);
    healthLayout->setContentsMargins(4, 4, 4, 4);
    m_healthPieChart = new HealthPieChart();
    healthLayout->addWidget(m_healthPieChart);
    chartLayout->addWidget(healthBox);

    // 告警趋势图
    auto* alertBox = new QGroupBox();
    auto* alertLayout = new QVBoxLayout(alertBox);
    alertLayout->setContentsMargins(4, 4, 4, 4);
    m_alertTrendChart = new AlertTrendChart();
    alertLayout->addWidget(m_alertTrendChart);
    chartLayout->addWidget(alertBox);

    // 全网流量面积图
    auto* trafficBox = new QGroupBox();
    auto* trafficLayout = new QVBoxLayout(trafficBox);
    trafficLayout->setContentsMargins(4, 4, 4, 4);
    m_trafficAreaChart = new TrafficAreaChart();
    trafficLayout->addWidget(m_trafficAreaChart);
    chartLayout->addWidget(trafficBox);

    mainLayout->addWidget(chartGroup, 1);

    // ═══════════════════════════════════════════════════════════════════════════
    // 底部: 设备状态表 + 实时告警 + Top告警设备
    // ═══════════════════════════════════════════════════════════════════════════

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(8);

    // ── 左侧: 设备状态表 + 快捷操作 ──
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    auto* deviceGroup = new QGroupBox("设备状态");
    deviceGroup;
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    m_deviceTable = new QTableWidget(0, 8);
    m_deviceTable->setHorizontalHeaderLabels(
        {"设备名", "IP", "类型", "CPU%", "内存%", "带宽%", "状态", "最后检测"});
    m_deviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_deviceTable->setColumnWidth(2, 80);
    m_deviceTable->setColumnWidth(3, 60);
    m_deviceTable->setColumnWidth(4, 60);
    m_deviceTable->setColumnWidth(5, 60);
    m_deviceTable->setColumnWidth(6, 60);
    m_deviceTable->setColumnWidth(7, 140);
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->setMinimumHeight(200);
    configTable(m_deviceTable);
    deviceLayout->addWidget(m_deviceTable);
    leftLayout->addWidget(deviceGroup, 1);

    // 快捷操作按钮
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    m_pingBtn = new QPushButton("Ping");
    styleButton(m_pingBtn, "#58A6FF", "#79C0FF");
    m_pingBtn->setProperty("action", "ping");

    m_inspectionBtn = new QPushButton("巡检");
    styleButton(m_inspectionBtn, "#3FB950", "#56D364");
    m_inspectionBtn->setProperty("action", "inspection");

    m_backupBtn = new QPushButton("备份");
    styleButton(m_backupBtn, "#D29922", "#DBAB4A");
    m_backupBtn->setProperty("action", "backup");

    m_scanBtn = new QPushButton("扫描");
    styleButton(m_scanBtn, "#8B949E", "#B4B4B4");
    m_scanBtn->setProperty("action", "scan");

    m_exportBtn = new QPushButton("导出");
    styleButton(m_exportBtn, "#F85149", "#FF7B72");

    actionLayout->addWidget(m_pingBtn);
    actionLayout->addWidget(m_inspectionBtn);
    actionLayout->addWidget(m_backupBtn);
    actionLayout->addWidget(m_scanBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_exportBtn);
    leftLayout->addLayout(actionLayout);

    bottomLayout->addWidget(leftWidget, 3);

    // ── 右侧: 实时告警滚动 + Top告警设备 ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    // 实时告警滚动区
    auto* alertScrollGroup = new QGroupBox("实时告警");
    alertScrollGroup;
    auto* alertScrollLayout = new QVBoxLayout(alertScrollGroup);
    m_alertListWidget = new QListWidget();
    m_alertListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_alertListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_alertListWidget->setMaximumHeight(150);
    m_alertListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    alertScrollLayout->addWidget(m_alertListWidget);
    rightLayout->addWidget(alertScrollGroup);

    // Top 告警设备表
    auto* topAlertGroup = new QGroupBox("Top 告警设备");
    topAlertGroup;
    auto* topAlertLayout = new QVBoxLayout(topAlertGroup);
    m_topAlertTable = new QTableWidget(0, 4);
    m_topAlertTable->setHorizontalHeaderLabels(
        {"设备名", "告警数", "严重告警", "最后告警时间"});
    m_topAlertTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_topAlertTable->setColumnWidth(1, 60);
    m_topAlertTable->setColumnWidth(2, 70);
    m_topAlertTable->setColumnWidth(3, 140);
    m_topAlertTable->verticalHeader()->setVisible(false);
    configTable(m_topAlertTable);
    topAlertLayout->addWidget(m_topAlertTable);
    rightLayout->addWidget(topAlertGroup, 1);

    bottomLayout->addWidget(rightWidget, 1);

    mainLayout->addLayout(bottomLayout, 2);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void DashboardWidget::setupConnections()
{
    connect(m_pingBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickAction);
    connect(m_inspectionBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickAction);
    connect(m_backupBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickAction);
    connect(m_scanBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickAction);
    connect(m_exportBtn, &QPushButton::clicked, this, &DashboardWidget::onExport);
}

// ─── Generate Mock Data ──────────────────────────────────────────────────────

void DashboardWidget::generateMockData()
{
    auto* rng = QRandomGenerator::global();

    // ── 20 个设备 ──
    m_data.clear();

    struct DeviceTemplate {
        QString name;
        QString ip;
        QString type;
    };

    QList<DeviceTemplate> devices = {
        {"Core-SW-01", "192.168.1.1", "交换机"},
        {"Core-SW-02", "192.168.1.2", "交换机"},
        {"Core-RT-01", "192.168.1.10", "路由器"},
        {"Core-RT-02", "192.168.1.11", "路由器"},
        {"FW-01", "192.168.1.100", "防火墙"},
        {"FW-02", "192.168.1.101", "防火墙"},
        {"ACC-SW-01", "192.168.2.1", "交换机"},
        {"ACC-SW-02", "192.168.2.2", "交换机"},
        {"ACC-SW-03", "192.168.2.3", "交换机"},
        {"ACC-SW-04", "192.168.2.4", "交换机"},
        {"AP-01", "192.168.3.10", "无线AP"},
        {"AP-02", "192.168.3.11", "无线AP"},
        {"AP-03", "192.168.3.12", "无线AP"},
        {"AP-04", "192.168.3.13", "无线AP"},
        {"SRV-DB-01", "192.168.4.10", "服务器"},
        {"SRV-DB-02", "192.168.4.11", "服务器"},
        {"SRV-APP-01", "192.168.4.20", "服务器"},
        {"SRV-APP-02", "192.168.4.21", "服务器"},
        {"SRV-WEB-01", "192.168.4.30", "服务器"},
        {"SRV-WEB-02", "192.168.4.31", "服务器"},
    };

    QStringList statuses = {"Online", "Online", "Online", "Online", "Online",
                            "Online", "Online", "Warning", "Offline"};
    QStringList alertDeviceNames = {"Core-SW-01", "Core-RT-01", "FW-01", "SRV-DB-01",
                                    "SRV-WEB-01", "ACC-SW-03", "AP-02", "FW-02"};

    for (int i = 0; i < devices.size(); ++i) {
        DashboardData d;
        d.name = devices[i].name;
        d.ip = devices[i].ip;
        d.type = devices[i].type;
        d.cpuUsage = rng->bounded(5, 95);
        d.memUsage = rng->bounded(10, 90);
        d.bandwidthUsage = rng->bounded(5, 95);
        d.status = statuses[rng->bounded(statuses.size())];

        // Ensure at least some devices have issues
        if (i == 17) d.status = "Offline";
        if (i == 15) d.status = "Warning";
        if (i == 7) d.status = "Warning";

        d.lastCheck = QDateTime::currentDateTime()
                          .addSecs(-rng->bounded(0, 300))
                          .toString("yyyy-MM-dd hh:mm:ss");

        // Assign alerts to some devices
        if (alertDeviceNames.contains(d.name)) {
            d.alertCount = rng->bounded(3, 25);
            d.criticalAlerts = rng->bounded(0, d.alertCount / 2);
        } else {
            d.alertCount = rng->bounded(0, 3);
            d.criticalAlerts = d.alertCount > 0 ? rng->bounded(0, d.alertCount) : 0;
        }

        d.lastAlertTime = d.alertCount > 0
            ? QDateTime::currentDateTime()
                  .addSecs(-rng->bounded(60, 86400))
                  .toString("yyyy-MM-dd hh:mm:ss")
            : "--";

        m_data.append(d);
    }

    // ── 100 条告警趋势数据 ──
    m_alertTrendData.clear();
    for (int i = 0; i < 100; ++i) {
        // Simulate a daily pattern: higher during day, lower at night
        int hour = (i * 24 / 100);
        int base = (hour >= 8 && hour <= 20) ? 15 : 5;
        m_alertTrendData.append(base + rng->bounded(0, 20));
    }

    // ── 流量趋势数据 ──
    m_trafficInData.clear();
    m_trafficOutData.clear();
    for (int i = 0; i < 100; ++i) {
        int hour = (i * 24 / 100);
        double base = (hour >= 8 && hour <= 20) ? 500 : 200;
        m_trafficInData.append(base + rng->bounded(0, 300));
        m_trafficOutData.append(base * 0.6 + rng->bounded(0, 200));
    }

    // ── 最近告警列表 (模拟实时告警) ──
    m_recentAlerts.clear();
    QStringList alertLevels = {"Info", "Info", "Warning", "Warning", "Critical", "Emergency"};
    QStringList alertMsgs = {
        "CPU 使用率超过阈值 85%",
        "内存使用率超过阈值 90%",
        "接口 GigabitEthernet0/1 Down",
        "设备 Ping 超时，疑似离线",
        "磁盘空间不足，剩余 5%",
        "温度过高，当前 72°C",
        "SNMP 认证失败",
        "BGP 邻居 Down",
        "流量异常，超过基线 200%",
        "端口安全违规 MAC 地址"
    };

    for (int i = 0; i < 20; ++i) {
        QStringList alert;
        alert << QDateTime::currentDateTime()
                     .addSecs(-rng->bounded(10, 3600))
                     .toString("hh:mm:ss");
        alert << alertLevels[rng->bounded(alertLevels.size())];
        alert << devices[rng->bounded(devices.size())].name;
        alert << alertMsgs[rng->bounded(alertMsgs.size())];
        m_recentAlerts.append(alert);
    }

    // Sort by time descending
    std::sort(m_recentAlerts.begin(), m_recentAlerts.end(),
              [](const QStringList& a, const QStringList& b) {
                  return a[0] > b[0];
              });

    Logger::instance().info("Dashboard",
        QString("模拟数据已生成: %1 个设备, %2 条告警趋势, %3 条实时告警")
            .arg(m_data.size()).arg(m_alertTrendData.size()).arg(m_recentAlerts.size()));
}

// ─── Update Device Status ────────────────────────────────────────────────────

void DashboardWidget::updateDeviceStatus()
{
    // ── 统计全局状态 ──
    int onlineCount = 0;
    int totalDevices = m_data.size();
    int totalAlerts = 0;
    double cpuSum = 0;
    double memSum = 0;
    double bwSum = 0;
    int healthyCount = 0;
    int warningCount = 0;
    int dangerCount = 0;

    for (const auto& d : m_data) {
        if (d.status == "Online") {
            onlineCount++;
            healthyCount++;
        } else if (d.status == "Warning") {
            warningCount++;
        } else {
            dangerCount++;
        }
        totalAlerts += d.alertCount;
        cpuSum += d.cpuUsage;
        memSum += d.memUsage;
        bwSum += d.bandwidthUsage;
    }

    int cpuAvg = totalDevices > 0 ? static_cast<int>(cpuSum / totalDevices) : 0;
    int memAvg = totalDevices > 0 ? static_cast<int>(memSum / totalDevices) : 0;
    int bwAvg = totalDevices > 0 ? static_cast<int>(bwSum / totalDevices) : 0;

    // ── 更新全局状态卡片 ──
    m_onlineDevicesLabel->setText(QString::number(onlineCount));
    m_totalDevicesLabel->setText(QString::number(totalDevices));
    m_alertCountLabel->setText(QString::number(totalAlerts));
    m_cpuAvgLabel->setText(QString("%1%").arg(cpuAvg));
    m_memAvgLabel->setText(QString("%1%").arg(memAvg));
    m_bandwidthAvgLabel->setText(QString("%1%").arg(bwAvg));

    // Alert count color
    if (totalAlerts > 10) {
        m_alertCountLabel->setStyleSheet(
            "font-size: 24px; color: #F85149; font-weight: bold; background: transparent; border: none;");
    } else if (totalAlerts > 5) {
        m_alertCountLabel->setStyleSheet(
            "font-size: 24px; color: #D29922; font-weight: bold; background: transparent; border: none;");
    } else {
        m_alertCountLabel->setStyleSheet(
            "font-size: 24px; color: #3FB950; font-weight: bold; background: transparent; border: none;");
    }

    // CPU avg color
    if (cpuAvg > 80) {
        m_cpuAvgLabel->setStyleSheet(
            "font-size: 24px; color: #F85149; font-weight: bold; background: transparent; border: none;");
    } else if (cpuAvg > 60) {
        m_cpuAvgLabel->setStyleSheet(
            "font-size: 24px; color: #D29922; font-weight: bold; background: transparent; border: none;");
    } else {
        m_cpuAvgLabel->setStyleSheet(
            "font-size: 24px; color: #D29922; font-weight: bold; background: transparent; border: none;");
    }

    // ── 更新健康度饼图 ──
    m_healthPieChart->setData(healthyCount, warningCount, dangerCount);

    // ── 更新告警趋势图 ──
    m_alertTrendChart->setData(m_alertTrendData);

    // ── 更新流量面积图 ──
    m_trafficAreaChart->setData(m_trafficInData, m_trafficOutData);

    // ── 更新设备状态表 ──
    m_deviceTable->setRowCount(0);
    for (const auto& d : m_data) {
        int row = m_deviceTable->rowCount();
        m_deviceTable->insertRow(row);

        auto* nameItem = new QTableWidgetItem(d.name);
        nameItem->setTextAlignment(Qt::AlignCenter);
        m_deviceTable->setItem(row, 0, nameItem);

        auto* ipItem = new QTableWidgetItem(d.ip);
        ipItem->setTextAlignment(Qt::AlignCenter);
        m_deviceTable->setItem(row, 1, ipItem);

        auto* typeItem = new QTableWidgetItem(d.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        m_deviceTable->setItem(row, 2, typeItem);

        auto* cpuItem = new QTableWidgetItem(QString("%1%").arg(d.cpuUsage));
        cpuItem->setTextAlignment(Qt::AlignCenter);
        if (d.cpuUsage > 80) cpuItem->setForeground(QColor("#F85149"));
        else if (d.cpuUsage > 60) cpuItem->setForeground(QColor("#D29922"));
        else cpuItem->setForeground(QColor("#3FB950"));
        m_deviceTable->setItem(row, 3, cpuItem);

        auto* memItem = new QTableWidgetItem(QString("%1%").arg(d.memUsage));
        memItem->setTextAlignment(Qt::AlignCenter);
        if (d.memUsage > 85) memItem->setForeground(QColor("#F85149"));
        else if (d.memUsage > 70) memItem->setForeground(QColor("#D29922"));
        else memItem->setForeground(QColor("#3FB950"));
        m_deviceTable->setItem(row, 4, memItem);

        auto* bwItem = new QTableWidgetItem(QString("%1%").arg(d.bandwidthUsage));
        bwItem->setTextAlignment(Qt::AlignCenter);
        if (d.bandwidthUsage > 80) bwItem->setForeground(QColor("#F85149"));
        else if (d.bandwidthUsage > 60) bwItem->setForeground(QColor("#D29922"));
        else bwItem->setForeground(QColor("#3FB950"));
        m_deviceTable->setItem(row, 5, bwItem);

        auto* statusItem = new QTableWidgetItem(d.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (d.status == "Online") {
            statusItem->setForeground(QColor("#3FB950"));
        } else if (d.status == "Warning") {
            statusItem->setForeground(QColor("#D29922"));
        } else {
            statusItem->setForeground(QColor("#F85149"));
        }
        m_deviceTable->setItem(row, 6, statusItem);

        auto* checkItem = new QTableWidgetItem(d.lastCheck);
        checkItem->setTextAlignment(Qt::AlignCenter);
        m_deviceTable->setItem(row, 7, checkItem);
    }

    // ── 更新实时告警列表 (最近10条) ──
    m_alertListWidget->clear();
    int alertCount = qMin(10, m_recentAlerts.size());
    for (int i = 0; i < alertCount; ++i) {
        const auto& a = m_recentAlerts[i];
        QString time = a[0];
        QString level = a[1];
        QString device = a[2];
        QString msg = a[3];

        QString levelColor;
        QString levelIcon;
        if (level == "Critical" || level == "Emergency") {
            levelColor = "#F85149";
            levelIcon = "🔴";
        } else if (level == "Warning") {
            levelColor = "#D29922";
            levelIcon = "🟡";
        } else {
            levelColor = "#58A6FF";
            levelIcon = "🔵";
        }

        QString text = QString("<span style='color:#8C8C8C;'>[%1]</span> "
                               "<span style='color:%2;'>%3 %4</span> "
                               "<span style='color:#E6EDF3;'>%5</span> - "
                               "<span style='color:#909399;'>%6</span>")
                           .arg(time, levelColor, levelIcon, level, device, msg);

        auto* item = new QListWidgetItem();
        auto* label = new QLabel(text);
        label->setTextFormat(Qt::RichText);
        item->setSizeHint(label->sizeHint());
        m_alertListWidget->addItem(item);
        m_alertListWidget->setItemWidget(item, label);
    }

    // Auto-scroll to top
    if (m_alertListWidget->count() > 0) {
        m_alertListWidget->scrollToTop();
    }

    // ── 更新 Top 告警设备表 ──
    // Sort by alert count descending
    QList<DashboardData> sorted = m_data;
    std::sort(sorted.begin(), sorted.end(),
              [](const DashboardData& a, const DashboardData& b) {
                  return a.alertCount > b.alertCount;
              });

    m_topAlertTable->setRowCount(0);
    int topCount = qMin(10, sorted.size());
    for (int i = 0; i < topCount; ++i) {
        if (sorted[i].alertCount == 0) break;

        int row = m_topAlertTable->rowCount();
        m_topAlertTable->insertRow(row);

        auto* nameItem = new QTableWidgetItem(sorted[i].name);
        nameItem->setTextAlignment(Qt::AlignCenter);
        m_topAlertTable->setItem(row, 0, nameItem);

        auto* countItem = new QTableWidgetItem(QString::number(sorted[i].alertCount));
        countItem->setTextAlignment(Qt::AlignCenter);
        if (sorted[i].alertCount > 10) countItem->setForeground(QColor("#F85149"));
        else if (sorted[i].alertCount > 5) countItem->setForeground(QColor("#D29922"));
        m_topAlertTable->setItem(row, 1, countItem);

        auto* criticalItem = new QTableWidgetItem(QString::number(sorted[i].criticalAlerts));
        criticalItem->setTextAlignment(Qt::AlignCenter);
        if (sorted[i].criticalAlerts > 0) criticalItem->setForeground(QColor("#F85149"));
        m_topAlertTable->setItem(row, 2, criticalItem);

        auto* timeItem = new QTableWidgetItem(sorted[i].lastAlertTime);
        timeItem->setTextAlignment(Qt::AlignCenter);
        m_topAlertTable->setItem(row, 3, timeItem);
    }

    Logger::instance().debug("Dashboard",
        QString("设备状态已更新: 在线=%1/%2, 告警=%3, CPU均值=%4%, 内存均值=%5%")
            .arg(onlineCount).arg(totalDevices).arg(totalAlerts).arg(cpuAvg).arg(memAvg));
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void DashboardWidget::onRefresh()
{
    Logger::instance().info("Dashboard", "手动刷新 Dashboard 数据");

    // Regenerate mock data with slight randomization
    auto* rng = QRandomGenerator::global();

    for (auto& d : m_data) {
        // Randomly fluctuate CPU/Mem/Bandwidth
        d.cpuUsage = qBound(1, d.cpuUsage + rng->bounded(-5, 6), 100);
        d.memUsage = qBound(1, d.memUsage + rng->bounded(-3, 4), 100);
        d.bandwidthUsage = qBound(1, d.bandwidthUsage + rng->bounded(-4, 5), 100);

        // Occasionally change status
        if (rng->bounded(100) < 5) {
            QStringList statuses = {"Online", "Online", "Online", "Warning", "Offline"};
            d.status = statuses[rng->bounded(statuses.size())];
        }

        d.lastCheck = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        // Occasionally update alerts
        if (rng->bounded(100) < 10) {
            d.alertCount = qMax(0, d.alertCount + rng->bounded(-1, 3));
            d.criticalAlerts = qMin(d.criticalAlerts + rng->bounded(-1, 2), d.alertCount);
            if (d.alertCount > 0) {
                d.lastAlertTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            }
        }
    }

    // Update alert trend - remove oldest, add new
    if (!m_alertTrendData.isEmpty()) {
        m_alertTrendData.removeFirst();
        m_alertTrendData.append(rng->bounded(0, 30));
    }

    // Update traffic data - remove oldest, add new
    if (!m_trafficInData.isEmpty()) {
        m_trafficInData.removeFirst();
        m_trafficOutData.removeFirst();
        double lastIn = m_trafficInData.isEmpty() ? 500 : m_trafficInData.last();
        double lastOut = m_trafficOutData.isEmpty() ? 300 : m_trafficOutData.last();
        m_trafficInData.append(lastIn + rng->bounded(-50, 51));
        m_trafficOutData.append(lastOut + rng->bounded(-30, 31));
    }

    updateDeviceStatus();
}

// ─── Export ──────────────────────────────────────────────────────────────────

void DashboardWidget::onExport()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 Dashboard 数据",
        QString("dashboard_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("Dashboard", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // Write device status
    stream << "=== 设备状态 ===\n";
    stream << "设备名,IP,类型,CPU%,内存%,带宽%,状态,告警数,严重告警,最后检测\n";
    for (const auto& d : m_data) {
        stream << d.name << "," << d.ip << "," << d.type << ","
               << d.cpuUsage << "," << d.memUsage << "," << d.bandwidthUsage << ","
               << d.status << "," << d.alertCount << "," << d.criticalAlerts << ","
               << d.lastCheck << "\n";
    }

    // Write summary
    int online = 0, totalAlerts = 0;
    double cpuSum = 0, memSum = 0;
    for (const auto& d : m_data) {
        if (d.status == "Online") online++;
        totalAlerts += d.alertCount;
        cpuSum += d.cpuUsage;
        memSum += d.memUsage;
    }
    stream << "\n=== 汇总统计 ===\n";
    stream << "在线设备," << online << "/" << m_data.size() << "\n";
    stream << "总告警数," << totalAlerts << "\n";
    stream << "CPU均值," << static_cast<int>(cpuSum / m_data.size()) << "%\n";
    stream << "内存均值," << static_cast<int>(memSum / m_data.size()) << "%\n";

    file.close();

    Logger::instance().info("Dashboard",
        QString("已导出 %1 条设备记录到: %2").arg(m_data.size()).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条设备记录到:\n%2").arg(m_data.size()).arg(filePath));
}

// ─── Quick Action ────────────────────────────────────────────────────────────

void DashboardWidget::onQuickAction()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString action = btn->property("action").toString();
    QString actionName;

    if (action == "ping") actionName = "Ping 全网检测";
    else if (action == "inspection") actionName = "全网巡检";
    else if (action == "backup") actionName = "配置备份";
    else if (action == "scan") actionName = "网络扫描";
    else return;

    Logger::instance().info("Dashboard",
        QString("触发快捷操作: %1").arg(actionName));

    int onlineCount = 0;
    for (const auto& d : m_data) {
        if (d.status == "Online") onlineCount++;
    }

    QMessageBox::information(this, actionName,
        QString("<b>%1</b> 已触发<br><br>"
                "目标设备: %2 台 (在线: %3 台)<br>"
                "请前往对应模块查看详情。")
            .arg(actionName)
            .arg(m_data.size())
            .arg(onlineCount));
}

// ─── Paint forwarding (deprecated - charts are self-painting) ─────────────────

void DashboardWidget::paintHealthPie() {}
void DashboardWidget::paintAlertTrend() {}
void DashboardWidget::paintTrafficArea() {}