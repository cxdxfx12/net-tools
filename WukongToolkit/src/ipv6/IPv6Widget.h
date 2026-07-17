#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QProcess>
#include <QTimer>

class IPv6Widget : public QWidget
{
    Q_OBJECT

public:
    explicit IPv6Widget(QWidget* parent = nullptr);
    ~IPv6Widget() override = default;

private slots:
    void onPing6();
    void onTrace6();
    void onCalculate();
    void onHealthCheck();
    void onExport();
    void onNDPRefresh();
    void onRouteRefresh();

private:
    void setupUI();
    void setupConnections();
    void parseNDPOutput(const QString& output);
    void parseRouteOutput(const QString& output);

    QLineEdit* m_ipv6Input;
    QSpinBox* m_prefixSpin;
    QPushButton* m_pingBtn;
    QPushButton* m_traceBtn;
    QTableWidget* m_ndpTable;
    QTableWidget* m_routeTable;
    QLineEdit* m_networkAddr;
    QLineEdit* m_broadcastAddr;
    QLineEdit* m_rangeAddr;
    QLineEdit* m_hostCount;
    QLabel* m_ipv4Status;
    QLabel* m_ipv6Status;
    QLabel* m_dnsStatus;
    QPushButton* m_healthBtn;
    QPlainTextEdit* m_healthResult;
    QPushButton* m_exportBtn;
    QProcess* m_process;
    QTimer* m_refreshTimer;
};