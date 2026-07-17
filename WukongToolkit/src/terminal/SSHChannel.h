#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

struct _LIBSSH2_SESSION;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;
struct _LIBSSH2_CHANNEL;
typedef struct _LIBSSH2_CHANNEL LIBSSH2_CHANNEL;

class SSHChannel : public QObject
{
    Q_OBJECT

public:
    explicit SSHChannel(LIBSSH2_SESSION* session, QObject* parent = nullptr);
    ~SSHChannel() override;

    bool openSession();
    bool requestPty(const QString& term = "xterm-256color",
                    int cols = 80, int rows = 24);
    bool setTerminalSize(int cols, int rows);
    bool shell();
    bool exec(const QString& command);
    void close();

    QByteArray read(int maxSize = 16384);
    int write(const QByteArray& data);
    bool isOpen() const { return m_channel != nullptr; }
    bool isEOF() const;

    // Send signals
    bool sendSignal(const QString& signal);
    bool sendBreak(int durationMs = 0);

    QString errorString() const { return m_errorString; }

signals:
    void channelOpened();
    void channelClosed();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& error);

private:
    LIBSSH2_SESSION* m_session;
    LIBSSH2_CHANNEL* m_channel = nullptr;
    QString m_errorString;
};