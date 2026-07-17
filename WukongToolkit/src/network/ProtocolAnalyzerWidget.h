#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLabel>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslCertificate>

// ============================================================================
// ProtocolAnalyzerWidget — 协议分析工具箱 (HTTP/SSL/NTP/WHOIS/Subnet)
// ============================================================================
class ProtocolAnalyzerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProtocolAnalyzerWidget(QWidget* parent = nullptr);
    ~ProtocolAnalyzerWidget() override;

private slots:
    // HTTP 标签页
    void onHttpSend();
    void onHttpReplyFinished(QNetworkReply* reply);
    void onHttpExport();

    // SSL/TLS 标签页
    void onSslCheck();
    void onSslProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSslExport();

    // NTP 标签页
    void onNtpQuery();
    void onNtpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onNtpExport();

    // WHOIS 标签页
    void onWhoisQuery();
    void onWhoisProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onWhoisExport();

    // 子网计算标签页
    void onSubnetCalc();
    void onSubnetExport();

private:
    void setupUI();
    void setupConnections();
    void setupHttpTab(QWidget* tab);
    void setupSslTab(QWidget* tab);
    void setupNtpTab(QWidget* tab);
    void setupWhoisTab(QWidget* tab);
    void setupSubnetTab(QWidget* tab);

    // NTP 解析辅助
    void parseNtpOutput(const QString& output);

    // 子网计算辅助
    quint32 ipToUint32(const QString& ip);
    QString uint32ToIp(quint32 val);
    int cidrToMaskInt(const QString& cidr);
    QString maskIntToDotted(int bits);

    // ==================== 通用 ====================
    QTabWidget* m_tabWidget;

    // ==================== Tab 1: HTTP Headers ====================
    QLineEdit*   m_httpUrlEdit;
    QPushButton* m_httpSendBtn;
    QLineEdit*   m_httpStatusEdit;
    QTableWidget* m_httpHeadersTable;
    QTextEdit*   m_httpRawText;
    QPushButton* m_httpExportBtn;
    QNetworkAccessManager* m_httpNam;

    // ==================== Tab 2: SSL/TLS ====================
    QLineEdit*   m_sslHostEdit;
    QSpinBox*    m_sslPortSpin;
    QPushButton* m_sslCheckBtn;
    QLabel*      m_sslSubjectLabel;
    QLabel*      m_sslIssuerLabel;
    QLabel*      m_sslValidFromLabel;
    QLabel*      m_sslValidToLabel;
    QLabel*      m_sslSANLabel;
    QLabel*      m_sslSerialLabel;
    QLabel*      m_sslFingerprintLabel;
    QPushButton* m_sslExportBtn;
    QProcess*    m_sslProcess;

    // ==================== Tab 3: NTP ====================
    QLineEdit*   m_ntpServerEdit;
    QPushButton* m_ntpQueryBtn;
    QLabel*      m_ntpTimeLabel;
    QLabel*      m_ntpStratumLabel;
    QLabel*      m_ntpDelayLabel;
    QLabel*      m_ntpOffsetLabel;
    QPushButton* m_ntpExportBtn;
    QProcess*    m_ntpProcess;

    // ==================== Tab 4: WHOIS ====================
    QLineEdit*     m_whoisInputEdit;
    QPushButton*   m_whoisQueryBtn;
    QPlainTextEdit* m_whoisResultText;
    QPushButton*   m_whoisExportBtn;
    QProcess*      m_whoisProcess;

    // ==================== Tab 5: Subnet Calc ====================
    QLineEdit*   m_subnetIpEdit;
    QLineEdit*   m_subnetMaskEdit;
    QPushButton* m_subnetCalcBtn;
    QLabel*      m_subnetNetworkLabel;
    QLabel*      m_subnetBroadcastLabel;
    QLabel*      m_subnetFirstHostLabel;
    QLabel*      m_subnetLastHostLabel;
    QLabel*      m_subnetTotalHostsLabel;
    QLabel*      m_subnetWildcardLabel;
    QLabel*      m_subnetBinaryLabel;
    QPushButton* m_subnetExportBtn;
};