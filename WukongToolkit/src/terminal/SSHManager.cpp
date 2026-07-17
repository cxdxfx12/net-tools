#include "terminal/SSHManager.h"
#include "log/Logger.h"

SSHManager::SSHManager(QObject* parent)
    : QObject(parent)
{
}

SSHManager::~SSHManager()
{
    closeAllSessions();
}

SSHManager& SSHManager::instance()
{
    static SSHManager s_instance;
    return s_instance;
}

SSHSession* SSHManager::createSession(const SSHConfig& config)
{
    auto* session = new SSHSession(config, this);
    m_sessions.insert(session->id(), session);

    connect(session, &SSHSession::connectionError, this, [this, id = session->id()](const QString& error) {
        emit sessionError(id, error);
    });

    Logger::instance().info("SSH", QString("Session created: %1 (total: %2)")
                            .arg(config.host).arg(m_sessions.size()));
    emit sessionCreated(session->id());
    return session;
}

void SSHManager::closeSession(const QString& sessionId)
{
    if (auto* s = m_sessions.value(sessionId)) {
        QString host = s->host();
        s->disconnect();
        s->deleteLater();
        m_sessions.remove(sessionId);
        Logger::instance().info("SSH", QString("Session closed: %1 (remaining: %2)")
                                .arg(host).arg(m_sessions.size()));
        emit sessionClosed(sessionId);
    }
}

void SSHManager::closeAllSessions()
{
    for (auto* s : m_sessions) {
        s->disconnect();
        s->deleteLater();
    }
    m_sessions.clear();
    Logger::instance().info("SSH", "All sessions closed");
}

SSHSession* SSHManager::session(const QString& sessionId) const
{
    return m_sessions.value(sessionId);
}

QList<SSHSession*> SSHManager::allSessions() const
{
    return m_sessions.values();
}

int SSHManager::sessionCount() const
{
    return m_sessions.size();
}