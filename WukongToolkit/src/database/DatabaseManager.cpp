#include "database/DatabaseManager.h"
#include "log/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfo>

namespace {
    const char* DB_CONNECTION_NAME = "wukong_main";
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    if (m_initialized) {
        return true;
    }

    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());

    // 使用固定连接名避免重复 addDatabase 时的警告
    if (QSqlDatabase::contains(DB_CONNECTION_NAME)) {
        m_db = QSqlDatabase::database(DB_CONNECTION_NAME);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION_NAME);
    }
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        Logger::instance().error("DB", "Failed to open database: " + m_db.lastError().text());
        return false;
    }

    if (!createTables()) {
        Logger::instance().error("DB", "Failed to create tables");
        return false;
    }

    m_initialized = true;
    Logger::instance().info("DB", "Database initialized: " + dbPath);
    return true;
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_initialized = false;
}

bool DatabaseManager::isOpen() const
{
    return m_initialized && m_db.isOpen();
}

const QSqlDatabase& DatabaseManager::database() const
{
    return m_db;
}

QSqlQuery DatabaseManager::createQuery() const
{
    return QSqlQuery(m_db);
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // Settings table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS settings ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "key TEXT UNIQUE NOT NULL,"
            "value TEXT"
            ")")) {
        Logger::instance().error("DB", "Create settings table failed: " + query.lastError().text());
        return false;
    }

    // Device table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS device ("
            "id TEXT PRIMARY KEY,"
            "name TEXT,"
            "host TEXT,"
            "port INTEGER DEFAULT 22,"
            "username TEXT,"
            "password TEXT,"
            "vendor INTEGER DEFAULT 0,"
            "model TEXT,"
            "version TEXT,"
            "protocol TEXT DEFAULT 'SSH',"
            "group_id TEXT,"
            "remark TEXT,"
            "favorite INTEGER DEFAULT 0,"
            "create_time TEXT,"
            "update_time TEXT"
            ")")) {
        Logger::instance().error("DB", "Create device table failed: " + query.lastError().text());
        return false;
    }

    // Device group table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS device_group ("
            "id TEXT PRIMARY KEY,"
            "parent_id TEXT,"
            "name TEXT,"
            "sort INTEGER DEFAULT 0"
            ")")) {
        Logger::instance().error("DB", "Create device_group table failed: " + query.lastError().text());
        return false;
    }

    // History table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "module TEXT,"
            "command TEXT,"
            "time TEXT,"
            "result TEXT"
            ")")) {
        Logger::instance().error("DB", "Create history table failed: " + query.lastError().text());
        return false;
    }

    // Plugin table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS plugin ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT UNIQUE,"
            "version TEXT,"
            "enable INTEGER DEFAULT 1,"
            "path TEXT"
            ")")) {
        Logger::instance().error("DB", "Create plugin table failed: " + query.lastError().text());
        return false;
    }

    // Log table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS log ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "time TEXT,"
            "level TEXT,"
            "module TEXT,"
            "message TEXT"
            ")")) {
        Logger::instance().error("DB", "Create log table failed: " + query.lastError().text());
        return false;
    }

    // Ping history table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS ping_history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "host TEXT,"
            "time TEXT,"
            "average REAL,"
            "loss REAL,"
            "max REAL,"
            "min REAL"
            ")")) {
        Logger::instance().error("DB", "Create ping_history table failed: " + query.lastError().text());
        return false;
    }

    // Traceroute history table
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS traceroute_history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "target TEXT,"
            "start_time TEXT,"
            "finish_time TEXT,"
            "hop_count INTEGER,"
            "average_rtt REAL"
            ")")) {
        Logger::instance().error("DB", "Create traceroute_history table failed: " + query.lastError().text());
        return false;
    }

    Logger::instance().info("DB", "All tables created successfully");
    return true;
}