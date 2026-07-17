#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QCheckBox>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include <QDateTime>

// ============================================================================
// Syslog 消息结构
// ============================================================================
struct SyslogMessage
{
    QDateTime time;
    QString   sourceIP;
    int       facility = 0;
    int       severity = 0;
    QString   hostname;
    QString   message;
    QString   rawData;
};

// ============================================================================
// SyslogWidget — Syslog 服务器 (RFC 3164)
// ============================================================================
class SyslogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SyslogWidget(QWidget* parent = nullptr);
    ~SyslogWidget() override;

    /// 根据 Facility 码返回名称
    static QString facilityName(int facility);
    /// 根据 Severity 码返回名称
    static QString severityName(int severity);

private slots:
    void onStartStop();
    void onClear();
    void onExportCSV();
    void onFilterChanged();

    // UDP
    void onUdpReadyRead();
    // TCP
    void onTcpNewConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();

private:
    void setupUI();
    void setupConnections();
    void parseSyslog(const QString& raw, const QString& sourceIP, SyslogMessage& outMsg);
    void addMessageRow(const SyslogMessage& msg);
    void applyFilters();
    void cleanupOldMessages();
    void updateStatus(bool running);
    QColor severityColor(int severity) const;

    static constexpr int MAX_MESSAGES = 10000;

    // --- UI 组件 ---
    QSpinBox*     m_portSpin;
    QComboBox*    m_protocolCombo;
    QPushButton*  m_startStopBtn;
    QLabel*       m_statusLabel;
    QLabel*       m_countLabel;
    QComboBox*    m_facilityFilter;
    QComboBox*    m_severityFilter;
    QLineEdit*    m_searchEdit;
    QCheckBox*    m_autoScrollCheck;
    QPushButton*  m_clearBtn;
    QPushButton*  m_exportBtn;
    QTableWidget* m_messageTable;

    // --- 网络 ---
    QUdpSocket*   m_udpSocket;
    QTcpServer*   m_tcpServer;
    QList<QTcpSocket*> m_tcpClients;   // 活跃 TCP 客户端
    QMap<QTcpSocket*, QByteArray> m_tcpBuffers; // TCP 接收缓冲区

    // --- 状态 ---
    bool          m_running;
    QList<SyslogMessage> m_messages;    // 全部消息
    QMutex        m_mutex;
};