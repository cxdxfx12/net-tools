#include "terminal/SSHSession.h"
#include "terminal/SSHThread.h"
#include "log/Logger.h"
#include <QUuid>
#include <QTimer>

SSHSession::SSHSession(const SSHConfig& config, QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_config(config)
{
    Logger::instance().info("SSH", QString("Session created: %1@%2:%3")
                            .arg(config.username, config.host).arg(config.port));
}

SSHSession::~SSHSession()
{
    disconnect();
    Logger::instance().info("SSH", QString("Session destroyed: %1").arg(m_config.host));
}

void SSHSession::connectToHost()
{
    if (m_state == SessionState::Connected || m_state == SessionState::Running) {
        Logger::instance().warning("SSH", "Already connected to " + m_config.host);
        return;
    }

    setState(SessionState::Connecting);
    Logger::instance().info("SSH", "Connecting to " + m_config.host);

    m_thread = std::make_unique<SSHThread>(m_config, m_id);
    connect(m_thread.get(), &SSHThread::dataReceived,
            this, &SSHSession::onThreadDataReceived);
    connect(m_thread.get(), &SSHThread::errorOccurred,
            this, &SSHSession::onThreadError);
    connect(m_thread.get(), &SSHThread::stateChanged,
            this, &SSHSession::onThreadStateChanged);

    m_thread->start();
}

void SSHSession::disconnect()
{
    if (m_thread) {
        m_thread->stop();
        m_thread->wait(3000);
        m_thread.reset();
    }
    setState(SessionState::Closed);
}

void SSHSession::sendData(const QByteArray& data)
{
    if (m_thread && m_state == SessionState::Running) {
        m_thread->sendData(data);
        m_lastActiveTime = QDateTime::currentDateTime();
    }
}

bool SSHSession::isConnected() const
{
    return m_state == SessionState::Connected || m_state == SessionState::Running;
}

void SSHSession::setState(SessionState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void SSHSession::onThreadDataReceived(const QByteArray& data)
{
    m_lastActiveTime = QDateTime::currentDateTime();
    if (m_state == SessionState::Connected) {
        setState(SessionState::Running);
    }
    emit dataReceived(data);
}

void SSHSession::onThreadError(const QString& error)
{
    Logger::instance().error("SSH", m_config.host + " - " + error);
    setState(SessionState::Error);
    emit connectionError(error);

    if (m_reconnectCount < MAX_RECONNECT) {
        startReconnect();
    }
}

void SSHSession::onThreadStateChanged(SessionState state)
{
    setState(state);
}

void SSHSession::startReconnect()
{
    m_reconnectCount++;
    int delay = m_reconnectCount * 1000; // 1s, 2s, 3s, 4s, 5s
    Logger::instance().info("SSH", QString("Reconnecting in %1s (attempt %2/%3)")
                            .arg(delay / 1000).arg(m_reconnectCount).arg(MAX_RECONNECT));

    emit reconnecting(m_reconnectCount);

    QTimer::singleShot(delay, this, [this]() {
        if (m_state == SessionState::Error) {
            connectToHost();
        }
    });
}