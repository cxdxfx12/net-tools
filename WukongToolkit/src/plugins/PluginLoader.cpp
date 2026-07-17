#include "plugins/PluginLoader.h"
#include "log/Logger.h"

#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QLibrary>

// ═══════════════════════════════════════════════════════════════════════════════
// PluginLoader
// ═══════════════════════════════════════════════════════════════════════════════

PluginLoader::PluginLoader(QObject* parent)
    : QObject(parent)
{
}

// ─── Public API ──────────────────────────────────────────────────────────────

bool PluginLoader::loadPlugin(const QString& path)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        QString err = QStringLiteral("Plugin file not found: %1").arg(path);
        Logger::instance().error(QStringLiteral("PLUGIN_LOADER"), err);
        emit pluginError(err);
        return false;
    }

    if (!validatePlugin(path)) {
        QString err = QStringLiteral("Plugin validation failed: %1").arg(path);
        Logger::instance().error(QStringLiteral("PLUGIN_LOADER"), err);
        emit pluginError(err);
        return false;
    }

    QString pluginId = extractPluginId(path);

    if (m_loadedPlugins.contains(pluginId)) {
        QString err = QStringLiteral("Plugin already loaded: %1").arg(pluginId);
        Logger::instance().warning(QStringLiteral("PLUGIN_LOADER"), err);
        emit pluginError(err);
        return false;
    }

    // Try loading as a Qt plugin via QPluginLoader
    QPluginLoader loader(path);
    QObject* instance = loader.instance();
    if (instance) {
        m_loadedPlugins.insert(pluginId, path);
        Logger::instance().info(QStringLiteral("PLUGIN_LOADER"),
            QStringLiteral("Plugin loaded (Qt plugin): %1").arg(pluginId));
        emit pluginLoaded(pluginId);
        return true;
    }

    // If QPluginLoader fails, try as a plain shared library
    QString errorString = loader.errorString();
    Logger::instance().warning(QStringLiteral("PLUGIN_LOADER"),
        QStringLiteral("QPluginLoader failed for %1: %2, trying QLibrary...")
            .arg(pluginId, errorString));

    QLibrary* lib = new QLibrary(path, this);
    if (lib->load()) {
        m_loadedPlugins.insert(pluginId, path);
        Logger::instance().info(QStringLiteral("PLUGIN_LOADER"),
            QStringLiteral("Plugin loaded (shared library): %1").arg(pluginId));
        emit pluginLoaded(pluginId);
        return true;
    }

    QString libError = lib->errorString();
    delete lib;

    QString err = QStringLiteral("Failed to load plugin %1: %2").arg(pluginId, libError);
    Logger::instance().error(QStringLiteral("PLUGIN_LOADER"), err);
    emit pluginError(err);
    return false;
}

bool PluginLoader::unloadPlugin(const QString& pluginId)
{
    if (!m_loadedPlugins.contains(pluginId)) {
        QString err = QStringLiteral("Plugin not loaded: %1").arg(pluginId);
        Logger::instance().warning(QStringLiteral("PLUGIN_LOADER"), err);
        return false;
    }

    QString path = m_loadedPlugins.value(pluginId);

    // Try QPluginLoader unload
    {
        QPluginLoader loader(path);
        if (loader.isLoaded()) {
            if (!loader.unload()) {
                Logger::instance().warning(QStringLiteral("PLUGIN_LOADER"),
                    QStringLiteral("QPluginLoader unload warning for %1: %2")
                        .arg(pluginId, loader.errorString()));
            }
        }
    }

    m_loadedPlugins.remove(pluginId);
    Logger::instance().info(QStringLiteral("PLUGIN_LOADER"),
        QStringLiteral("Plugin unloaded: %1").arg(pluginId));
    emit pluginUnloaded(pluginId);
    return true;
}

QStringList PluginLoader::loadedPlugins() const
{
    return m_loadedPlugins.keys();
}

QString PluginLoader::pluginInfo(const QString& pluginId) const
{
    if (!m_loadedPlugins.contains(pluginId)) {
        return QStringLiteral("Plugin not loaded: %1").arg(pluginId);
    }

    QString path = m_loadedPlugins.value(pluginId);
    QString name = extractPluginName(path);
    QString version = extractPluginVersion(path);

    return QStringLiteral("ID: %1\nName: %2\nVersion: %3\nPath: %4")
        .arg(pluginId, name, version, path);
}

bool PluginLoader::isLoaded(const QString& pluginId) const
{
    return m_loadedPlugins.contains(pluginId);
}

// ─── Private Methods ─────────────────────────────────────────────────────────

bool PluginLoader::validatePlugin(const QString& path)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        return false;
    }

    if (!fi.isFile()) {
        return false;
    }

    // Check if it's a valid dynamic library by extension
    QString suffix = fi.suffix().toLower();
    if (suffix != QStringLiteral("dylib")
        && suffix != QStringLiteral("so")
        && suffix != QStringLiteral("dll")) {
        Logger::instance().warning(QStringLiteral("PLUGIN_LOADER"),
            QStringLiteral("Unexpected plugin extension: %1").arg(suffix));
    }

    // Try to read metadata from the plugin file
    // For Qt plugins, metadata is embedded; we check if it's loadable
    QPluginLoader loader(path);
    QJsonObject meta = loader.metaData();
    if (!meta.isEmpty()) {
        Logger::instance().debug(QStringLiteral("PLUGIN_LOADER"),
            QStringLiteral("Plugin metadata found for: %1").arg(path));
        return true;
    }

    // For non-Qt plugins, just verify it's a valid library
    if (!QLibrary::isLibrary(path)) {
        Logger::instance().error(QStringLiteral("PLUGIN_LOADER"),
            QStringLiteral("Not a valid dynamic library: %1").arg(path));
        return false;
    }

    return true;
}

QString PluginLoader::extractPluginId(const QString& path) const
{
    QFileInfo fi(path);
    QString baseName = fi.completeBaseName();

    // Remove "lib" prefix on Unix
    if (baseName.startsWith(QStringLiteral("lib"))) {
        baseName = baseName.mid(3);
    }

    return baseName;
}

QString PluginLoader::extractPluginName(const QString& path) const
{
    QFileInfo fi(path);

    // Try to read metadata from Qt plugin
    QPluginLoader loader(path);
    QJsonObject meta = loader.metaData();
    if (!meta.isEmpty()) {
        QJsonObject metaObj = meta.value(QStringLiteral("MetaData")).toObject();
        QString name = metaObj.value(QStringLiteral("name")).toString();
        if (!name.isEmpty()) {
            return name;
        }
    }

    // Fallback: use filename
    return fi.completeBaseName();
}

QString PluginLoader::extractPluginVersion(const QString& path) const
{
    // Try to read metadata from Qt plugin
    QPluginLoader loader(path);
    QJsonObject meta = loader.metaData();
    if (!meta.isEmpty()) {
        QJsonObject metaObj = meta.value(QStringLiteral("MetaData")).toObject();
        QString version = metaObj.value(QStringLiteral("version")).toString();
        if (!version.isEmpty()) {
            return version;
        }
    }

    return QStringLiteral("unknown");
}