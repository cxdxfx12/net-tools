#include "terminal/TelnetWidget.h"
#include "terminal/TerminalWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

// Telnet 协议常量
static const quint8 IAC  = 255;
static const quint8 WILL = 251;
static const quint8 WONT = 252;
static const quint8 DO   = 253;
static const quint8 DONT = 254;
static const quint8 SB   = 250;
static const quint8 SE   = 240;

TelnetWidget::TelnetWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

TelnetWidget::~TelnetWidget()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void TelnetWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // --- 连接配置 ---
    auto* configGroup = new QGroupBox("Telnet 连接");
    auto* configForm = new QFormLayout(configGroup);
    configForm->setSpacing(6);

    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("例如: 192.168.1.1 或 switch.example.com");
    configForm->addRow("主机地址:", m_hostEdit);

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(23);
    configForm->addRow("端口:", m_portSpin);

    m_encodingCombo = new QComboBox();
    m_encodingCombo->addItem("UTF-8",       "UTF-8");
    m_encodingCombo->addItem("GBK",         "GBK");
    m_encodingCombo->addItem("Shift-JIS",   "Shift-JIS");
    m_encodingCombo->addItem("Latin-1",     "Latin-1");
    configForm->addRow("编码:", m_encodingCombo);

    mainLayout->addWidget(configGroup);

    // --- 按钮栏与状态 ---
    auto* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    m_statusIcon = new QLabel();
    m_statusIcon->setFixedSize(16, 16);
    m_statusIcon->setStyleSheet(
        "background-color: #999999; border-radius: 8px;");
    controlLayout->addWidget(m_statusIcon);

    m_statusLabel = new QLabel("未连接");
    controlLayout->addWidget(m_statusLabel);

    controlLayout->addStretch();

    m_connectBtn = new QPushButton("连接");
    m_disconnectBtn = new QPushButton("断开");
    m_disconnectBtn->setEnabled(false);

    controlLayout->addWidget(m_connectBtn);
    controlLayout->addWidget(m_disconnectBtn);
    mainLayout->addLayout(controlLayout);

    // --- 终端区域 ---
    m_terminal = new TerminalWidget(this);
    m_terminal->setConnected(false);
    mainLayout->addWidget(m_terminal, 1);
}

void TelnetWidget::setupConnections()
{
    connect(m_connectBtn, &QPushButton::clicked,
            this, &TelnetWidget::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked,
            this, &TelnetWidget::onDisconnectClicked);
    connect(m_terminal, &TerminalWidget::dataReady,
            this, &TelnetWidget::onTerminalDataReady);
}

void TelnetWidget::setState(TelnetState state)
{
    if (m_state == state) return;
    m_state = state;

    switch (state) {
    case TelnetState::Disconnected:
        m_statusIcon->setStyleSheet(
            "background-color: #999999; border-radius: 8px;");
        m_statusLabel->setText("未连接");
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
        m_hostEdit->setEnabled(true);
        m_portSpin->setEnabled(true);
        m_terminal->setConnected(false);
        break;
    case TelnetState::Connecting:
        m_statusIcon->setStyleSheet(
            "background-color: #FFA500; border-radius: 8px;");
        m_statusLabel->setText("正在连接...");
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
        m_hostEdit->setEnabled(false);
        m_portSpin->setEnabled(false);
        break;
    case TelnetState::Connected:
        m_statusIcon->setStyleSheet(
            "background-color: #00AA00; border-radius: 8px;");
        m_statusLabel->setText(QString("已连接 — %1:%2")
                               .arg(m_hostEdit->text().trimmed())
                               .arg(m_portSpin->value()));
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
        m_hostEdit->setEnabled(false);
        m_portSpin->setEnabled(false);
        m_terminal->setConnected(true);
        break;
    case TelnetState::Disconnecting:
        m_statusIcon->setStyleSheet(
            "background-color: #FFA500; border-radius: 8px;");
        m_statusLabel->setText("正在断开...");
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(false);
        break;
    case TelnetState::Error:
        m_statusIcon->setStyleSheet(
            "background-color: #CC0000; border-radius: 8px;");
        m_statusLabel->setText("连接错误");
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
        m_hostEdit->setEnabled(true);
        m_portSpin->setEnabled(true);
        m_terminal->setConnected(false);
        break;
    }

    emit stateChanged(state);
    if (state == TelnetState::Connected) emit connected();
    if (state == TelnetState::Disconnected) emit disconnected();
}

void TelnetWidget::onConnectClicked()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入主机地址。");
        return;
    }

    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_readBuffer.clear();
    m_terminal->clearTerminal();

    setState(TelnetState::Connecting);

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected,
            this, &TelnetWidget::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TelnetWidget::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &TelnetWidget::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TelnetWidget::onSocketReadyRead);

    Logger::instance().info("Telnet",
                            QString("正在连接 %1:%2 ...").arg(host).arg(m_portSpin->value()));

    m_socket->connectToHost(host, static_cast<quint16>(m_portSpin->value()));
}

void TelnetWidget::onDisconnectClicked()
{
    if (!m_socket) return;

    setState(TelnetState::Disconnecting);

    Logger::instance().info("Telnet",
                            QString("正在断开 %1:%2").arg(m_hostEdit->text().trimmed())
                                .arg(m_portSpin->value()));

    m_socket->disconnectFromHost();
}

void TelnetWidget::onSocketConnected()
{
    // 发送 Telnet DO ECHO 和 WILL ECHO，告知服务器支持回显协商
    QByteArray init;
    init.append(static_cast<char>(IAC));
    init.append(static_cast<char>(DO));
    init.append(static_cast<char>(1));   // ECHO
    init.append(static_cast<char>(IAC));
    init.append(static_cast<char>(WILL));
    init.append(static_cast<char>(1));   // ECHO
    init.append(static_cast<char>(IAC));
    init.append(static_cast<char>(WILL));
    init.append(static_cast<char>(3));   // SUPPRESS GO AHEAD
    m_socket->write(init);

    setState(TelnetState::Connected);

    Logger::instance().info("Telnet",
                            QString("已连接到 %1:%2").arg(m_hostEdit->text().trimmed())
                                .arg(m_portSpin->value()));
}

void TelnetWidget::onSocketDisconnected()
{
    Logger::instance().info("Telnet",
                            QString("连接已断开: %1:%2").arg(m_hostEdit->text().trimmed())
                                .arg(m_portSpin->value()));

    if (m_state == TelnetState::Disconnecting) {
        setState(TelnetState::Disconnected);
    } else {
        // 被动断开
        setState(TelnetState::Disconnected);
    }
}

void TelnetWidget::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errMsg = m_socket->errorString();

    Logger::instance().error("Telnet",
                             QString("连接错误: %1").arg(errMsg));

    setState(TelnetState::Error);
    emit errorOccurred(errMsg);
}

void TelnetWidget::onSocketReadyRead()
{
    if (!m_socket) return;

    QByteArray data = m_socket->readAll();
    m_readBuffer.append(data);

    QByteArray cleanData = stripTelnetCommands(m_readBuffer);
    m_readBuffer.clear();

    if (!cleanData.isEmpty()) {
        m_terminal->appendOutput(cleanData);
    }
}

void TelnetWidget::onTerminalDataReady(const QByteArray& data)
{
    writeToSocket(data);
}

void TelnetWidget::writeToSocket(const QByteArray& data)
{
    if (!m_socket || m_state != TelnetState::Connected) return;
    m_socket->write(data);
}

QByteArray TelnetWidget::stripTelnetCommands(const QByteArray& data)
{
    QByteArray result;
    result.reserve(data.size());

    int i = 0;
    const int len = data.size();

    while (i < len) {
        quint8 byte = static_cast<quint8>(data[i]);

        if (byte != IAC) {
            // 普通数据，直接输出
            result.append(data[i]);
            i++;
            continue;
        }

        // IAC 序列处理
        if (i + 1 >= len) {
            // IAC 在结尾，可能是半截序列，保留到下次
            m_readBuffer.append(data[i]);
            i++;
            break;
        }

        quint8 cmd = static_cast<quint8>(data[i + 1]);

        if (cmd == IAC) {
            // IAC IAC → 转义为单个 0xFF
            result.append(static_cast<char>(IAC));
            i += 2;
        } else if (cmd == WILL || cmd == WONT || cmd == DO || cmd == DONT) {
            // 3 字节选项协商序列: IAC <cmd> <option>
            if (i + 2 >= len) {
                // 半截序列，保留到下次
                m_readBuffer.append(data.mid(i));
                break;
            }
            i += 3;
        } else if (cmd == SB) {
            // 子协商: IAC SB ... IAC SE
            int j = i + 2;
            bool foundEnd = false;
            while (j < len - 1) {
                if (static_cast<quint8>(data[j]) == IAC &&
                    static_cast<quint8>(data[j + 1]) == SE) {
                    foundEnd = true;
                    break;
                }
                j++;
            }
            if (foundEnd) {
                i = j + 2; // 跳过 IAC SE
            } else {
                // 子协商跨包，保留到下次
                m_readBuffer.append(data.mid(i));
                break;
            }
        } else {
            // 其他 IAC 命令 (SE, NOP, DM, BRK, IP, AO, AYT, EC, EL, GA 等)
            // 均为 2 字节序列
            i += 2;
        }
    }

    return result;
}