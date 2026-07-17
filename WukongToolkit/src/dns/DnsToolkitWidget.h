#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QElapsedTimer>
#include <QDateTime>

// ============================================================================
// DnsToolkitWidget — 高级 DNS 诊断工具
// ============================================================================
class DnsToolkitWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DnsToolkitWidget(QWidget* parent = nullptr);
    ~DnsToolkitWidget() override;

private slots:
    void onQuery();
    void onBatchQuery();
    void onCompareDns();
    void onExport();
    void onServerChanged(int index);

private:
    void setupUI();
    void setupConnections();
    QString resolveDNS(const QString& domain, const QString& type, const QString& server);
    void addHistoryEntry(const QString& domain, const QString& type, const QString& status);

    void runDigQuery(const QString& domain, const QString& type, const QString& server);
    void parseDigResult(const QString& domain, const QString& type, const QString& server,
                        const QString& output, qint64 elapsedMs);
    void addResultRow(const QString& domain, const QString& type, const QString& result,
                      int ttl, const QString& server, double responseTimeMs);
    void setQueryButtonsEnabled(bool enabled);

    // --- UI 组件 ---
    // 查询输入区
    QLineEdit*    m_domainEdit;
    QComboBox*    m_recordTypeCombo;
    QComboBox*    m_dnsServerCombo;
    QLineEdit*    m_customServerEdit;
    QPushButton*  m_queryBtn;

    // 批量查询
    QPlainTextEdit* m_batchEdit;
    QPushButton*    m_batchBtn;

    // 多 DNS 对比
    QPushButton*  m_compareBtn;

    // 结果区
    QTableWidget*   m_resultTable;
    QPlainTextEdit* m_detailEdit;

    // 历史区
    QTableWidget* m_historyTable;
    QPushButton*  m_exportBtn;

    // --- 查询状态 ---
    QProcess*      m_process;
    QElapsedTimer  m_timer;
    QString        m_currentDomain;
    QString        m_currentType;
    QString        m_currentServer;
    bool           m_isBatchQuery;
    QStringList    m_batchDomains;
    int            m_batchIndex;
    bool           m_isCompareQuery;
    QStringList    m_compareServers;
    int            m_compareIndex;
    QString        m_compareDomain;
    QString        m_compareType;
};