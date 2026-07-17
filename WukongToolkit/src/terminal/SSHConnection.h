#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

// Forward declare libssh2 types
struct _LIBSSH2_SESSION;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

enum class SSHConnectionState
{
    Disconnected,
    Resolving,
    Connecting,
    Handshaking,
    Authenticating,
    Connected,
    Disconnecting,
    Error
};

class SSHConnection : public QObject
{
    Q_OBJECT

public:
    explicit SSHConnection(QObject* parent = nullptr);
    ~SSHConnection() override;

    void connectToHost(const QString& host, int port, int timeoutMs = 10000);
    void disconnect();
    bool isConnected() const;
    SSHConnectionState state() const { return m_state; }
    LIBSSH2_SESSION* session() const { return m_session; }

    QString host() const { return m_host; }
    int port() const { return m_port; }

    QString errorString() const { return m_errorString; }
    QString banner() const { return m_banner; }

signals:
    void stateChanged(SSHConnectionState state);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void bannerReceived(const QString& banner);

private slots:
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketDisconnected();
    void onTimeout();

private:
    void setState(SSHConnectionState state);
    void performHandshake();
    void cleanup();

    QTcpSocket* m_socket;
    LIBSSH2_SESSION* m_session = nullptr;
    QTimer* m_timeoutTimer;

    QString m_host;
    int m_port = 22;
    SSHConnectionState m_state = SSHConnectionState::Disconnected;
    QString m_errorString;
    QString m_banner;
};