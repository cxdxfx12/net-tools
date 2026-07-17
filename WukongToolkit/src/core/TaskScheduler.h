#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QTimer>
#include <functional>

class TaskScheduler : public QObject
{
    Q_OBJECT

public:
    explicit TaskScheduler(QObject* parent = nullptr);

    void start();
    void stop();
    bool isRunning() const;

    QString addTask(const QString& name, int intervalMs, std::function<void()> callback);
    void removeTask(const QString& taskId);
    void pauseTask(const QString& taskId);
    void resumeTask(const QString& taskId);
    QStringList activeTasks() const;

signals:
    void taskExecuted(const QString& taskId, const QString& taskName);
    void taskError(const QString& taskId, const QString& error);

private slots:
    void executeTask(const QString& taskId);

private:
    struct ScheduledTask {
        QString id;
        QString name;
        int intervalMs = 0;
        std::function<void()> callback;
        QTimer* timer = nullptr;
        bool paused = false;
    };

    QMap<QString, ScheduledTask*> m_tasks;
    bool m_running = false;
};