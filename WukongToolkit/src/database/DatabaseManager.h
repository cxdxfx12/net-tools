#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString& dbPath);
    void close();
    bool isOpen() const;
    QSqlDatabase& database();

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createTables();

    QSqlDatabase m_db;
    bool m_initialized = false;
};