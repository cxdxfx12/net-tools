#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QPainter>

// ============================================================================
// DashboardData — 设备状态数据
// ============================================================================
struct DashboardData
{
    QString name;
    QString ip;
    QString type;
    int cpuUsage;
    int memUsage;
    int bandwidthUsage;
    QString status;       // Online / Warning / Offline
    QString lastCheck;
    int alertCount;
    int criticalAlerts;
    QString lastAlertTime;
};

// ============================================================================
// HealthPieChart — 设备健康度环形饼图
// ============================================================================
class HealthPieChart : public QWidget
{
    Q_OBJECT

public:
    explicit HealthPieChart(QWidget* parent = nullptr);

    void setData(int healthy, int warning, int danger);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_healthy;
    int m_warning;
    int m_danger;
};

// ============================================================================
// AlertTrendChart — 告警趋势折线图
// ============================================================================
class AlertTrendChart : public QWidget
{
    Q_OBJECT

public:
    explicit AlertTrendChart(QWidget* parent = nullptr);

    void setData(const QList<int>& data);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<int> m_data;
    int m_maxValue;
};

// ============================================================================
// TrafficAreaChart — 全网流量趋势面积图
// ============================================================================
class TrafficAreaChart : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficAreaChart(QWidget* parent = nullptr);

    void setData(const QList<double>& inData, const QList<double>& outData);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<double> m_inData;
    QList<double> m_outData;
    double m_maxValue;
};

// ============================================================================
// DashboardWidget — Dashboard & NOC Center (第60章)
// ============================================================================
class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);
    ~DashboardWidget() override;

private slots:
    void onRefresh();
    void onExport();
    void onQuickAction();

private:
    void setupUI();
    void setupConnections();
    void paintHealthPie();
    void paintAlertTrend();
    void paintTrafficArea();
    void generateMockData();
    void updateDeviceStatus();

    // ── 全局状态卡片 ──
    QLabel* m_onlineDevicesLabel;
    QLabel* m_totalDevicesLabel;
    QLabel* m_alertCountLabel;
    QLabel* m_cpuAvgLabel;
    QLabel* m_memAvgLabel;
    QLabel* m_bandwidthAvgLabel;

    // ── 图表区 ──
    HealthPieChart* m_healthPieChart;
    AlertTrendChart* m_alertTrendChart;
    TrafficAreaChart* m_trafficAreaChart;

    // ── 实时告警滚动 ──
    QListWidget* m_alertListWidget;

    // ── 设备状态表格 ──
    QTableWidget* m_deviceTable;

    // ── Top 告警设备表格 ──
    QTableWidget* m_topAlertTable;

    // ── 快捷操作按钮 ──
    QPushButton* m_pingBtn;
    QPushButton* m_inspectionBtn;
    QPushButton* m_backupBtn;
    QPushButton* m_scanBtn;
    QPushButton* m_exportBtn;

    // ── 定时器 ──
    QTimer* m_refreshTimer;

    // ── 数据 ──
    QList<DashboardData> m_data;
    QList<int> m_alertTrendData;
    QList<double> m_trafficInData;
    QList<double> m_trafficOutData;
    QList<QStringList> m_recentAlerts; // 每项: time, level, device, message
};