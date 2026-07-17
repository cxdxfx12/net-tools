#include "terminal/SSHAuth.h"
#include "log/Logger.h"

#include <libssh2.h>

SSHAuth::SSHAuth(LIBSSH2_SESSION* session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
}

void SSHAuth::setCredentials(const QString& username, const QString& password)
{
    m_username = username;
    m_password = password;
    m_method = SSHAuthMethod::Password;
}

void SSHAuth::setKeyAuth(const QString& username, const QString& publicKeyPath,
                         const QString& privateKeyPath, const QString& passphrase)
{
    m_username = username;
    m_publicKeyPath = publicKeyPath;
    m_privateKeyPath = privateKeyPath;
    m_passphrase = passphrase;
    m_method = SSHAuthMethod::PublicKey;
}

bool SSHAuth::authenticate()
{
    if (!m_session) {
        m_errorString = "No SSH session available";
        return false;
    }

    emit authenticationStarted();

    switch (m_method) {
    case SSHAuthMethod::Password:
        return authenticatePassword();
    case SSHAuthMethod::PublicKey:
        return authenticatePublicKey();
    default:
        // Try keyboard-interactive as fallback
        return authenticateKeyboardInteractive();
    }
}

QStringList SSHAuth::availableMethods()
{
    if (!m_session) return {};

    char* userAuthList = libssh2_userauth_list(m_session, m_username.toUtf8().constData(),
                                               m_username.toUtf8().size());
    if (!userAuthList) {
        // Check if already authenticated
        if (libssh2_userauth_authenticated(m_session)) {
            return {"authenticated"};
        }
        return {};
    }

    QStringList methods;
    QString methodsStr = QString::fromUtf8(userAuthList);
    for (const auto& method : methodsStr.split(',')) {
        methods.append(method.trimmed());
    }
    return methods;
}

bool SSHAuth::authenticatePassword()
{
    Logger::instance().info("SSH", "Authenticating with password for " + m_username);

    int rc;
    while ((rc = libssh2_userauth_password(m_session,
                                           m_username.toUtf8().constData(),
                                           m_password.toUtf8().constData())) ==
           LIBSSH2_ERROR_EAGAIN) {
        // Wait for socket to be ready
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Password authentication failed: %1")
                            .arg(errMsg ? errMsg : "unknown error");
        Logger::instance().error("SSH", m_errorString);
        emit authenticationFailed(m_errorString);
        return false;
    }

    Logger::instance().info("SSH", "Password authentication successful");
    emit authenticationSuccess();
    return true;
}

bool SSHAuth::authenticatePublicKey()
{
    Logger::instance().info("SSH", "Authenticating with public key for " + m_username);

    QByteArray passphraseBytes = m_passphrase.toUtf8();
    const char* passphrase = passphraseBytes.isEmpty() ? nullptr : passphraseBytes.constData();

    int rc;
    while ((rc = libssh2_userauth_publickey_fromfile(
                m_session,
                m_username.toUtf8().constData(),
                m_publicKeyPath.isEmpty() ? nullptr : m_publicKeyPath.toUtf8().constData(),
                m_privateKeyPath.toUtf8().constData(),
                passphrase)) == LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Public key authentication failed: %1")
                            .arg(errMsg ? errMsg : "unknown error");
        Logger::instance().error("SSH", m_errorString);
        emit authenticationFailed(m_errorString);
        return false;
    }

    Logger::instance().info("SSH", "Public key authentication successful");
    emit authenticationSuccess();
    return true;
}

bool SSHAuth::authenticateKeyboardInteractive()
{
    Logger::instance().info("SSH", "Authenticating with keyboard-interactive for " + m_username);

    // Use keyboard-interactive with auto-respond using password
    int rc;
    while ((rc = libssh2_userauth_keyboard_interactive_ex(
                m_session,
                m_username.toUtf8().constData(),
                static_cast<unsigned int>(m_username.toUtf8().size()),
                [](const char* name, int nameLen,
                   const char* instruction, int instructionLen,
                   int numPrompts,
                   const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
                   LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
                   void** abstract) {
                    Q_UNUSED(name) Q_UNUSED(nameLen)
                    Q_UNUSED(instruction) Q_UNUSED(instructionLen)
                    Q_UNUSED(abstract)
                    if (responses && prompts && numPrompts > 0) {
                        // Auto-respond with empty string
                        responses[0].text = strdup("");
                        responses[0].length = 0;
                    }
                })) == LIBSSH2_ERROR_EAGAIN) {
        // Wait
    }

    if (rc != 0) {
        char* errMsg = nullptr;
        libssh2_session_last_error(m_session, &errMsg, nullptr, 0);
        m_errorString = QString("Keyboard-interactive authentication failed: %1")
                            .arg(errMsg ? errMsg : "unknown error");
        Logger::instance().error("SSH", m_errorString);
        emit authenticationFailed(m_errorString);
        return false;
    }

    Logger::instance().info("SSH", "Keyboard-interactive authentication successful");
    emit authenticationSuccess();
    return true;
}