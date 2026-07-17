#pragma once

#include "SSHConfig.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <memory>

class SSHThread;

class SSHSession : public QObject
{
    Q_OBJECT

public:
    explicit SSHSession(const SSHConfig& config, QObject* parent = nullptr);
    ~SSHSession() override;

    QString id() const { return m_id; }
    QString host() const { return m_config.host; }
    int port() const { return m_config.port; }
    QString username() const { return m_config.username; }
    SessionState state() const { return m_state; }

    void connectToHost();
    void disconnect();
    void sendData(const QByteArray& data);
    bool isConnected() const;

signals:
    void stateChanged(SessionState newState);
    void dataReceived(const QByteArray& data);
    void connectionError(const QString& error);
    void reconnecting(int attempt);

private slots:
    void onThreadDataReceived(const QByteArray& data);
    void onThreadError(const QString& error);
    void onThreadStateChanged(SessionState state);

private:
    void setState(SessionState state);
    void startReconnect();

    QString m_id;
    SSHConfig m_config;
    SessionState m_state = SessionState::None;
    std::unique_ptr<SSHThread> m_thread;
    QDateTime m_lastActiveTime;
    int m_reconnectCount = 0;
    static constexpr int MAX_RECONNECT = 5;
};