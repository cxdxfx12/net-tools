#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <QDateTime>

// ============================================================================
// SyslogCenterWidget — 企业级 Syslog 日志中心 (Chapter 40)
// ============================================================================
class SyslogCenterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SyslogCenterWidget(QWidget* parent = nullptr);
    ~SyslogCenterWidget() override;

    static QString facilityName(int code);
    static QString severityName(int severity);

private slots:
    void onStartServer();
    void onStopServer();
    void onApplyFilter();
    void onClearFilter();
    void onExport();
    void onClearLogs();
    void onLogSelected();

    // UDP
    void onUdpReadyRead();
    // TCP
    void onTcpNewConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();

private:
    void setupUI();
    void setupConnections();
    void processLogMessage(const QString& rawMessage, const QString& sourceHost);
    void parseRFC3164(const QString& msg, QString& timestamp, QString& host,
                      QString& facility, QString& severity, QString& message);
    void parseRFC5424(const QString& msg, QString& timestamp, QString& host,
                      QString& facility, QString& severity, QString& message);
    void addLogEntry(const QString& time, const QString& host, const QString& severity,
                     const QString& facility, const QString& message);
    void updateStatistics();
    QString severityToColor(const QString& severity) const;
    void updateStatus(bool running);
    bool matchesFilter(const QString& severity, const QString& facility,
                       const QString& host, const QString& message) const;
    void addDemoLogs();

    // ========== 服务器控制 ==========
    QSpinBox*     m_portSpin;
    QComboBox*    m_protocolCombo;
    QPushButton*  m_startBtn;
    QPushButton*  m_stopBtn;
    QLabel*       m_statusLabel;

    // ========== 过滤区 ==========
    QComboBox*    m_severityFilter;
    QComboBox*    m_facilityFilter;
    QComboBox*    m_deviceFilter;
    QLineEdit*    m_searchEdit;
    QPushButton*  m_applyFilterBtn;
    QPushButton*  m_clearFilterBtn;

    // ========== 日志表格 ==========
    QTableWidget* m_logTable;

    // ========== 日志详情 ==========
    QPlainTextEdit* m_logDetail;

    // ========== 统计区 ==========
    QLabel*       m_totalCountLabel;
    QLabel*       m_todayCountLabel;
    QLabel*       m_errorCountLabel;

    // ========== 底部控制 ==========
    QCheckBox*    m_autoScrollCheck;
    QPushButton*  m_exportBtn;
    QPushButton*  m_clearBtn;

    // ========== 网络 ==========
    QUdpSocket*   m_udpSocket;
    QTcpServer*   m_tcpServer;
    QList<QTcpSocket*> m_tcpClients;
    QMap<QTcpSocket*, QByteArray> m_tcpBuffers;

    // ========== 状态 ==========
    bool          m_running;
    QMutex        m_mutex;

    // ========== 数据存储 ==========
    struct LogEntry {
        QDateTime time;
        QString   host;
        QString   severity;
        QString   facility;
        QString   message;
    };
    QList<LogEntry> m_logEntries;
    QStringList     m_deviceList;

    static constexpr int MAX_ENTRIES = 50000;
};