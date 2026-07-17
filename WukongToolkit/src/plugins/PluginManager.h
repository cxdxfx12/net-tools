#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include "IPlugin.h"

struct PluginInfo {
    QString name;
    QString version;
    QString author;
    QString description;
    QString entry;
    QString icon;
    QString path;
    bool enabled = true;
};

class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager& instance();

    void setPluginDir(const QString& dir);
    QString pluginDir() const;

    void scanPlugins();
    bool loadPlugin(const QString& name);
    bool unloadPlugin(const QString& name);
    bool isPluginLoaded(const QString& name) const;

    QVector<PluginInfo> availablePlugins() const;
    QVector<PluginInfo> loadedPlugins() const;

signals:
    void pluginLoaded(const QString& name);
    void pluginUnloaded(const QString& name);
    void pluginLoadFailed(const QString& name, const QString& error);

private:
    explicit PluginManager(QObject* parent = nullptr);
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    QString m_pluginDir;
};