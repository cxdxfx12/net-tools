#include "terminal/SSHConnection.h"
#include "log/Logger.h"

#include <libssh2.h>

SSHConnection::SSHConnection(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);

    connect(m_socket, &QTcpSocket::connected, this, &SSHConnection::onSocketConnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &SSHConnection::onSocketError);
    connect(m_socket, &QTcpSocket::disconnected, this, &SSHConnection::onSocketDisconnected);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SSHConnection::onTimeout);
}

SSHConnection::~SSHConnection()
{
    disconnect();
}

void SSHConnection::connectToHost(const QString& host, int port, int timeoutMs)
{
    if (m_state != SSHConnectionState::Disconnected &&
        m_state != SSHConnectionState::Error) {
        Logger::instance().warning("SSH", "Already connecting or connected");
        return;
    }

    m_host = host;
    m_port = port;
    m_errorString.clear();
    m_banner.clear();

    setState(SSHConnectionState::Resolving);
    Logger::instance().info("SSH", QString("Connecting to %1:%2").arg(host).arg(port));

    m_timeoutTimer->start(timeoutMs);
    m_socket->connectToHost(host, port);
}

void SSHConnection::disconnect()
{
    setState(SSHConnectionState::Disconnecting);
    m_timeoutTimer->stop();
    cleanup();
    setState(SSHConnectionState::Disconnected);
    emit disconnected();
}

bool SSHConnection::isConnected() const
{
    return m_state == SSHConnectionState::Connected;
}

void SSHConnection::setState(SSHConnectionState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void SSHConnection::onSocketConnected()
{
    Logger::instance().info("SSH", "TCP connected to " + m_host);
    setState(SSHConnectionState::Connecting);
    performHandshake();
}

void SSHConnection::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    m_errorString = m_socket->errorString();
    Logger::instance().error("SSH", "Socket error: " + m_errorString);
    setState(SSHConnectionState::Error);
    emit errorOccurred(m_errorString);
}

void SSHConnection::onSocketDisconnected()
{
    Logger::instance().info("SSH", "Socket disconnected from " + m_host);
    cleanup();
    if (m_state != SSHConnectionState::Disconnecting) {
        setState(SSHConnectionState::Disconnected);
        emit disconnected();
    }
}

void SSHConnection::onTimeout()
{
    m_errorString = "Connection timeout";
    Logger::instance().error("SSH", m_errorString);
    setState(SSHConnectionState::Error);
    emit errorOccurred(m_errorString);
    cleanup();
}

void SSHConnection::performHandshake()
{
    setState(SSHConnectionState::Handshaking);

    // Initialize libssh2 session
    int rc = libssh2_init(0);
    if (rc != 0) {
        m_errorString = QString("libssh2_init failed: %1").arg(rc);
        Logger::instance().error("SSH", m_errorString);
        setState(SSHConnectionState::Error);
        emit errorOccurred(m_errorString);
        return;
    }

    m_session = libssh2_session_init();
    if (!m_session) {
        m_errorString = "libssh2_session_init failed";
        Logger::instance().error("SSH", m_errorString);
        setState(SSHConnectionState::Error);
        emit errorOccurred(m_errorString);
        return;
    }

    // Set to non-blocking mode
    libssh2_session_set_blocking(m_session, 0);

    // Perform handshake
    while ((rc = libssh2_session_handshake(m_session, m_socket->socketDescriptor())) ==
           LIBSSH2_ERROR_EAGAIN) {
        if (!m_socket->waitForReadyRead(100)) {
            if (m_state == SSHConnectionState::Disconnecting) return;
        }
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Handshake failed: %1").arg(errMsg ? errMsg : "unknown");
        Logger::instance().error("SSH", m_errorString);
        setState(SSHConnectionState::Error);
        emit errorOccurred(m_errorString);
        return;
    }

    // Get server banner
    const char* banner = libssh2_session_banner_get(m_session);
    if (banner) {
        m_banner = QString::fromUtf8(banner);
        emit bannerReceived(m_banner);
    }

    Logger::instance().info("SSH", "Handshake completed with " + m_host);
    setState(SSHConnectionState::Connected);
    emit connected();
}

void SSHConnection::cleanup()
{
    if (m_session) {
        libssh2_session_disconnect(m_session, "Client disconnect");
        libssh2_session_free(m_session);
        m_session = nullptr;
        libssh2_exit();
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
}