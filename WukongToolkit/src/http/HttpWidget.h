#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QDateTime>

// ============================================================================
// HttpWidget — HTTP/HTTPS/REST API 调试工具
// ============================================================================
class HttpWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HttpWidget(QWidget* parent = nullptr);
    ~HttpWidget() override;

private slots:
    void onSend();
    void onAddHeader();
    void onRemoveHeader();
    void onMethodChanged(const QString& method);
    void onHistoryItemClicked(QListWidgetItem* item);
    void onClearHistory();
    void onExportResponse();
    void onNetworkReplyFinished(QNetworkReply* reply);

private:
    void setupUI();
    void setupConnections();
    void sendRequest();
    void addToHistory(const QString& url, const QString& method);
    void clearResponse();
    void updateResponseDisplay(QNetworkReply* reply);
    void formatJson(const QByteArray& data, QPlainTextEdit* editor);
    QString statusColor(int code) const;

    // --- UI 组件 ---
    // 请求区域
    QLineEdit*      m_urlEdit;
    QComboBox*      m_methodCombo;
    QPushButton*    m_sendBtn;
    QPushButton*    m_addHeaderBtn;
    QPushButton*    m_removeHeaderBtn;
    QTableWidget*   m_headersTable;

    // 请求体区域
    QPlainTextEdit* m_requestBodyEdit;
    QComboBox*      m_contentTypeCombo;

    // 认证区域
    QLineEdit*      m_authUserEdit;
    QLineEdit*      m_authPassEdit;
    QCheckBox*      m_authEnableCheck;

    // 超时设置
    QSpinBox*       m_timeoutSpin;

    // 响应区域
    QLabel*         m_statusCodeLabel;
    QLabel*         m_responseTimeLabel;
    QLabel*         m_responseSizeLabel;
    QTableWidget*   m_responseHeadersTable;
    QPlainTextEdit* m_responseBodyEdit;
    QPushButton*    m_exportBtn;

    // 请求历史
    QListWidget*    m_historyList;
    QPushButton*    m_clearHistoryBtn;

    // --- 网络 ---
    QNetworkAccessManager* m_networkManager;
    QElapsedTimer          m_timer;
};