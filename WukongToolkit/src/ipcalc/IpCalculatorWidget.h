#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>

class IpCalculatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IpCalculatorWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void setupConnections();

    void onCalculate();
    void onBatchCalculate();
    void onExport();
    void addHistoryEntry(const QString& ip, int prefix);

    struct IPv4Info {
        QString ip;
        QString network;
        QString broadcast;
        QString subnetMask;
        QString wildcard;
        int prefix;
        quint32 hostCount;
        QString firstHost;
        QString lastHost;
        QString binary;
    };

    // ── Input widgets ──
    QLineEdit* m_ipEdit;
    QSpinBox* m_prefixSpin;
    QPushButton* m_calcBtn;
    QPlainTextEdit* m_batchEdit;
    QPushButton* m_batchBtn;

    // ── Result widgets ──
    QLineEdit* m_networkEdit;
    QLineEdit* m_broadcastEdit;
    QLineEdit* m_rangeEdit;
    QLineEdit* m_hostCountEdit;
    QLineEdit* m_maskEdit;
    QLineEdit* m_wildcardEdit;
    QPlainTextEdit* m_binaryEdit;

    // ── History ──
    QTableWidget* m_historyTable;
    QPushButton* m_exportBtn;

    // ── Data ──
    QVector<IPv4Info> m_history;
};