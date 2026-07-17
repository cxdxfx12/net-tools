#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QProcess>
#include <QElapsedTimer>
#include <QStringList>
#include <QDateTime>

// ============================================================================
// DNSWidget — DNS 查询工具
// ============================================================================
class DNSWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DNSWidget(QWidget* parent = nullptr);
    ~DNSWidget() override;

private slots:
    void onQuery();
    void onReverseQuery();
    void onBulkQuery();
    void onHistoryItemClicked(QListWidgetItem* item);
    void onExportResults();
    void onClearHistory();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupConnections();
    void runDig(const QStringList& args);
    void parseDigOutput(const QString& output);
    void addResultRow(const QString& name, int ttl, const QString& type, const QString& value);
    void clearResults();
    void addToHistory(const QString& queryStr, const QString& server, const QString& recordType);
    void updateInfoLabels(const QString& server, qint64 elapsedMs);
    void exportToCSV();
    QStringList buildDigArgs(const QString& domain, const QString& server, const QString& recordType);

    // --- UI 组件 ---
    // 正向查询
    QLineEdit*   m_domainEdit;
    QLineEdit*   m_serverEdit;
    QComboBox*   m_recordTypeCombo;
    QPushButton* m_queryBtn;

    // 反向查询
    QLineEdit*   m_reverseEdit;
    QPushButton* m_reverseBtn;

    // 批量查询
    QTextEdit*   m_bulkEdit;
    QPushButton* m_bulkBtn;

    // 结果表
    QTableWidget* m_resultTable;

    // 信息标签
    QLabel* m_serverInfoLabel;
    QLabel* m_queryTimeLabel;

    // 查询历史
    QListWidget* m_historyList;
    QPushButton* m_clearHistoryBtn;
    QPushButton* m_exportBtn;

    // --- 查询状态 ---
    QProcess*    m_process;
    QElapsedTimer m_timer;
    QString      m_currentServer;
    QString      m_currentQueryDomain;
    QString      m_currentRecordType;
    bool         m_isBulkQuery;
    QStringList  m_bulkDomains;
    int          m_bulkIndex;
    bool         m_isReverseQuery;
};