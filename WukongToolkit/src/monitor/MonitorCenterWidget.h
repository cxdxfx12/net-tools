#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <QStringList>
#include <QList>

struct MonitorDeviceInfo
{
    QString name;
    QString ip;
    QString community;
};

class MonitorCenterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorCenterWidget(QWidget* parent = nullptr);
    ~MonitorCenterWidget() override;

private slots:
    void onAddDevice();
    void onRefresh();
    void onToggleAutoRefresh(bool enabled);
    void onExport();
    void onFullScreen();

private:
    void setupUI();
    void setupConnections();
    void updateDeviceList();
    void queryDeviceMetrics(const QString& ip);
    void addMetricRow(const QString& metric, const QString& value,
                      const QString& status, const QString& threshold);
    void addInterfaceRow(const QString& name, const QString& status,
                         const QString& inTraffic, const QString& outTraffic,
                         const QString& errors, const QString& drops);
    void clearMetrics();
    void clearInterfaces();
    void updateProgressBar(QProgressBar* bar, int value);
    QString formatTraffic(qint64 bytes) const;
    QString formatUptime(qint64 ticks) const;
    void saveDevices();
    void loadDevices();
    bool checkSnmpAvailable() const;
    void generateSimulatedMetrics();
    void generateSimulatedInterfaces();
    void queryInterfacesViaSnmp(const QString& ip, const QString& community);

    // UI elements
    QComboBox* m_deviceCombo;
    QPushButton* m_addDeviceBtn;
    QCheckBox* m_autoRefreshCheck;
    QSpinBox* m_intervalSpin;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_fullScreenBtn;

    // Dashboard cards
    QProgressBar* m_cpuBar;
    QProgressBar* m_memBar;
    QLabel* m_tempLabel;
    QLabel* m_tempValueLabel;
    QLabel* m_uptimeLabel;
    QLabel* m_uptimeValueLabel;

    // Tables
    QTableWidget* m_metricsTable;
    QTableWidget* m_interfaceTable;

    // Timer
    QTimer* m_autoRefreshTimer;

    // Process
    QProcess* m_process;
    int m_currentQueryType; // 0=None, 1=Metrics, 2=Interfaces

    // Device list
    QList<MonitorDeviceInfo> m_devices;

    // Full screen state
    bool m_isFullScreen;
};