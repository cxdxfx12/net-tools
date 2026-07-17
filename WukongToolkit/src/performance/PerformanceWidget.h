#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QStringList>

// ═══════════════════════════════════════════════════════════════════════════════
// PerfDataPoint — 性能数据点
// ═══════════════════════════════════════════════════════════════════════════════

struct PerfDataPoint
{
    QDateTime timestamp;
    double cpu;
    double mem;
    double temp;
    double bandwidth;
};

// ═══════════════════════════════════════════════════════════════════════════════
// TrendChartWidget — 趋势折线图子控件
// ═══════════════════════════════════════════════════════════════════════════════

class TrendChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum ChartType { CPU, Memory, Bandwidth, Temperature };

    explicit TrendChartWidget(ChartType type, QWidget* parent = nullptr);

    void setData(const QList<PerfDataPoint>& data);
    void setTitle(const QString& title);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ChartType m_chartType;
    QString m_title;
    QList<PerfDataPoint> m_data;

    QColor lineColor() const;
    QColor fillColor() const;
    QString valueLabel(double value) const;
    double maxValue() const;
    double minValue() const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// PerformanceWidget — 性能管理中心
// ═══════════════════════════════════════════════════════════════════════════════

class PerformanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PerformanceWidget(QWidget* parent = nullptr);
    ~PerformanceWidget() override;

private slots:
    void onRefresh();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void paintCPUTrend();
    void paintMemoryTrend();
    void paintBandwidthTrend();
    void paintTemperatureTrend();
    void generateMockData();
    void updateTopN();

    // UI — 设备选择 & 时间范围
    QComboBox* m_deviceCombo;
    QComboBox* m_timeRangeCombo;

    // UI — 四个趋势图
    TrendChartWidget* m_cpuTrendWidget;
    TrendChartWidget* m_memoryTrendWidget;
    TrendChartWidget* m_bandwidthTrendWidget;
    TrendChartWidget* m_temperatureTrendWidget;

    // UI — 表格
    QTableWidget* m_topNTable;
    QTableWidget* m_historyTable;

    // UI — 按钮
    QPushButton* m_exportBtn;

    // 数据
    QList<PerfDataPoint> m_data;
    QTimer* m_refreshTimer;
};