#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QProcess>
#include <QVector>
#include <QDateTime>

class PingChartWidget;

class PingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PingWidget(QWidget* parent = nullptr);
    ~PingWidget() override;

private:
    void setupUI();
    void setupConnections();

    void startPing();
    void stopPing();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void parseLine(const QString& line);
    void parseReplyLine(const QString& line);
    void parseStatisticsLine(const QString& block);
    void addResultRow(const QString& host, int seq, int ttl, double timeMs, const QString& status);
    void updateStatistics();
    void saveToHistory(const QString& host, double avg, double loss, double max, double min);
    void exportToCSV();

    void nextHost();
    void startPingForHost(const QString& host);

    // UI elements
    QLineEdit* m_hostEdit;
    QSpinBox* m_countSpin;
    QSpinBox* m_intervalSpin;
    QSpinBox* m_sizeSpin;
    QSpinBox* m_timeoutSpin;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_exportBtn;
    QTableWidget* m_resultTable;
    PingChartWidget* m_chartWidget;

    // Statistics labels
    QLabel* m_sentLabel;
    QLabel* m_receivedLabel;
    QLabel* m_lostLabel;
    QLabel* m_minLabel;
    QLabel* m_avgLabel;
    QLabel* m_maxLabel;
    QLabel* m_stddevLabel;

    // Ping state
    QProcess* m_process;
    QStringList m_hosts;
    int m_currentHostIndex;
    int m_sent;
    int m_received;
    double m_minTime;
    double m_maxTime;
    double m_totalTime;
    QVector<double> m_times;
    QString m_currentHost;
    bool m_running;
    bool m_stopping;
    int m_seq;
    QString m_pendingStatisticsHost;
    int m_pendingStatisticsSent;
    int m_pendingStatisticsReceived;
    QString m_statisticsBuffer;
    bool m_collectingStats;

    // Result data for CSV export
    struct PingResult {
        QString host;
        int seq;
        int ttl;
        double timeMs;
        QString status;
    };
    QVector<PingResult> m_results;
};

// ─── PingChartWidget: simple latency line chart ────────────────────────

class PingChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PingChartWidget(QWidget* parent = nullptr);

    void addPoint(double value);
    void clear();
    void setMaxPoints(int max);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_points;
    int m_maxPoints;
};