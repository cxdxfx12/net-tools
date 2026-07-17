#include "terminal/CommandHistory.h"
#include "database/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>

CommandHistory::CommandHistory(QObject* parent)
    : QObject(parent)
{
}

void CommandHistory::addCommand(const QString& command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }

    // Don't add duplicate consecutive commands
    if (!m_commands.isEmpty() && m_commands.last() == command) {
        return;
    }

    m_commands.append(command);
    if (m_commands.size() > m_maxSize) {
        m_commands.removeFirst();
    }
    m_position = m_commands.size();
    emit commandAdded(command);
}

QString CommandHistory::previous()
{
    if (m_commands.isEmpty()) {
        return {};
    }
    if (m_position > 0) {
        m_position--;
    }
    return m_commands.value(m_position);
}

QString CommandHistory::next()
{
    if (m_position < m_commands.size() - 1) {
        m_position++;
        return m_commands.value(m_position);
    }
    m_position = m_commands.size();
    return {};
}

void CommandHistory::resetPosition()
{
    m_position = m_commands.size();
}

void CommandHistory::setMaxSize(int max)
{
    m_maxSize = max;
    while (m_commands.size() > m_maxSize) {
        m_commands.removeFirst();
    }
}

int CommandHistory::maxSize() const
{
    return m_maxSize;
}

QVector<QString> CommandHistory::allCommands() const
{
    return m_commands;
}

void CommandHistory::clear()
{
    m_commands.clear();
    m_position = -1;
}

void CommandHistory::saveToDb(const QString& sessionId)
{
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    for (const auto& cmd : m_commands) {
        query.prepare("INSERT INTO history (module, command, time, result) "
                      "VALUES ('SSH', :cmd, datetime('now'), :session)");
        query.bindValue(":cmd", cmd);
        query.bindValue(":session", sessionId);
        query.exec();
    }
}

void CommandHistory::loadFromDb(const QString& sessionId)
{
    auto& db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("SELECT command FROM history WHERE module='SSH' AND result=:session "
                  "ORDER BY id DESC LIMIT :max");
    query.bindValue(":session", sessionId);
    query.bindValue(":max", m_maxSize);
    if (query.exec()) {
        while (query.next()) {
            m_commands.prepend(query.value(0).toString());
        }
    }
}