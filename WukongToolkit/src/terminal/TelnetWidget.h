#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTcpSocket>
#include <QByteArray>

class TerminalWidget;

enum class TelnetState
{
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

class TelnetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TelnetWidget(QWidget* parent = nullptr);
    ~TelnetWidget() override;

    TelnetState state() const { return m_state; }
    bool isConnected() const { return m_state == TelnetState::Connected; }

signals:
    void stateChanged(TelnetState state);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketReadyRead();
    void onTerminalDataReady(const QByteArray& data);

private:
    void setupUI();
    void setupConnections();
    void setState(TelnetState state);
    QByteArray stripTelnetCommands(const QByteArray& data);
    void writeToSocket(const QByteArray& data);

    // UI
    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QComboBox* m_encodingCombo;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel* m_statusLabel;
    QLabel* m_statusIcon;
    TerminalWidget* m_terminal;

    // Connection
    QTcpSocket* m_socket = nullptr;
    TelnetState m_state = TelnetState::Disconnected;
    QByteArray m_readBuffer;
};