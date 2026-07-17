#include "installer/DeployConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

DeployConfig::DeployConfig(QObject* parent)
    : QObject(parent)
    , m_serverUrl(QString())
    , m_serverPort(8080)
    , m_updateChannel(QStringLiteral("stable"))
    , m_autoUpdate(true)
{
}

DeployConfig::~DeployConfig() = default;

DeployConfig& DeployConfig::instance()
{
    static DeployConfig s_instance;
    return s_instance;
}

bool DeployConfig::loadConfig(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("serverUrl")) {
        m_serverUrl = obj["serverUrl"].toString();
    }
    if (obj.contains("serverPort")) {
        m_serverPort = obj["serverPort"].toInt(8080);
    }
    if (obj.contains("updateChannel")) {
        m_updateChannel = obj["updateChannel"].toString(QStringLiteral("stable"));
    }
    if (obj.contains("autoUpdate")) {
        m_autoUpdate = obj["autoUpdate"].toBool(true);
    }

    return true;
}

bool DeployConfig::saveConfig()
{
    QJsonObject obj;
    obj["serverUrl"] = m_serverUrl;
    obj["serverPort"] = m_serverPort;
    obj["updateChannel"] = m_updateChannel;
    obj["autoUpdate"] = m_autoUpdate;

    QJsonDocument doc(obj);

    QFile file("deploy_config.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QString DeployConfig::serverUrl() const
{
    return m_serverUrl;
}

int DeployConfig::serverPort() const
{
    return m_serverPort;
}

QString DeployConfig::updateChannel() const
{
    return m_updateChannel;
}

bool DeployConfig::autoUpdate() const
{
    return m_autoUpdate;
}