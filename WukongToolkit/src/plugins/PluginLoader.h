#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

class PluginLoader : public QObject
{
    Q_OBJECT

public:
    explicit PluginLoader(QObject* parent = nullptr);

    bool loadPlugin(const QString& path);
    bool unloadPlugin(const QString& pluginId);
    QStringList loadedPlugins() const;
    QString pluginInfo(const QString& pluginId) const;
    bool isLoaded(const QString& pluginId) const;

signals:
    void pluginLoaded(const QString& pluginId);
    void pluginUnloaded(const QString& pluginId);
    void pluginError(const QString& error);

private:
    bool validatePlugin(const QString& path);
    QString extractPluginId(const QString& path) const;
    QString extractPluginName(const QString& path) const;
    QString extractPluginVersion(const QString& path) const;

    QMap<QString, QString> m_loadedPlugins; // id -> path
};