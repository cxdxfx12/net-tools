#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QList>

struct WirelessAP
{
    QString name;
    QString ip;
    QString mac;
    QString model;
    int channel24G;
    int channel5G;
    QString status;
    int clientCount;
};

struct WirelessSSID
{
    QString name;
    QString securityType;
    QString band;
    int vlan;
    QString status;
};

struct WirelessClient
{
    QString mac;
    QString ip;
    QString apName;
    QString ssid;
    int signalStrength;
    int connectedDuration;
};

class WirelessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WirelessWidget(QWidget* parent = nullptr);
    ~WirelessWidget() override;

private slots:
    void onRefresh();
    void onExport();
    void onACChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void paintChannelChart();
    void generateMockAPData();
    void generateMockClientData();
    void generateMockSSIDData();

    // UI elements
    QComboBox* m_acCombo;
    QLabel* m_acNameLabel;
    QLabel* m_acIpLabel;
    QLabel* m_onlineAPLabel;
    QLabel* m_clientCountLabel;
    QLabel* m_uptimeLabel;
    QTableWidget* m_apTable;
    QTableWidget* m_ssidTable;
    QTableWidget* m_clientTable;
    QWidget* m_channelChart;
    QPushButton* m_exportBtn;

    QTimer* m_refreshTimer;

    QList<WirelessAP> m_apList;
    QList<WirelessClient> m_clientList;
    QList<WirelessSSID> m_ssidList;
};