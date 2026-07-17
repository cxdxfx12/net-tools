#pragma once

#include "SSHConfig.h"
#include "SSHSession.h"
#include <QObject>
#include <QMap>
#include <memory>

class SSHManager : public QObject
{
    Q_OBJECT

public:
    static SSHManager& instance();

    SSHSession* createSession(const SSHConfig& config);
    void closeSession(const QString& sessionId);
    void closeAllSessions();
    SSHSession* session(const QString& sessionId) const;
    QList<SSHSession*> allSessions() const;
    int sessionCount() const;

signals:
    void sessionCreated(const QString& sessionId);
    void sessionClosed(const QString& sessionId);
    void sessionError(const QString& sessionId, const QString& error);

private:
    explicit SSHManager(QObject* parent = nullptr);
    ~SSHManager() override;
    SSHManager(const SSHManager&) = delete;
    SSHManager& operator=(const SSHManager&) = delete;

    QMap<QString, SSHSession*> m_sessions;
};