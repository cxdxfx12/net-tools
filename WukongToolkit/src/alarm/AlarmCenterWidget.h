#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QList>
#include <QString>

// ============================================================================
// 告警条目结构
// ============================================================================
struct AlarmEntry
{
    QString id;
    QString time;
    QString level;       // Info / Warning / Critical / Emergency
    QString source;      // Ping / SNMP / Syslog / SSH / HTTP
    QString device;
    QString message;
    QString status;      // Unacknowledged / Acknowledged / Recovered
    QString acknowledgedBy;
};

// ============================================================================
// AlarmCenterWidget — 告警中心 (第39章)
// ============================================================================
class AlarmCenterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlarmCenterWidget(QWidget* parent = nullptr);

    /// 添加告警
    void addAlarm(const QString& level, const QString& source,
                  const QString& device, const QString& message);

private slots:
    void onApplyFilter();
    void onClearFilter();
    void onAcknowledge();
    void onRecover();
    void onAcknowledgeAll();
    void onExport();
    void onAlarmSelected();

private:
    void setupUI();
    void setupConnections();
    void updateStatistics();
    void applyFilters();
    QString levelToColor(const QString& level) const;

    // --- 告警数据 ---
    QList<AlarmEntry> m_alarms;

    // --- 统计区 ---
    QLabel* m_totalLabel;
    QLabel* m_unacknowledgedLabel;
    QLabel* m_criticalLabel;
    QLabel* m_emergencyLabel;

    // --- 过滤区 ---
    QComboBox*   m_levelFilter;
    QComboBox*   m_sourceFilter;
    QComboBox*   m_deviceFilter;
    QComboBox*   m_statusFilter;
    QLineEdit*   m_searchEdit;
    QPushButton* m_applyFilterBtn;
    QPushButton* m_clearFilterBtn;

    // --- 告警表格 ---
    QTableWidget* m_alarmTable;

    // --- 操作按钮 ---
    QPushButton*   m_ackBtn;
    QPushButton*   m_recoverBtn;
    QPushButton*   m_ackAllBtn;
    QPushButton*   m_exportBtn;

    // --- 详情面板 ---
    QPlainTextEdit* m_detailView;
};