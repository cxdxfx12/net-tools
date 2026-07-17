#include "terminal/SSHThread.h"
#include "terminal/SSHConnection.h"
#include "terminal/SSHAuth.h"
#include "terminal/SSHChannel.h"
#include "log/Logger.h"
#include <libssh2.h>
#include <QElapsedTimer>

SSHThread::SSHThread(const SSHConfig& config, const QString& sessionId,
                     QObject* parent)
    : QThread(parent)
    , m_config(config)
    , m_sessionId(sessionId)
{
}

SSHThread::~SSHThread()
{
    stop();
}

void SSHThread::stop()
{
    m_running = false;
    m_sendCondition.wakeAll();
}

void SSHThread::sendData(const QByteArray& data)
{
    QMutexLocker locker(&m_sendMutex);
    m_sendBuffer.append(data);
    m_sendCondition.wakeAll();
}

void SSHThread::run()
{
    Logger::instance().info("SSH", QString("Thread started for %1").arg(m_config.host));
    emit stateChanged(SessionState::Connecting);

    if (!connectAndAuthenticate()) {
        return;
    }

    emit stateChanged(SessionState::Connected);
    Logger::instance().info("SSH", QString("SSH Core connected to %1:%2")
                            .arg(m_config.host).arg(m_config.port));

    readLoop();
}

bool SSHThread::connectAndAuthenticate()
{
    // Step 1: TCP + SSH Handshake
    m_connection = new SSHConnection();
    m_connection->connectToHost(m_config.host, m_config.port, m_config.connectTimeout);

    // Wait for connection (non-blocking polling)
    while (m_running && !m_connection->isConnected()) {
        if (m_connection->state() == SSHConnectionState::Error) {
            QString error = m_connection->errorString();
            Logger::instance().error("SSH", "Connection failed: " + error);
            emit errorOccurred(error);
            emit stateChanged(SessionState::Error);
            delete m_connection;
            m_connection = nullptr;
            return false;
        }
        msleep(10);
    }

    if (!m_connection->isConnected()) {
        delete m_connection;
        m_connection = nullptr;
        return false;
    }

    // Step 2: Authentication
    SSHAuth auth(m_connection->session());

    if (!m_config.privateKey.isEmpty()) {
        auth.setKeyAuth(m_config.username, m_config.privateKey + ".pub",
                        m_config.privateKey, m_config.passphrase);
    } else {
        auth.setCredentials(m_config.username, m_config.password);
    }

    if (!auth.authenticate()) {
        QString error = auth.errorString();
        Logger::instance().error("SSH", "Authentication failed: " + error);
        emit errorOccurred(error);
        emit stateChanged(SessionState::Error);
        m_connection->disconnect();
        delete m_connection;
        m_connection = nullptr;
        return false;
    }

    // Step 3: Open channel + PTY + Shell
    m_channel = new SSHChannel(m_connection->session());

    if (!m_channel->openSession()) {
        QString error = m_channel->errorString();
        emit errorOccurred(error);
        emit stateChanged(SessionState::Error);
        m_connection->disconnect();
        delete m_channel;
        m_channel = nullptr;
        delete m_connection;
        m_connection = nullptr;
        return false;
    }

    if (!m_channel->requestPty("xterm-256color", 80, 24)) {
        QString error = m_channel->errorString();
        emit errorOccurred(error);
        emit stateChanged(SessionState::Error);
        m_connection->disconnect();
        delete m_channel;
        m_channel = nullptr;
        delete m_connection;
        m_connection = nullptr;
        return false;
    }

    if (!m_channel->shell()) {
        QString error = m_channel->errorString();
        emit errorOccurred(error);
        emit stateChanged(SessionState::Error);
        m_connection->disconnect();
        delete m_channel;
        m_channel = nullptr;
        delete m_connection;
        m_connection = nullptr;
        return false;
    }

    m_running = true;
    return true;
}

void SSHThread::readLoop()
{
    QElapsedTimer keepAliveTimer;
    keepAliveTimer.start();

    while (m_running && m_channel && m_channel->isOpen()) {
        // Step 1: Flush send queue
        {
            QMutexLocker locker(&m_sendMutex);
            if (!m_sendBuffer.isEmpty()) {
                int written = m_channel->write(m_sendBuffer);
                if (written > 0) {
                    m_sendBuffer.remove(0, written);
                }
                m_sendCondition.notify_all();
            }
        }

        // Step 2: Read from channel
        QByteArray data = m_channel->read(READ_BUF_SIZE);
        if (!data.isEmpty()) {
            emit dataReceived(data);
        }

        // Step 3: Check EOF
        if (m_channel->isEOF()) {
            Logger::instance().info("SSH", "Channel EOF for " + m_config.host);
            break;
        }

        // Step 4: KeepAlive
        if (m_config.keepAlive && keepAliveTimer.elapsed() >= KEEPALIVE_INTERVAL_MS) {
            sendKeepAlive();
            keepAliveTimer.restart();
        }

        // Step 5: Wait for pending sends
        {
            QMutexLocker locker(&m_sendMutex);
            if (m_sendBuffer.isEmpty()) {
                m_sendCondition.wait(&m_sendMutex, 5); // 5ms timeout
            }
        }

        // Give CPU a break
        msleep(2);
    }

    Logger::instance().info("SSH", QString("SSH thread read loop ended for %1").arg(m_config.host));

    // Cleanup
    if (m_channel) {
        m_channel->close();
        delete m_channel;
        m_channel = nullptr;
    }
    if (m_connection) {
        m_connection->disconnect();
        delete m_connection;
        m_connection = nullptr;
    }

    emit stateChanged(SessionState::Closed);
}

void SSHThread::sendKeepAlive()
{
    if (m_channel && m_channel->isOpen()) {
        // Send SSH_MSG_IGNORE via libssh2 keepalive
        int seconds = 0;
        int rc = libssh2_keepalive_send(m_connection->session(), &seconds);
        if (rc == 0) {
            Logger::instance().debug("SSH", "KeepAlive sent to " + m_config.host);
        }
    }
}