#include "plugins/PluginManager.h"
#include "log/Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QPluginLoader>

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
    , m_pluginDir("plugins")
{
}

PluginManager& PluginManager::instance()
{
    static PluginManager s_instance;
    return s_instance;
}

void PluginManager::setPluginDir(const QString& dir)
{
    m_pluginDir = dir;
}

QString PluginManager::pluginDir() const
{
    return m_pluginDir;
}

void PluginManager::scanPlugins()
{
    QDir dir(m_pluginDir);
    if (!dir.exists()) {
        QDir().mkpath(m_pluginDir);
        Logger::instance().info("PLUGIN", "Plugin directory created: " + m_pluginDir);
        return;
    }

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& subdir : subdirs) {
        QString pluginJsonPath = dir.filePath(subdir + "/plugin.json");
        QFileInfo fi(pluginJsonPath);
        if (!fi.exists()) {
            continue;
        }

        QFile file(pluginJsonPath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();

        PluginInfo info;
        info.name = obj["name"].toString();
        info.version = obj["version"].toString();
        info.author = obj["author"].toString();
        info.description = obj["description"].toString();
        info.entry = obj["entry"].toString();
        info.icon = obj["icon"].toString();
        info.path = dir.filePath(subdir);

        Logger::instance().info("PLUGIN", "Found plugin: " + info.name + " v" + info.version);
    }
}

bool PluginManager::loadPlugin(const QString& name)
{
    Q_UNUSED(name)
    // Plugin loading will be implemented when DLL-based plugins are ready
    Logger::instance().info("PLUGIN", "Plugin loading deferred: " + name);
    return true;
}

bool PluginManager::unloadPlugin(const QString& name)
{
    Q_UNUSED(name)
    Logger::instance().info("PLUGIN", "Plugin unloaded: " + name);
    return true;
}

bool PluginManager::isPluginLoaded(const QString& name) const
{
    Q_UNUSED(name)
    return false;
}

QVector<PluginInfo> PluginManager::availablePlugins() const
{
    return {};
}

QVector<PluginInfo> PluginManager::loadedPlugins() const
{
    return {};
}