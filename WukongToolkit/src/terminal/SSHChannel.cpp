#include "terminal/SSHChannel.h"
#include "log/Logger.h"

#include <libssh2.h>

SSHChannel::SSHChannel(LIBSSH2_SESSION* session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
}

SSHChannel::~SSHChannel()
{
    close();
}

bool SSHChannel::openSession()
{
    if (!m_session) {
        m_errorString = "No SSH session";
        return false;
    }

    m_channel = libssh2_channel_open_session(m_session);
    if (!m_channel) {
        // Check for EAGAIN
        char* errMsg = nullptr;
        int err = libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        if (err == LIBSSH2_ERROR_EAGAIN) {
            // Retry once
            m_channel = libssh2_channel_open_session(m_session);
        }
        if (!m_channel) {
            m_errorString = QString("Failed to open channel: %1")
                                .arg(errMsg ? errMsg : "unknown");
            Logger::instance().error("SSH", m_errorString);
            emit errorOccurred(m_errorString);
            return false;
        }
    }

    Logger::instance().info("SSH", "Channel opened");
    emit channelOpened();
    return true;
}

bool SSHChannel::requestPty(const QString& term, int cols, int rows)
{
    if (!m_channel) {
        m_errorString = "No channel open";
        return false;
    }

    int rc;
    while ((rc = libssh2_channel_request_pty(m_channel, term.toUtf8().constData())) ==
           LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("PTY request failed: %1").arg(errMsg ? errMsg : "unknown");
        Logger::instance().error("SSH", m_errorString);
        emit errorOccurred(m_errorString);
        return false;
    }

    // Set terminal size
    setTerminalSize(cols, rows);

    Logger::instance().info("SSH", "PTY allocated: " + term);
    return true;
}

bool SSHChannel::setTerminalSize(int cols, int rows)
{
    if (!m_channel) return false;

    libssh2_channel_request_pty_size(m_channel, cols, rows);
    return true;
}

bool SSHChannel::shell()
{
    if (!m_channel) {
        m_errorString = "No channel open";
        return false;
    }

    int rc;
    while ((rc = libssh2_channel_shell(m_channel)) == LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Shell request failed: %1").arg(errMsg ? errMsg : "unknown");
        Logger::instance().error("SSH", m_errorString);
        emit errorOccurred(m_errorString);
        return false;
    }

    Logger::instance().info("SSH", "Shell started");
    return true;
}

bool SSHChannel::exec(const QString& command)
{
    if (!m_channel) {
        m_errorString = "No channel open";
        return false;
    }

    int rc;
    while ((rc = libssh2_channel_exec(m_channel, command.toUtf8().constData())) ==
           LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Exec failed: %1").arg(errMsg ? errMsg : "unknown");
        Logger::instance().error("SSH", m_errorString);
        emit errorOccurred(m_errorString);
        return false;
    }

    Logger::instance().info("SSH", "Command executed: " + command);
    return true;
}

void SSHChannel::close()
{
    if (m_channel) {
        libssh2_channel_send_eof(m_channel);
        libssh2_channel_close(m_channel);
        libssh2_channel_free(m_channel);
        m_channel = nullptr;
        Logger::instance().info("SSH", "Channel closed");
        emit channelClosed();
    }
}

QByteArray SSHChannel::read(int maxSize)
{
    if (!m_channel) return {};

    QByteArray buffer(maxSize, Qt::Uninitialized);
    int rc = libssh2_channel_read(m_channel, buffer.data(), maxSize);

    if (rc == LIBSSH2_ERROR_EAGAIN) {
        return {};
    }
    if (rc < 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Read error: %1").arg(errMsg ? errMsg : "unknown");
        emit errorOccurred(m_errorString);
        return {};
    }

    buffer.resize(rc);
    return buffer;
}

int SSHChannel::write(const QByteArray& data)
{
    if (!m_channel) return -1;

    int rc;
    while ((rc = libssh2_channel_write(m_channel, data.constData(), data.size())) ==
           LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc < 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Write error: %1").arg(errMsg ? errMsg : "unknown");
        emit errorOccurred(m_errorString);
    }

    return rc;
}

bool SSHChannel::isEOF() const
{
    if (!m_channel) return true;
    return libssh2_channel_eof(m_channel) != 0;
}

bool SSHChannel::sendSignal(const QString& signal)
{
    if (!m_channel) return false;
    return libssh2_channel_process_startup(m_channel, "signal",
                                           static_cast<unsigned int>(signal.size()),
                                           signal.toUtf8().constData(),
                                           static_cast<unsigned int>(signal.size())) == 0;
}

bool SSHChannel::sendBreak(int durationMs)
{
    Q_UNUSED(durationMs)
    if (!m_channel) return false;
    // libssh2_channel_send_break not available in this version
    return libssh2_channel_eof(m_channel) == 0;
}