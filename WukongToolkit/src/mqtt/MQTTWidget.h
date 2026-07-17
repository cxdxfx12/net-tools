#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QComboBox>
#include <QLabel>
#include <QProcess>

// ============================================================================
// MQTTWidget — MQTT 协议调试和测试工具
// ============================================================================
class MQTTWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MQTTWidget(QWidget* parent = nullptr);
    ~MQTTWidget() override;

private slots:
    void onConnect();
    void onDisconnect();
    void onPublish();
    void onSubscribe();
    void onUnsubscribe();
    void onClearOutput();
    void onClearTopics();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupConnections();
    void appendOutput(const QString& text, const QString& color = "#E6EDF3");
    void executeMosquittoCmd(const QStringList& args, const QString& desc);
    void addTopicToHistory(const QString& topic);

    // --- 连接信息 ---
    QLineEdit*    m_hostEdit;
    QSpinBox*     m_portSpin;
    QLineEdit*    m_clientIdEdit;
    QLineEdit*    m_userEdit;
    QLineEdit*    m_passEdit;
    QPushButton*  m_connectBtn;
    QPushButton*  m_disconnectBtn;

    // --- 发布 ---
    QLineEdit*    m_pubTopicEdit;
    QLineEdit*    m_pubPayloadEdit;
    QComboBox*    m_pubQosCombo;
    QPushButton*  m_pubRetainCheck;
    QPushButton*  m_publishBtn;

    // --- 订阅 ---
    QLineEdit*    m_subTopicEdit;
    QComboBox*    m_subQosCombo;
    QPushButton*  m_subscribeBtn;
    QPushButton*  m_unsubscribeBtn;

    // --- 主题列表 ---
    QListWidget*  m_topicsList;
    QPushButton*  m_clearTopicsBtn;

    // --- 输出 ---
    QPlainTextEdit* m_outputText;
    QPushButton*    m_clearOutputBtn;

    // --- 状态 ---
    QLabel*       m_statusLabel;
    QProcess*     m_process;
    bool          m_connected;
};
