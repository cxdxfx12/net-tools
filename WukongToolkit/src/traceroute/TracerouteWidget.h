#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QProcess>
#include <QDateTime>
#include <QVector>

struct TracerouteHop
{
    int hop = 0;
    QString ip;
    QString hostname;
    double rtt1 = -1.0;
    double rtt2 = -1.0;
    double rtt3 = -1.0;
    double avgRtt = -1.0;
    double lossPercent = 0.0;
    QString asNumber;
    QString location;
};

class TracerouteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TracerouteWidget(QWidget* parent = nullptr);
    ~TracerouteWidget() override;

private slots:
    void onStart();
    void onStop();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExportCSV();
    void onClear();
    void onTableItemSelected(int row, int column);

private:
    void setupUI();
    void setupConnections();
    void parseLine(const QString& line);
    void addHopToTable(const TracerouteHop& hop);
    void updateStatistics();
    void saveToHistory();
    void loadHistory();
    void ensureHopDetailTable();

    // UI elements
    QLineEdit* m_targetEdit;
    QSpinBox* m_maxHopsSpin;
    QSpinBox* m_timeoutSpin;
    QSpinBox* m_queriesSpin;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_clearBtn;
    QTableWidget* m_resultsTable;
    QLabel* m_hopDetailLabel;
    QLabel* m_statTotalHops;
    QLabel* m_statAvgRtt;
    QLabel* m_statPacketLoss;

    // Process
    QProcess* m_process;
    QVector<TracerouteHop> m_hops;
    QDateTime m_startTime;
    bool m_running = false;
    int m_currentHop = 0;
    QString m_targetHost;
    QString m_targetIp;
};