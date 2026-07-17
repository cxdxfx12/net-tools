#pragma once

#include "SSHConfig.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QByteArray>

class SSHConnection;
class SSHAuth;
class SSHChannel;

class SSHThread : public QThread
{
    Q_OBJECT

public:
    explicit SSHThread(const SSHConfig& config, const QString& sessionId,
                       QObject* parent = nullptr);
    ~SSHThread() override;

    void stop();
    void sendData(const QByteArray& data);

signals:
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& error);
    void stateChanged(SessionState state);

protected:
    void run() override;

private:
    bool connectAndAuthenticate();
    void readLoop();
    void sendKeepAlive();

    SSHConfig m_config;
    QString m_sessionId;
    bool m_running = false;

    SSHConnection* m_connection = nullptr;
    SSHChannel* m_channel = nullptr;

    // Send queue
    QMutex m_sendMutex;
    QByteArray m_sendBuffer;
    QWaitCondition m_sendCondition;

    // KeepAlive
    static constexpr int KEEPALIVE_INTERVAL_MS = 60000;
    static constexpr int READ_BUF_SIZE = 16384;
};