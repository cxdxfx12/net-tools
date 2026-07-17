#include "ui/StatusBarManager.h"
#include <QDateTime>
#include <QThread>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#endif

StatusBarManager::StatusBarManager(QStatusBar* statusBar, QObject* parent)
    : QObject(parent)
    , m_statusBar(statusBar)
    , m_version("v1.0.0")
{
    m_cpuLabel = new QLabel("CPU --%");
    m_memoryLabel = new QLabel("RAM --MB");
    m_threadLabel = new QLabel("Thread 0");
    m_pluginLabel = new QLabel("Plugin 0");
    m_versionLabel = new QLabel(m_version);
    m_timeLabel = new QLabel();

    m_cpuLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");
    m_memoryLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");
    m_threadLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");
    m_pluginLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");
    m_versionLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");
    m_timeLabel->setStyleSheet("padding: 0 8px; color: #8C8C8C;");

    m_statusBar->addPermanentWidget(m_cpuLabel);
    m_statusBar->addPermanentWidget(m_memoryLabel);
    m_statusBar->addPermanentWidget(m_threadLabel);
    m_statusBar->addPermanentWidget(m_pluginLabel);
    m_statusBar->addPermanentWidget(m_versionLabel);
    m_statusBar->addPermanentWidget(m_timeLabel);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &StatusBarManager::refresh);
}

void StatusBarManager::startMonitoring()
{
    m_timer->start(1000);
    refresh();
}

void StatusBarManager::stopMonitoring()
{
    m_timer->stop();
}

void StatusBarManager::setPluginCount(int count)
{
    m_pluginCount = count;
    m_pluginLabel->setText(QString("Plugin %1").arg(count));
}

void StatusBarManager::setVersion(const QString& version)
{
    m_version = version;
    m_versionLabel->setText(version);
}

void StatusBarManager::refresh()
{
    // Thread count
    int threadCount = QThread::idealThreadCount();
    m_threadLabel->setText(QString("Thread %1").arg(threadCount));

    // Time
    m_timeLabel->setText(QDateTime::currentDateTime().toString("HH:mm"));

    // Plugin count
    m_pluginLabel->setText(QString("Plugin %1").arg(m_pluginCount));

#ifdef Q_OS_MACOS
    // Memory usage via mach
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS) {
        double memMB = info.resident_size / (1024.0 * 1024.0);
        m_memoryLabel->setText(QString("RAM %1MB").arg(static_cast<int>(memMB)));
    }
#else
    m_memoryLabel->setText("RAM --MB");
#endif

    m_cpuLabel->setText("CPU --%");
}