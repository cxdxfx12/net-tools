#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QPlainTextEdit>
#include <QList>

// ============================================================================
// QoSPolicy — QoS 策略数据结构
// ============================================================================

struct QoSPolicy
{
    QString name;
    QString interface;
    QString classifier;
    QString action;
    QString direction;
};

// ============================================================================
// QueueInfo — 队列详情数据结构
// ============================================================================

struct QueueInfo
{
    QString name;
    int currentUtilization;    // 百分比
    int peakUtilization;       // 百分比
    double queueTime;          // 毫秒
    int droppedPackets;
};

// ============================================================================
// DscpChartWidget — DSCP 分布图自定义绘制组件
// ============================================================================

class DscpChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DscpChartWidget(QWidget* parent = nullptr);

    void setData(int ef, int af41, int af31, int af21, int be);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_ef;
    int m_af41;
    int m_af31;
    int m_af21;
    int m_be;
};

// ============================================================================
// QueueChartWidget — 队列利用率图自定义绘制组件
// ============================================================================

class QueueChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QueueChartWidget(QWidget* parent = nullptr);

    void setQueues(const QList<QueueInfo>& queues);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<QueueInfo> m_queues;
};

// ============================================================================
// QoSWidget — QoS 分析中心主界面
// ============================================================================

class QoSWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QoSWidget(QWidget* parent = nullptr);
    ~QoSWidget() override;

private slots:
    void onRefresh();
    void onExport();
    void onDeviceChanged(int index);
    void onInterfaceChanged(int index);

private:
    // ── UI Setup ──
    void setupUI();
    void setupConnections();

    // ── Chart Painting ──
    void paintDSCPChart();
    void paintQueueChart();

    // ── Analysis ──
    void analyzeQoS();
    int calculateHealthScore();

    // ── UI Components ──
    QComboBox* m_deviceCombo;
    QComboBox* m_interfaceCombo;
    QLabel*    m_healthScoreLabel;
    DscpChartWidget*  m_dscpChart;
    QueueChartWidget* m_queueChart;
    QLabel*    m_latencyLabel;
    QLabel*    m_jitterLabel;
    QLabel*    m_packetLossLabel;
    QTableWidget*     m_policyTable;
    QTableWidget*     m_queueDetailTable;
    QPlainTextEdit*   m_congestionText;
    QPushButton*      m_exportBtn;
    QPushButton*      m_refreshBtn;

    // ── Timer ──
    QTimer* m_refreshTimer;

    // ── Data ──
    QList<QoSPolicy> m_policies;
    QList<QueueInfo>  m_queues;
};