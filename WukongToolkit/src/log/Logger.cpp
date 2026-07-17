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
    QMutexLocker locker(&m_mutex);
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
    QMutexLocker locker(&m_mutex);
    m_logDir = dir;
    QDir().mkpath(m_logDir);
    rotateLogFile();
}

QString Logger::logDir() const
{
    QMutexLocker locker(&m_mutex);
    return m_logDir;
}

void Logger::rotateLogFile()
{
    // 注意：调用者必须已持有 m_mutex
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

// 公共日志方法，避免代码重复
void Logger::log(const QString& level, const QString& module, const QString& msg)
{
    // 使用 24 小时制避免上午/下午混淆
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString line = QString("%1 [%2] [%3] %4").arg(time, level, module, msg);

    writeToFile(line);
    qDebug().noquote() << line;
    emit logMessage(time, level, module, msg);
}

void Logger::info(const QString& module, const QString& msg)
{
    log(QStringLiteral("INFO"), module, msg);
}

void Logger::warning(const QString& module, const QString& msg)
{
    log(QStringLiteral("WARNING"), module, msg);
}

void Logger::error(const QString& module, const QString& msg)
{
    log(QStringLiteral("ERROR"), module, msg);
}

void Logger::debug(const QString& module, const QString& msg)
{
    log(QStringLiteral("DEBUG"), module, msg);
}