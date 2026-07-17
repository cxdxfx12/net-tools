#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString& dbPath);
    void close();
    bool isOpen() const;

    // 安全的数据库访问：返回 const 引用，防止外部修改连接
    const QSqlDatabase& database() const;

    // 便捷查询方法
    QSqlQuery createQuery() const;

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createTables();

    QSqlDatabase m_db;
    bool m_initialized = false;
};