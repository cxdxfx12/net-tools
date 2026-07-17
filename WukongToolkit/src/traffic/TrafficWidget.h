#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QList>

// ─── Traffic Data Structures ─────────────────────────────────────────────────

struct TrafficData
{
    QString interface;
    qint64 inBytes;
    qint64 outBytes;
    double utilization;
    QString status;
};

struct TopTalkerData
{
    QString srcIP;
    QString dstIP;
    QString protocol;
    qint64 traffic;
    int sessions;
};

struct BroadcastData
{
    QString type;
    qint64 packets;
    qint64 bytes;
    double percentage;
};

// ─── Bandwidth Trend Chart Widget ────────────────────────────────────────────

class BandwidthTrendWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BandwidthTrendWidget(QWidget* parent = nullptr);
    void setData(const QList<TrafficData>& data);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<TrafficData> m_data;
};

// ─── Protocol Pie Chart Widget ───────────────────────────────────────────────

class ProtocolPieWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProtocolPieWidget(QWidget* parent = nullptr);
    void setData(qint64 tcpBytes, qint64 udpBytes, qint64 icmpBytes, qint64 othersBytes);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qint64 m_tcpBytes = 0;
    qint64 m_udpBytes = 0;
    qint64 m_icmpBytes = 0;
    qint64 m_othersBytes = 0;
};

// ─── Traffic Analysis Center Widget ──────────────────────────────────────────

class TrafficWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficWidget(QWidget* parent = nullptr);
    ~TrafficWidget() override;

private slots:
    void onRefresh();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void paintBandwidthChart();
    void paintProtocolPie();
    void generateMockData();
    void updateTopTalker();

    // UI Components
    QComboBox* m_deviceCombo;
    QComboBox* m_timeRangeCombo;
    BandwidthTrendWidget* m_bandwidthChart;
    QTableWidget* m_topInterfaceTable;
    ProtocolPieWidget* m_protocolPie;
    QTableWidget* m_topTalkerTable;
    QTableWidget* m_broadcastTable;
    QPushButton* m_exportBtn;

    // Data
    QTimer* m_refreshTimer;
    QList<TrafficData> m_data;
    QList<TopTalkerData> m_topTalkers;
    QList<BroadcastData> m_broadcasts;
};