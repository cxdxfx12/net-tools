#include "log/Logger.h"
#include <QDir>
#include <QDebug>

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_logDir("logs")
    , m_stream(&m_logFile)
{
    QDir().mkpath(m_logDir);
    m_currentDate = QDate::currentDate();
    rotateLogFile();
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

void Logger::setLogDir(const QString& dir)
{
    m_logDir = dir;
    QDir().mkpath(m_logDir);
    rotateLogFile();
}

QString Logger::logDir() const
{
    return m_logDir;
}

void Logger::rotateLogFile()
{
    QDate today = QDate::currentDate();
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }

    QString fileName = QString("%1/%2.log")
                           .arg(m_logDir)
                           .arg(today.toString("yyyy-MM-dd"));
    m_logFile.setFileName(fileName);
    if (!m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << fileName;
    }
    m_stream.setDevice(&m_logFile);
    m_currentDate = today;
}

void Logger::writeToFile(const QString& line)
{
    QMutexLocker locker(&m_mutex);

    QDate today = QDate::currentDate();
    if (today != m_currentDate) {
        rotateLogFile();
    }

    m_stream << line << "\n";
    m_stream.flush();
}

void Logger::info(const QString& module, const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString line = QString("%1 [INFO] [%2] %3").arg(time, module, msg);

    writeToFile(line);
    qDebug().noquote() << line;
    emit logMessage(time, "INFO", module, msg);
}

void Logger::warning(const QString& module, const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString line = QString("%1 [WARNING] [%2] %3").arg(time, module, msg);

    writeToFile(line);
    qDebug().noquote() << line;
    emit logMessage(time, "WARNING", module, msg);
}

void Logger::error(const QString& module, const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString line = QString("%1 [ERROR] [%2] %3").arg(time, module, msg);

    writeToFile(line);
    qDebug().noquote() << line;
    emit logMessage(time, "ERROR", module, msg);
}

void Logger::debug(const QString& module, const QString& msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString line = QString("%1 [DEBUG] [%2] %3").arg(time, module, msg);

    writeToFile(line);
    qDebug().noquote() << line;
    emit logMessage(time, "DEBUG", module, msg);
}