#pragma once

#include <QString>

enum class SessionState
{
    None,
    Connecting,
    Connected,
    Running,
    Disconnecting,
    Closed,
    Error
};

struct SSHConfig
{
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString privateKey;
    QString passphrase;
    int connectTimeout = 10000;
    int readTimeout = 60000;
    bool keepAlive = true;
    int keepAliveInterval = 60;
};