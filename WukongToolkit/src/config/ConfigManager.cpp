#include "config/ConfigManager.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"
#include <QSqlQuery>
#include <QSqlError>

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_configPath("config.db")
{
}

ConfigManager& ConfigManager::instance()
{
    static ConfigManager s_instance;
    return s_instance;
}

QString ConfigManager::configPath() const
{
    return m_configPath;
}

void ConfigManager::setConfigPath(const QString& path)
{
    m_configPath = path;
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)");
    query.bindValue(":key", key);
    query.bindValue(":value", value.toString());
    if (!query.exec()) {
        Logger::instance().error("CONFIG", "Failed to save config: " + key);
    }

    emit configChanged(key, value);
}

QVariant ConfigManager::value(const QString& key, const QVariant& defaultValue) const
{
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return defaultValue;
    }

    QSqlQuery query(db);
    query.prepare("SELECT value FROM settings WHERE key = :key");
    query.bindValue(":key", key);
    if (query.exec() && query.next()) {
        return query.value(0);
    }
    return defaultValue;
}

bool ConfigManager::contains(const QString& key) const
{
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM settings WHERE key = :key");
    query.bindValue(":key", key);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

void ConfigManager::loadFromDb()
{
    Logger::instance().info("CONFIG", "Configuration loaded from database");
}

void ConfigManager::saveToDb()
{
    Logger::instance().info("CONFIG", "Configuration saved to database");
}