#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QUdpSocket>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <QMap>
#include <QList>

struct FlowRecord {
    quint32 srcaddr = 0;
    quint32 dstaddr = 0;
    quint16 srcport = 0;
    quint16 dstport = 0;
    quint8 protocol = 0;
    quint32 dPkts = 0;
    quint32 dOctets = 0;
    quint64 timestamp = 0;
    QString sourceIP;
};

struct TopTalker {
    QString srcIP;
    QString dstIP;
    quint8 protocol = 0;
    quint16 srcPort = 0;
    quint16 dstPort = 0;
    quint64 packets = 0;
    quint64 bytes = 0;
};

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget* parent = nullptr);
    void addDataPoint(qreal value);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<qreal> m_data;
    static constexpr int MAX_POINTS = 120;
};

class FlowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlowWidget(QWidget* parent = nullptr);
    ~FlowWidget() override;

private slots:
    void onStartStop();
    void onReadDatagram();
    void onRefreshTimer();
    void onExportCSV();
    void onClear();

private:
    void setupUI();
    void setupConnections();
    void parseNetFlowV5(const QByteArray& data, const QString& sourceIP);
    QString ipToString(quint32 ip) const;
    QString protocolName(quint8 proto) const;
    void updateTopTalkers();
    void updateSummary();
    void exportTableToCSV(QTableWidget* table, const QString& title);

    QSpinBox* m_portSpin;
    QComboBox* m_protocolCombo;
    QPushButton* m_startStopBtn;
    QLabel* m_statusLabel;
    QLabel* m_totalFlowsLabel;
    QLabel* m_totalPacketsLabel;
    QLabel* m_totalBytesLabel;
    QLabel* m_activeSourcesLabel;
    QTableWidget* m_topTalkersTable;
    ChartWidget* m_chartWidget;
    QTableWidget* m_flowTable;
    QPushButton* m_clearBtn;
    QPushButton* m_exportBtn;

    QUdpSocket* m_socket;
    QTimer* m_refreshTimer;
    QMutex m_mutex;

    QVector<FlowRecord> m_flowRecords;
    QSet<QString> m_activeSources;
    QMap<QString, TopTalker> m_talkerMap;
    bool m_running = false;
};