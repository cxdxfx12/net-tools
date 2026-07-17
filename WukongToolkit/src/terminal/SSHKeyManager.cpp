#include "terminal/SSHKeyManager.h"
#include "log/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <libssh2.h>

SSHKeyManager::SSHKeyManager(QObject* parent)
    : QObject(parent)
{
    m_defaultKeyDir = QDir::homePath() + "/.ssh";
}

QString SSHKeyManager::defaultKeyDir() const
{
    return m_defaultKeyDir;
}

QList<KeyInfo> SSHKeyManager::loadKeys(const QString& keyDir)
{
    QList<KeyInfo> keys;
    QString dir = keyDir.isEmpty() ? m_defaultKeyDir : keyDir;

    QDir d(dir);
    if (!d.exists()) {
        return keys;
    }

    QStringList filters = {"id_rsa", "id_dsa", "id_ecdsa", "id_ed25519", "id_ecdsa_sk",
                           "id_ed25519_sk", "*.pem", "*.key"};
    QStringList entries = d.entryList(filters, QDir::Files | QDir::Readable);

    for (const auto& entry : entries) {
        QString privatePath = d.absoluteFilePath(entry);
        QString publicPath = findPublicKey(privatePath);

        KeyInfo info;
        info.name = entry;
        info.path = privatePath;

        if (!publicPath.isEmpty()) {
            KeyInfo pubInfo = parsePublicKey(publicPath);
            info.type = pubInfo.type;
            info.bits = pubInfo.bits;
            info.fingerprint = fingerprint(publicPath);
            info.comment = pubInfo.comment;
            info.publicKey = pubInfo.publicKey;
        }

        keys.append(info);
        Logger::instance().info("SSH", "Key loaded: " + entry);
    }

    return keys;
}

KeyInfo SSHKeyManager::loadKey(const QString& privateKeyPath, const QString& passphrase)
{
    KeyInfo info;
    QFileInfo fi(privateKeyPath);
    if (!fi.exists()) {
        Logger::instance().error("SSH", "Key file not found: " + privateKeyPath);
        return info;
    }

    info.name = fi.fileName();
    info.path = privateKeyPath;

    QString publicPath = findPublicKey(privateKeyPath);
    if (!publicPath.isEmpty()) {
        KeyInfo pubInfo = parsePublicKey(publicPath);
        info.type = pubInfo.type;
        info.bits = pubInfo.bits;
        info.fingerprint = fingerprint(publicPath);
        info.comment = pubInfo.comment;
        info.publicKey = pubInfo.publicKey;
    }

    emit keyLoaded(info.name);
    return info;
}

bool SSHKeyManager::validateKey(const QString& privateKeyPath, const QString& passphrase)
{
    QFileInfo fi(privateKeyPath);
    if (!fi.exists()) {
        Logger::instance().error("SSH", "Key file not found: " + privateKeyPath);
        return false;
    }

    // Use ssh-keygen to validate
    QProcess process;
    QStringList args;
    args << "-y" << "-f" << privateKeyPath;
    if (!passphrase.isEmpty()) {
        args << "-P" << passphrase;
    }

    process.start("ssh-keygen", args);
    process.waitForFinished(5000);

    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        Logger::instance().error("SSH", "Key validation failed: " + error);
        emit keyLoadFailed(privateKeyPath, error);
        return false;
    }

    return true;
}

bool SSHKeyManager::generateKeyPair(const QString& path, const QString& type,
                                     int bits, const QString& comment)
{
    QProcess process;
    QStringList args;
    args << "-t" << type.toLower()
         << "-b" << QString::number(bits)
         << "-f" << path
         << "-N" << ""
         << "-q";

    if (!comment.isEmpty()) {
        args << "-C" << comment;
    }

    process.start("ssh-keygen", args);
    process.waitForFinished(10000);

    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        Logger::instance().error("SSH", "Key generation failed: " + error);
        return false;
    }

    Logger::instance().info("SSH", "Key pair generated: " + path);
    emit keyGenerated(path);
    return true;
}

QString SSHKeyManager::fingerprint(const QString& publicKeyPath) const
{
    QProcess process;
    process.start("ssh-keygen", {"-lf", publicKeyPath});
    process.waitForFinished(3000);

    if (process.exitCode() == 0) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QStringList parts = output.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            return parts[1]; // SHA256 fingerprint
        }
    }

    return {};
}

KeyInfo SSHKeyManager::parsePublicKey(const QString& publicKeyPath) const
{
    KeyInfo info;
    QFile file(publicKeyPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return info;
    }

    QString content = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    QStringList parts = content.split(' ', Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        info.type = parts[0];
        info.publicKey = parts[1].toUtf8();

        if (parts.size() >= 3) {
            info.comment = parts[2];
        }

        // Estimate bits from base64 length
        QByteArray decoded = QByteArray::fromBase64(info.publicKey);
        info.bits = decoded.size() * 8;
    }

    return info;
}

QString SSHKeyManager::findPublicKey(const QString& privateKeyPath) const
{
    QStringList candidates = {
        privateKeyPath + ".pub",
        privateKeyPath + ".pubkey",
        privateKeyPath + ".pem.pub",
    };

    for (const auto& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}