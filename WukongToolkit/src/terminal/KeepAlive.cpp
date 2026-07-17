#include "terminal/KeepAlive.h"
#include "terminal/SSHSession.h"
#include "log/Logger.h"

KeepAlive::KeepAlive(SSHSession* session, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &KeepAlive::sendKeepAlive);
}

void KeepAlive::start(int intervalSec)
{
    m_intervalSec = intervalSec;
    m_failCount = 0;
    m_timer->start(intervalSec * 1000);
    Logger::instance().debug("SSH", QString("KeepAlive started (interval: %1s)").arg(intervalSec));
}

void KeepAlive::stop()
{
    m_timer->stop();
    Logger::instance().debug("SSH", "KeepAlive stopped");
}

bool KeepAlive::isRunning() const
{
    return m_timer->isActive();
}

int KeepAlive::interval() const
{
    return m_intervalSec;
}

void KeepAlive::sendKeepAlive()
{
    if (!m_session || !m_session->isConnected()) {
        m_failCount++;
        Logger::instance().warning("SSH", QString("KeepAlive failed (%1/%2)")
                                   .arg(m_failCount).arg(MAX_FAIL_COUNT));
        emit keepAliveFailed();

        if (m_failCount >= MAX_FAIL_COUNT) {
            stop();
            Logger::instance().error("SSH", "KeepAlive: max failures reached, disconnecting");
            m_session->disconnect();
        }
        return;
    }

    // Send SSH_MSG_IGNORE via the session
    m_session->sendData(QByteArray()); // Trigger keepalive in thread
    m_failCount = 0;
    emit keepAliveSent();
}