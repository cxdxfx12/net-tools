#include "config/ConfigManager.h"
#include "database/DatabaseManager.h"
#include "log/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMutexLocker>
#include <QMutex>

namespace {
    QMutex& configMutex() {
        static QMutex s_mutex;
        return s_mutex;
    }
}

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
    QMutexLocker locker(&configMutex());
    m_configPath = path;
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&configMutex());

    // 更新内存缓存
    m_cache[key] = value;

    // 持久化到数据库
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        Logger::instance().warning("CONFIG", "Database not open, config only cached in memory: " + key);
        emit configChanged(key, value);
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)");
    query.bindValue(":key", key);
    query.bindValue(":value", value.toString());
    if (!query.exec()) {
        Logger::instance().error("CONFIG",
            QString("Failed to save config key='%1': %2").arg(key, query.lastError().text()));
    }

    emit configChanged(key, value);
}

QVariant ConfigManager::value(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&configMutex());

    // 优先从内存缓存读取
    if (m_cache.contains(key)) {
        return m_cache[key];
    }

    // 回退到数据库
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        return defaultValue;
    }

    QSqlQuery query(db);
    query.prepare("SELECT value FROM settings WHERE key = :key");
    query.bindValue(":key", key);
    if (query.exec() && query.next()) {
        QVariant dbValue = query.value(0);
        // 写入缓存以提高后续读取性能
        const_cast<ConfigManager*>(this)->m_cache[key] = dbValue;
        return dbValue;
    }
    return defaultValue;
}

bool ConfigManager::contains(const QString& key) const
{
    QMutexLocker locker(&configMutex());

    // 优先检查内存缓存
    if (m_cache.contains(key)) {
        return true;
    }

    // 回退到数据库
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
    QMutexLocker locker(&configMutex());

    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        Logger::instance().warning("CONFIG", "Database not open, cannot load config");
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT key, value FROM settings")) {
        Logger::instance().error("CONFIG",
            QString("Failed to load config from database: %1").arg(query.lastError().text()));
        return;
    }

    int count = 0;
    m_cache.clear();
    while (query.next()) {
        QString key = query.value(0).toString();
        QVariant val = query.value(1);
        m_cache[key] = val;
        ++count;
    }

    Logger::instance().info("CONFIG",
        QString("Configuration loaded from database: %1 entries").arg(count));
}

void ConfigManager::saveToDb()
{
    QMutexLocker locker(&configMutex());

    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) {
        Logger::instance().warning("CONFIG", "Database not open, cannot save config");
        return;
    }

    QSqlQuery query(db);
    int count = 0;

    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)");
        query.bindValue(":key", it.key());
        query.bindValue(":value", it.value().toString());
        if (query.exec()) {
            ++count;
        } else {
            Logger::instance().error("CONFIG",
                QString("Failed to save config key='%1': %2").arg(it.key(), query.lastError().text()));
        }
    }

    Logger::instance().info("CONFIG",
        QString("Configuration saved to database: %1 entries").arg(count));
}