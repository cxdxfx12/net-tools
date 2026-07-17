#pragma once

#include <QObject>
#include <QVector>
#include <QString>

class CommandHistory : public QObject
{
    Q_OBJECT

public:
    explicit CommandHistory(QObject* parent = nullptr);

    void addCommand(const QString& command);
    QString previous();
    QString next();
    void resetPosition();

    void setMaxSize(int max);
    int maxSize() const;

    QVector<QString> allCommands() const;
    void clear();

    void saveToDb(const QString& sessionId);
    void loadFromDb(const QString& sessionId);

signals:
    void commandAdded(const QString& command);

private:
    QVector<QString> m_commands;
    int m_position = -1;
    int m_maxSize = 1000;
};