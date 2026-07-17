#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QTimer>
#include <QProcess>
#include <QTabWidget>
#include <QMap>
#include <QPair>

class SNMPMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SNMPMonitorWidget(QWidget* parent = nullptr);
    ~SNMPMonitorWidget() override;

private slots:
    void onStartMonitor();
    void onStopMonitor();
    void onPoll();
    void onExport();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onMonitorTypeChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void querySystemInfo(const QString& ip, const QString& community);
    void queryInterfaces(const QString& ip, const QString& community);
    void queryCPU(const QString& ip, const QString& community);
    void queryMemory(const QString& ip, const QString& community);
    void queryTemperature(const QString& ip, const QString& community);
    void queryCustomOid(const QString& ip, const QString& community, const QString& oid);
    void addMetricRow(const QString& metric, const QString& value, const QString& status);
    void addMetricRowTo(QTableWidget* table, const QString& metric, const QString& value, const QString& status);
    void addInterfaceRow(const QString& name, const QString& status,
                         const QString& inTraffic, const QString& outTraffic,
                         const QString& inPkts, const QString& outPkts,
                         const QString& errors);
    QString buildCommunityArgs(const QString& ip, const QString& community) const;
    QString formatTraffic(qint64 bytes) const;
    void clearMetricsTable(QTableWidget* table);
    void parseSnmpLine(const QString& line, QMap<QString, QString>& resultMap);
    int extractLastOidComponent(const QString& oidStr) const;

    // UI elements
    QLineEdit* m_ipEdit;
    QComboBox* m_versionCombo;
    QLineEdit* m_communityEdit;
    QComboBox* m_monitorTypeCombo;
    QLineEdit* m_customOidEdit;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QSpinBox* m_intervalSpin;
    QCheckBox* m_autoRefreshCheck;
    QPushButton* m_exportBtn;
    QTabWidget* m_tabWidget;
    QTableWidget* m_systemTable;
    QTableWidget* m_interfaceTable;
    QTableWidget* m_cpuMemTable;

    // Timer
    QTimer* m_pollTimer;
    bool m_isMonitoring;

    // Process
    QProcess* m_process;
    int m_currentQueryType; // 0=None, 1=SystemInfo, 2=Interfaces, 3=CPU, 4=Memory, 5=Temperature, 6=CustomOID

    // Previous interface counter values for rate calculation (ifIndex -> {inOctets, outOctets, inPkts, outPkts, errors})
    QMap<int, QVector<qint64>> m_prevInterfaceCounters;
};