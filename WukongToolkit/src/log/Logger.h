#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger& instance();

    void info(const QString& module, const QString& msg);
    void warning(const QString& module, const QString& msg);
    void error(const QString& module, const QString& msg);
    void debug(const QString& module, const QString& msg);

    void setLogDir(const QString& dir);
    QString logDir() const;

signals:
    void logMessage(const QString& time, const QString& level,
                    const QString& module, const QString& message);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const QString& level, const QString& module, const QString& msg);
    void writeToFile(const QString& line);
    void rotateLogFile();

    mutable QMutex m_mutex;
    QString m_logDir;
    QFile m_logFile;
    QTextStream m_stream;
    QDate m_currentDate;
};