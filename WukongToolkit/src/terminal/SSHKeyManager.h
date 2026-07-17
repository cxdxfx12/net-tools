#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

struct KeyInfo
{
    QString name;
    QString path;
    QString type; // RSA, DSA, ECDSA, ED25519
    int bits = 0;
    QString fingerprint;
    QString comment;
    QByteArray publicKey;
};

class SSHKeyManager : public QObject
{
    Q_OBJECT

public:
    explicit SSHKeyManager(QObject* parent = nullptr);

    QList<KeyInfo> loadKeys(const QString& keyDir = QString());
    KeyInfo loadKey(const QString& privateKeyPath, const QString& passphrase = QString());
    bool validateKey(const QString& privateKeyPath, const QString& passphrase = QString());

    bool generateKeyPair(const QString& path, const QString& type = "RSA",
                         int bits = 2048, const QString& comment = QString());
    QString fingerprint(const QString& publicKeyPath) const;

    QString defaultKeyDir() const;

signals:
    void keyLoaded(const QString& name);
    void keyLoadFailed(const QString& path, const QString& error);
    void keyGenerated(const QString& path);

private:
    KeyInfo parsePublicKey(const QString& publicKeyPath) const;
    QString findPublicKey(const QString& privateKeyPath) const;

    QString m_defaultKeyDir;
};