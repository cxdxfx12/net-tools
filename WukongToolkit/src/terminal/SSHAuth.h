#pragma once

#include <QObject>
#include <QString>

struct _LIBSSH2_SESSION;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

enum class SSHAuthMethod
{
    None,
    Password,
    PublicKey,
    KeyboardInteractive,
    Agent
};

class SSHAuth : public QObject
{
    Q_OBJECT

public:
    explicit SSHAuth(LIBSSH2_SESSION* session, QObject* parent = nullptr);

    void setCredentials(const QString& username, const QString& password);
    void setKeyAuth(const QString& username, const QString& publicKeyPath,
                    const QString& privateKeyPath, const QString& passphrase = QString());

    QString username() const { return m_username; }
    SSHAuthMethod method() const { return m_method; }

    bool authenticate();
    QStringList availableMethods();
    QString errorString() const { return m_errorString; }

signals:
    void authenticationStarted();
    void authenticationSuccess();
    void authenticationFailed(const QString& reason);
    void keyboardInteractivePrompt(const QString& name, const QString& instruction,
                                   const QStringList& prompts);

private:
    bool authenticatePassword();
    bool authenticatePublicKey();
    bool authenticateKeyboardInteractive();

    LIBSSH2_SESSION* m_session;
    QString m_username;
    QString m_password;
    QString m_publicKeyPath;
    QString m_privateKeyPath;
    QString m_passphrase;
    SSHAuthMethod m_method = SSHAuthMethod::None;
    QString m_errorString;
};