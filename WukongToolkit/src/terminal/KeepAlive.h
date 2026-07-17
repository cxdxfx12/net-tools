#pragma once

#include <QObject>
#include <QTimer>

class SSHSession;

class KeepAlive : public QObject
{
    Q_OBJECT

public:
    explicit KeepAlive(SSHSession* session, QObject* parent = nullptr);

    void start(int intervalSec = 60);
    void stop();
    bool isRunning() const;
    int interval() const;

signals:
    void keepAliveSent();
    void keepAliveFailed();
    void keepAliveRecovered();

private slots:
    void sendKeepAlive();

private:
    SSHSession* m_session;
    QTimer* m_timer;
    int m_intervalSec = 60;
    int m_failCount = 0;
    static constexpr int MAX_FAIL_COUNT = 3;
};