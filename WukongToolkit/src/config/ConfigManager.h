#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QSettings>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager& instance();

    void setValue(const QString& key, const QVariant& value);
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    bool contains(const QString& key) const;

    void loadFromDb();
    void saveToDb();

    QString configPath() const;
    void setConfigPath(const QString& path);

signals:
    void configChanged(const QString& key, const QVariant& value);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    QString m_configPath;
};