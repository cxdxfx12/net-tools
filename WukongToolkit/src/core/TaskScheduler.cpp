#include "core/TaskScheduler.h"
#include "log/Logger.h"

#include <QTimer>
#include <QUuid>

TaskScheduler::TaskScheduler(QObject* parent)
    : QObject(parent)
{
    Logger::instance().info(QStringLiteral("SCHEDULER"), QStringLiteral("TaskScheduler initialized"));
}

void TaskScheduler::start()
{
    if (m_running) {
        return;
    }

    m_running = true;
    Logger::instance().info(QStringLiteral("SCHEDULER"), QStringLiteral("TaskScheduler started"));

    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        ScheduledTask* task = it.value();
        if (task->timer && !task->paused) {
            task->timer->start(task->intervalMs);
        }
    }
}

void TaskScheduler::stop()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    Logger::instance().info(QStringLiteral("SCHEDULER"), QStringLiteral("TaskScheduler stopped"));

    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        ScheduledTask* task = it.value();
        if (task->timer) {
            task->timer->stop();
        }
    }
}

bool TaskScheduler::isRunning() const
{
    return m_running;
}

QString TaskScheduler::addTask(const QString& name, int intervalMs, std::function<void()> callback)
{
    ScheduledTask* task = new ScheduledTask();
    task->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task->name = name;
    task->intervalMs = intervalMs;
    task->callback = std::move(callback);
    task->paused = false;

    task->timer = new QTimer(this);
    task->timer->setInterval(intervalMs);

    QString taskId = task->id;
    connect(task->timer, &QTimer::timeout, this, [this, taskId]() {
        executeTask(taskId);
    });

    m_tasks[task->id] = task;

    Logger::instance().info(QStringLiteral("SCHEDULER"),
        QStringLiteral("Task added: %1 (id=%2, interval=%3ms)")
            .arg(name, task->id).arg(intervalMs));

    if (m_running) {
        task->timer->start(intervalMs);
    }

    return task->id;
}

void TaskScheduler::removeTask(const QString& taskId)
{
    if (!m_tasks.contains(taskId)) {
        Logger::instance().warning(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task not found: %1").arg(taskId));
        return;
    }

    ScheduledTask* task = m_tasks[taskId];

    // 先停止 timer 并断开所有信号连接，防止 pending 事件访问已删除的 task
    if (task->timer) {
        task->timer->stop();
        task->timer->disconnect();
        task->timer->deleteLater();
        task->timer = nullptr;
    }

    // 从 map 中移除，确保 executeTask 不会再找到此 task
    QString taskName = task->name;
    m_tasks.remove(taskId);

    Logger::instance().info(QStringLiteral("SCHEDULER"),
        QStringLiteral("Task removed: %1 (id=%2)").arg(taskName, taskId));

    // 最后安全删除 task
    delete task;
}

void TaskScheduler::pauseTask(const QString& taskId)
{
    if (!m_tasks.contains(taskId)) {
        Logger::instance().warning(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task not found: %1").arg(taskId));
        return;
    }

    ScheduledTask* task = m_tasks[taskId];
    if (task->timer && !task->paused) {
        task->timer->stop();
        task->paused = true;
        Logger::instance().info(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task paused: %1 (id=%2)").arg(task->name, taskId));
    }
}

void TaskScheduler::resumeTask(const QString& taskId)
{
    if (!m_tasks.contains(taskId)) {
        Logger::instance().warning(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task not found: %1").arg(taskId));
        return;
    }

    ScheduledTask* task = m_tasks[taskId];
    if (task->timer && task->paused) {
        task->paused = false;
        if (m_running) {
            task->timer->start(task->intervalMs);
        }
        Logger::instance().info(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task resumed: %1 (id=%2)").arg(task->name, taskId));
    }
}

QStringList TaskScheduler::activeTasks() const
{
    QStringList list;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        const ScheduledTask* task = it.value();
        QString status = task->paused ? QStringLiteral(" (paused)") : QString();
        list.append(task->name + status);
    }
    return list;
}

void TaskScheduler::executeTask(const QString& taskId)
{
    if (!m_tasks.contains(taskId)) {
        return;
    }

    ScheduledTask* task = m_tasks[taskId];
    if (task->paused) {
        return;
    }

    Logger::instance().debug(QStringLiteral("SCHEDULER"),
        QStringLiteral("Executing task: %1").arg(task->name));

    try {
        if (task->callback) {
            task->callback();
        }
        emit taskExecuted(taskId, task->name);
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        Logger::instance().error(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task error: %1 - %2").arg(task->name, errorMsg));
        emit taskError(taskId, errorMsg);
    } catch (...) {
        Logger::instance().error(QStringLiteral("SCHEDULER"),
            QStringLiteral("Task error: %1 - unknown exception").arg(task->name));
        emit taskError(taskId, QStringLiteral("Unknown exception"));
    }
}