#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QList>

struct FlowRec
{
    QString   srcIP;
    QString   dstIP;
    quint16   srcPort = 0;
    quint16   dstPort = 0;
    quint8    protocol = 0;
    quint64   packets = 0;
    quint64   bytes = 0;
    QDateTime startTime;
    QDateTime endTime;
};

struct TopTalkerEntry
{
    QString srcIP;
    quint64 bytes = 0;
    int     flows = 0;
    double  percentage = 0.0;
};

struct TopAppEntry
{
    QString  app;
    QString  protocol;
    quint16  port = 0;
    int      flows = 0;
    quint64  bytes = 0;
};

class FlowCollectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlowCollectorWidget(QWidget* parent = nullptr);
    ~FlowCollectorWidget() override;

private slots:
    void onStartStop();
    void onProcessDatagram();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void parseNetFlowV5(const QByteArray& data, const QString& sourceIP);
    void parseNetFlowV9(const QByteArray& data, const QString& sourceIP);
    void parseIPFIX(const QByteArray& data, const QString& sourceIP);
    void addFlowRec(const FlowRec& rec);
    void updateTopTalker();
    void updateTopApp();
    void updateStatusLabel();

    QString ipToString(quint32 addr) const;
    QString protocolName(quint8 protocol) const;
    QString detectApp(quint16 port, quint8 protocol) const;
    void    loadDemoRecords();

    QSpinBox*     m_portSpin;
    QPushButton*  m_startStopBtn;
    QLabel*       m_statusLabel;
    QTableWidget* m_flowTable;
    QTableWidget* m_topTalkerTable;
    QTableWidget* m_topAppTable;
    QPushButton*  m_exportBtn;
    QUdpSocket*   m_udpSocket;
    QTimer*       m_statsTimer;
    bool          m_running;
    QList<FlowRec> m_records;
    int           m_packetCount;
    int           m_flowCount;

    static constexpr int MAX_RECORDS = 50000;
};