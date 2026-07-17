#include "mqtt/MQTTWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// MQTTWidget 实现
// ============================================================================

MQTTWidget::MQTTWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_connected(false)
{
    setupUI();
    setupConnections();
}

MQTTWidget::~MQTTWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void MQTTWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 连接区 ──
    auto* connGroup = new QGroupBox("MQTT 连接");
    auto* connLayout = new QHBoxLayout(connGroup);
    connLayout->setSpacing(8);

    auto* hostLabel = new QLabel("Broker:");
    
    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("test.mosquitto.org");
    m_hostEdit->setMinimumWidth(160);

    auto* portLabel = new QLabel("端口:");
    
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(1883);
    m_portSpin->setFixedWidth(80);

    auto* clientLabel = new QLabel("Client ID:");
    
    m_clientIdEdit = new QLineEdit();
    m_clientIdEdit->setPlaceholderText("wukong-mqtt-client");
    m_clientIdEdit->setMinimumWidth(140);

    auto* userLabel = new QLabel("用户:");
    
    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("(可选)");
    m_userEdit->setMinimumWidth(80);

    auto* passLabel = new QLabel("密码:");
    
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("(可选)");
    m_passEdit->setMinimumWidth(80);

    m_connectBtn = new QPushButton("连接");
    m_connectBtn->setFixedHeight(32);

    m_disconnectBtn = new QPushButton("断开");
    m_disconnectBtn->setFixedHeight(32);
    m_disconnectBtn->setEnabled(false);

    connLayout->addWidget(hostLabel);
    connLayout->addWidget(m_hostEdit, 2);
    connLayout->addWidget(portLabel);
    connLayout->addWidget(m_portSpin);
    connLayout->addWidget(clientLabel);
    connLayout->addWidget(m_clientIdEdit, 2);
    connLayout->addWidget(userLabel);
    connLayout->addWidget(m_userEdit, 1);
    connLayout->addWidget(passLabel);
    connLayout->addWidget(m_passEdit, 1);
    connLayout->addWidget(m_connectBtn);
    connLayout->addWidget(m_disconnectBtn);

    mainLayout->addWidget(connGroup);

    // ── 状态标签 ──
    m_statusLabel = new QLabel("未连接");
    
    mainLayout->addWidget(m_statusLabel);

    // ── 发布区 ──
    auto* pubGroup = new QGroupBox("发布消息 (Publish)");
    auto* pubLayout = new QVBoxLayout(pubGroup);
    pubLayout->setSpacing(6);

    auto* pubTopicRow = new QHBoxLayout();
    pubTopicRow->setSpacing(8);
    auto* pubTopicLabel = new QLabel("主题:");
    
    m_pubTopicEdit = new QLineEdit();
    m_pubTopicEdit->setPlaceholderText("test/topic");
    pubTopicRow->addWidget(pubTopicLabel);
    pubTopicRow->addWidget(m_pubTopicEdit, 1);

    auto* pubQosLabel = new QLabel("QoS:");
    
    m_pubQosCombo = new QComboBox();
    m_pubQosCombo->addItems({"0", "1", "2"});
    pubTopicRow->addWidget(pubQosLabel);
    pubTopicRow->addWidget(m_pubQosCombo);

    m_pubRetainCheck = new QPushButton("Retain");
    m_pubRetainCheck->setCheckable(true);
    m_pubRetainCheck->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 5px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:checked {"
        "  background-color: #58A6FF; color: white;"
        "  border-color: #58A6FF;"
        "}"
    );
    pubTopicRow->addWidget(m_pubRetainCheck);

    pubLayout->addLayout(pubTopicRow);

    auto* pubPayloadRow = new QHBoxLayout();
    pubPayloadRow->setSpacing(8);
    auto* pubPayloadLabel = new QLabel("消息:");
    
    m_pubPayloadEdit = new QLineEdit();
    m_pubPayloadEdit->setPlaceholderText("输入要发布的消息内容...");
    pubPayloadRow->addWidget(pubPayloadLabel);
    pubPayloadRow->addWidget(m_pubPayloadEdit, 1);

    m_publishBtn = new QPushButton("发布");
    m_publishBtn->setEnabled(false);
    pubPayloadRow->addWidget(m_publishBtn);

    pubLayout->addLayout(pubPayloadRow);
    mainLayout->addWidget(pubGroup);

    // ── 订阅区 ──
    auto* subGroup = new QGroupBox("订阅主题 (Subscribe)");
    auto* subLayout = new QVBoxLayout(subGroup);
    subLayout->setSpacing(6);

    auto* subTopicRow = new QHBoxLayout();
    subTopicRow->setSpacing(8);
    auto* subTopicLabel = new QLabel("主题:");
    
    m_subTopicEdit = new QLineEdit();
    m_subTopicEdit->setPlaceholderText("test/# (支持通配符 + 和 #)");
    subTopicRow->addWidget(subTopicLabel);
    subTopicRow->addWidget(m_subTopicEdit, 1);

    auto* subQosLabel = new QLabel("QoS:");
    
    m_subQosCombo = new QComboBox();
    m_subQosCombo->addItems({"0", "1", "2"});
    subTopicRow->addWidget(subQosLabel);
    subTopicRow->addWidget(m_subQosCombo);

    m_subscribeBtn = new QPushButton("订阅");
    m_subscribeBtn->setEnabled(false);
    subTopicRow->addWidget(m_subscribeBtn);

    m_unsubscribeBtn = new QPushButton("取消订阅");
    m_unsubscribeBtn->setEnabled(false);
    subTopicRow->addWidget(m_unsubscribeBtn);

    subLayout->addLayout(subTopicRow);
    mainLayout->addWidget(subGroup);

    // ── 主题列表 + 输出 ──
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(8);

    // 主题列表
    auto* topicsGroup = new QGroupBox("已订阅主题");
    auto* topicsLayout = new QVBoxLayout(topicsGroup);
    topicsLayout->setSpacing(4);

    m_topicsList = new QListWidget();
    topicsLayout->addWidget(m_topicsList, 1);

    m_clearTopicsBtn = new QPushButton("清空");
    m_clearTopicsBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    topicsLayout->addWidget(m_clearTopicsBtn);

    // 输出
    auto* outputGroup = new QGroupBox("消息日志");
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(4);

    m_outputText = new QPlainTextEdit();
    m_outputText->setReadOnly(true);
    m_outputText->setMaximumBlockCount(5000);
    outputLayout->addWidget(m_outputText, 1);

    m_clearOutputBtn = new QPushButton("清空日志");
    m_clearOutputBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    outputLayout->addWidget(m_clearOutputBtn);

    bottomLayout->addWidget(topicsGroup, 1);
    bottomLayout->addWidget(outputGroup, 3);
    mainLayout->addLayout(bottomLayout, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void MQTTWidget::setupConnections()
{
    connect(m_connectBtn, &QPushButton::clicked, this, &MQTTWidget::onConnect);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MQTTWidget::onDisconnect);
    connect(m_publishBtn, &QPushButton::clicked, this, &MQTTWidget::onPublish);
    connect(m_subscribeBtn, &QPushButton::clicked, this, &MQTTWidget::onSubscribe);
    connect(m_unsubscribeBtn, &QPushButton::clicked, this, &MQTTWidget::onUnsubscribe);
    connect(m_clearOutputBtn, &QPushButton::clicked, this, &MQTTWidget::onClearOutput);
    connect(m_clearTopicsBtn, &QPushButton::clicked, this, &MQTTWidget::onClearTopics);
    connect(m_pubPayloadEdit, &QLineEdit::returnPressed, this, &MQTTWidget::onPublish);
    connect(m_subTopicEdit, &QLineEdit::returnPressed, this, &MQTTWidget::onSubscribe);
}

// ─── Output Helper ─────────────────────────────────────────────────────

void MQTTWidget::appendOutput(const QString& text, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m_outputText->appendHtml(
        QString("<span style='color:#8C8C8C;'>[%1]</span> "
                "<span style='color:%2;'>%3</span>")
            .arg(timestamp, color, text.toHtmlEscaped()));
    // 自动滚动到底部
    QScrollBar* bar = m_outputText->verticalScrollBar();
    bar->setValue(bar->maximum());
}

// ─── Execute mosquitto command ─────────────────────────────────────────

void MQTTWidget::executeMosquittoCmd(const QStringList& args, const QString& desc)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MQTTWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MQTTWidget::onProcessFinished);

    m_process->start("mosquitto_pub", args);
    appendOutput(desc, "#58A6FF");
}

// ─── Slot: Connect ─────────────────────────────────────────────────────

void MQTTWidget::onConnect()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 MQTT Broker 地址。");
        return;
    }

    // 测试连接 - 使用 mosquitto_pub 发送一个测试消息
    QStringList args;
    args << "-h" << host << "-p" << QString::number(m_portSpin->value());
    args << "-i" << (m_clientIdEdit->text().trimmed().isEmpty() ? "wukong-test" : m_clientIdEdit->text().trimmed());
    args << "-t" << "wukong/test/connection";
    args << "-m" << "ping";
    args << "-q" << "0";

    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text().trimmed();
    if (!user.isEmpty()) {
        args << "-u" << user;
        if (!pass.isEmpty()) args << "-P" << pass;
    }

    // 先断开之前的连接
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MQTTWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, host](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            m_connected = true;
            m_connectBtn->setEnabled(false);
            m_disconnectBtn->setEnabled(true);
            m_publishBtn->setEnabled(true);
            m_subscribeBtn->setEnabled(true);
            m_unsubscribeBtn->setEnabled(true);
            m_statusLabel->setText("已连接 - " + host + ":" + QString::number(m_portSpin->value()));
            
            appendOutput("连接成功! Broker: " + host, "#3FB950");
            Logger::instance().info("MQTT", "已连接到 " + host);
        } else {
            QString err = QString::fromUtf8(m_process ? m_process->readAllStandardError() : QByteArray());
            appendOutput("连接失败: " + err, "#F85149");
            Logger::instance().error("MQTT", "连接失败: " + err);
        }
    });

    m_process->start("mosquitto_pub", args);
    appendOutput("正在连接到 " + host + ":" + QString::number(m_portSpin->value()) + " ...", "#D29922");
}

// ─── Slot: Disconnect ──────────────────────────────────────────────────

void MQTTWidget::onDisconnect()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
        m_process = nullptr;
    }

    m_connected = false;
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
    m_publishBtn->setEnabled(false);
    m_subscribeBtn->setEnabled(false);
    m_unsubscribeBtn->setEnabled(false);
    m_statusLabel->setText("未连接");
    
    m_topicsList->clear();
    appendOutput("已断开连接", "#F85149");
    Logger::instance().info("MQTT", "已断开连接");
}

// ─── Slot: Publish ─────────────────────────────────────────────────────

void MQTTWidget::onPublish()
{
    if (!m_connected) {
        QMessageBox::warning(this, "未连接", "请先连接到 MQTT Broker。");
        return;
    }

    QString topic = m_pubTopicEdit->text().trimmed();
    if (topic.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入发布主题。");
        return;
    }

    QString payload = m_pubPayloadEdit->text();
    QString host = m_hostEdit->text().trimmed();
    QString qos = m_pubQosCombo->currentText();
    bool retain = m_pubRetainCheck->isChecked();

    QStringList args;
    args << "-h" << host << "-p" << QString::number(m_portSpin->value());
    args << "-i" << (m_clientIdEdit->text().trimmed().isEmpty() ? "wukong-pub" : m_clientIdEdit->text().trimmed() + "-pub");
    args << "-t" << topic;
    args << "-m" << payload;
    args << "-q" << qos;
    if (retain) args << "-r";

    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text().trimmed();
    if (!user.isEmpty()) {
        args << "-u" << user;
        if (!pass.isEmpty()) args << "-P" << pass;
    }

    executeMosquittoCmd(args, QString("发布 → %1 [QoS:%2%3]: %4")
                               .arg(topic, qos, retain ? ",Retain" : "", payload));
    Logger::instance().info("MQTT", "发布消息到 " + topic);
}

// ─── Slot: Subscribe ───────────────────────────────────────────────────

void MQTTWidget::onSubscribe()
{
    if (!m_connected) {
        QMessageBox::warning(this, "未连接", "请先连接到 MQTT Broker。");
        return;
    }

    QString topic = m_subTopicEdit->text().trimmed();
    if (topic.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入订阅主题。");
        return;
    }

    QString host = m_hostEdit->text().trimmed();
    QString qos = m_subQosCombo->currentText();

    QStringList args;
    args << "-h" << host << "-p" << QString::number(m_portSpin->value());
    args << "-i" << (m_clientIdEdit->text().trimmed().isEmpty() ? "wukong-sub" : m_clientIdEdit->text().trimmed() + "-sub");
    args << "-t" << topic;
    args << "-q" << qos;
    args << "-v"; // verbose

    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text().trimmed();
    if (!user.isEmpty()) {
        args << "-u" << user;
        if (!pass.isEmpty()) args << "-P" << pass;
    }

    // 在单独的进程中运行订阅（会阻塞等待消息）
    QProcess* subProc = new QProcess(this);
    connect(subProc, &QProcess::readyReadStandardOutput, this, [this, subProc, topic]() {
        QString msg = QString::fromUtf8(subProc->readAllStandardOutput()).trimmed();
        if (!msg.isEmpty()) {
            appendOutput("收到 [" + topic + "]: " + msg, "#3FB950");
        }
    });
    connect(subProc, &QProcess::readyReadStandardError, this, [this, subProc]() {
        QString err = QString::fromUtf8(subProc->readAllStandardError()).trimmed();
        if (!err.isEmpty()) {
            appendOutput(err, "#F85149");
        }
    });

    subProc->start("mosquitto_sub", args);
    appendOutput("订阅主题: " + topic + " [QoS:" + qos + "]", "#58A6FF");
    addTopicToHistory(topic);
    Logger::instance().info("MQTT", "订阅主题: " + topic);
}

// ─── Slot: Unsubscribe ─────────────────────────────────────────────────

void MQTTWidget::onUnsubscribe()
{
    QString topic = m_subTopicEdit->text().trimmed();
    if (topic.isEmpty()) {
        // 尝试从主题列表获取
        QListWidgetItem* item = m_topicsList->currentItem();
        if (item) {
            topic = item->text();
        } else {
            QMessageBox::warning(this, "输入错误", "请输入或选择要取消订阅的主题。");
            return;
        }
    }

    appendOutput("取消订阅: " + topic, "#D29922");

    // 删除主题列表中的项目
    for (int i = 0; i < m_topicsList->count(); ++i) {
        if (m_topicsList->item(i)->text() == topic) {
            delete m_topicsList->takeItem(i);
            break;
        }
    }

    Logger::instance().info("MQTT", "取消订阅: " + topic);
}

// ─── Slot: Process Ready Read ──────────────────────────────────────────

void MQTTWidget::onProcessReadyRead()
{
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardOutput()).trimmed();
    if (!output.isEmpty()) {
        appendOutput(output, "#E6EDF3");
    }
}

// ─── Slot: Process Finished ────────────────────────────────────────────

void MQTTWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    if (!m_process) return;

    QString err = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    if (!err.isEmpty() && exitCode != 0) {
        appendOutput(err, "#F85149");
    }

    m_process->deleteLater();
    m_process = nullptr;
}

// ─── Topic History ─────────────────────────────────────────────────────

void MQTTWidget::addTopicToHistory(const QString& topic)
{
    // 去重
    for (int i = 0; i < m_topicsList->count(); ++i) {
        if (m_topicsList->item(i)->text() == topic) return;
    }
    m_topicsList->addItem(topic);
}

// ─── Clear ─────────────────────────────────────────────────────────────

void MQTTWidget::onClearOutput()
{
    m_outputText->clear();
}

void MQTTWidget::onClearTopics()
{
    m_topicsList->clear();
}
