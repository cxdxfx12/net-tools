#pragma once

#include <QObject>

class EventBus;
class TaskScheduler;

class AppCore : public QObject
{
    Q_OBJECT

public:
    static AppCore& instance();

    bool initialize();
    void start();
    void shutdown();

    EventBus* eventBus() const;
    TaskScheduler* taskScheduler() const;

private:
    explicit AppCore(QObject* parent = nullptr);
    ~AppCore() override;
    AppCore(const AppCore&) = delete;
    AppCore& operator=(const AppCore&) = delete;

    EventBus* m_eventBus = nullptr;
    TaskScheduler* m_taskScheduler = nullptr;
    bool m_initialized = false;
};