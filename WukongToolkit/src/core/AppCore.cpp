#include "core/AppCore.h"
#include "core/EventBus.h"
#include "core/TaskScheduler.h"
#include "log/Logger.h"

AppCore::AppCore(QObject* parent)
    : QObject(parent)
{
}

AppCore::~AppCore()
{
    shutdown();
}

AppCore& AppCore::instance()
{
    static AppCore s_instance;
    return s_instance;
}

bool AppCore::initialize()
{
    if (m_initialized) {
        Logger::instance().warning(QStringLiteral("CORE"), QStringLiteral("AppCore already initialized"));
        return true;
    }

    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("Initializing System Core..."));

    m_eventBus = new EventBus(this);
    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("EventBus created"));

    m_taskScheduler = new TaskScheduler(this);
    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("TaskScheduler created"));

    m_initialized = true;
    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("System Core initialized successfully"));
    return true;
}

void AppCore::start()
{
    if (!m_initialized) {
        Logger::instance().error(QStringLiteral("CORE"), QStringLiteral("Cannot start: AppCore not initialized"));
        return;
    }

    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("Starting System Core services..."));

    if (m_taskScheduler) {
        m_taskScheduler->start();
        Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("TaskScheduler started"));
    }

    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("System Core services started"));
}

void AppCore::shutdown()
{
    if (!m_initialized) {
        return;
    }

    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("Shutting down System Core..."));

    if (m_taskScheduler) {
        m_taskScheduler->stop();
        Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("TaskScheduler stopped"));
    }

    m_initialized = false;
    Logger::instance().info(QStringLiteral("CORE"), QStringLiteral("System Core shutdown complete"));
}

EventBus* AppCore::eventBus() const
{
    return m_eventBus;
}

TaskScheduler* AppCore::taskScheduler() const
{
    return m_taskScheduler;
}