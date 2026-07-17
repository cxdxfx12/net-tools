#include "ui/StatusBarManager.h"
#include "ui/ThemeManager.h"
#include <QDateTime>
#include <QThread>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

StatusBarManager::StatusBarManager(QStatusBar* statusBar, QObject* parent)
    : QObject(parent)
    , m_statusBar(statusBar)
    , m_version("v1.0.0")
{
    // ── Create capsule-style indicators ──
    auto makeCapsule = [](const QString& icon, const QString& label, QLabel*& valueLabel) -> QWidget* {
        auto* capsule = new QWidget();
        capsule->setStyleSheet(
            "QWidget {"
            "  background-color: rgba(255,255,255,0.04);"
            "  border: 1px solid rgba(255,255,255,0.06);"
            "  border-radius: 6px;"
            "  padding: 0px 0px;"
            "}"
        );
        auto* layout = new QHBoxLayout(capsule);
        layout->setContentsMargins(8, 2, 8, 2);
        layout->setSpacing(6);

        if (!icon.isEmpty()) {
            auto* iconLbl = new QLabel(icon);
            iconLbl->setStyleSheet("font-size: 10px; background: transparent; border: none; color: #8B949E;");
            layout->addWidget(iconLbl);
        }
        if (!label.isEmpty()) {
            auto* lbl = new QLabel(label);
            lbl->setStyleSheet("font-size: 10px; background: transparent; border: none; color: #484F58;");
            layout->addWidget(lbl);
        }
        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #8B949E;");
        layout->addWidget(valueLabel);

        return capsule;
    };

    m_cpuCapsule = makeCapsule(QString::fromUtf8("\xE2\x9A\xA1"), "CPU", m_cpuLabel);
    m_memCapsule = makeCapsule(QString::fromUtf8("\xF0\x9F\x92\xBE"), "RAM", m_memoryLabel);
    m_threadCapsule = makeCapsule(QString::fromUtf8("\xE2\x9F\xBF"), "", m_threadLabel);
    m_pluginCapsule = makeCapsule(QString::fromUtf8("\xE2\x8B\x97"), "", m_pluginLabel);
    m_versionCapsule = makeCapsule("", "", m_versionLabel);
    m_timeCapsule = makeCapsule(QString::fromUtf8("\xF0\x9F\x95\x90"), "", m_timeLabel);

    m_versionLabel->setText(m_version);

    // ── Company link (left side) ──
    auto* companyLabel = new QLabel();
    companyLabel->setText(QString::fromUtf8(
        "<a href=\"https://www.hbdxm.com\" style=\"color: %1; text-decoration: none; font-size: 10px;\">"
        "杭州喵喵至家网络有限公司"
        "</a>"
    ).arg(ThemeManager::textSecondary().name()));
    companyLabel->setStyleSheet("background: transparent; border: none; padding: 0px 8px;");
    companyLabel->setOpenExternalLinks(false);
    connect(companyLabel, &QLabel::linkActivated, this, [](const QString& url) {
        QDesktopServices::openUrl(QUrl(url));
    });

    m_statusBar->addWidget(companyLabel);

    m_statusBar->addPermanentWidget(m_cpuCapsule);
    m_statusBar->addPermanentWidget(m_memCapsule);
    m_statusBar->addPermanentWidget(m_threadCapsule);
    m_statusBar->addPermanentWidget(m_pluginCapsule);
    m_statusBar->addPermanentWidget(m_versionCapsule);
    m_statusBar->addPermanentWidget(m_timeCapsule);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &StatusBarManager::refresh);
}

void StatusBarManager::startMonitoring()
{
    m_timer->start(2000);
    refresh();
}

void StatusBarManager::stopMonitoring()
{
    m_timer->stop();
}

void StatusBarManager::setPluginCount(int count)
{
    m_pluginCount = count;
    m_pluginLabel->setText(QString("Plugins %1").arg(count));
}

void StatusBarManager::setVersion(const QString& version)
{
    m_version = version;
    m_versionLabel->setText(version);
}

void StatusBarManager::refresh()
{
    int threadCount = QThread::idealThreadCount();
    m_threadLabel->setText(QString("%1 threads").arg(threadCount));
    m_timeLabel->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
    m_pluginLabel->setText(QString("Plugins %1").arg(m_pluginCount));

#ifdef Q_OS_MACOS
    // Memory
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS) {
        double memMB = info.resident_size / (1024.0 * 1024.0);
        m_memoryLabel->setText(QString("%1 MB").arg(static_cast<int>(memMB)));

        // Color-code memory usage
        if (memMB > 200) {
            m_memoryLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #F85149;");
        } else if (memMB > 100) {
            m_memoryLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #D29922;");
        } else {
            m_memoryLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #8B949E;");
        }
    }

    // CPU usage via host_processor_info
    static uint64_t prevTotal = 0;
    static uint64_t prevIdle = 0;

    host_cpu_load_info_data_t cpuInfo;
    mach_msg_type_number_t cpuCount = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                        (host_info_t)&cpuInfo, &cpuCount) == KERN_SUCCESS) {
        uint64_t user = cpuInfo.cpu_ticks[CPU_STATE_USER];
        uint64_t system = cpuInfo.cpu_ticks[CPU_STATE_SYSTEM];
        uint64_t idle = cpuInfo.cpu_ticks[CPU_STATE_IDLE];
        uint64_t nice = cpuInfo.cpu_ticks[CPU_STATE_NICE];
        uint64_t total = user + system + idle + nice;

        if (prevTotal > 0) {
            uint64_t totalDelta = total - prevTotal;
            uint64_t idleDelta = idle - prevIdle;
            if (totalDelta > 0) {
                double cpuPct = 100.0 * (1.0 - (double)idleDelta / totalDelta);
                m_cpuLabel->setText(QString("%1%").arg(static_cast<int>(cpuPct)));

                if (cpuPct > 50) {
                    m_cpuLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #F85149;");
                } else if (cpuPct > 25) {
                    m_cpuLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #D29922;");
                } else {
                    m_cpuLabel->setStyleSheet("font-size: 10px; font-weight: 600; background: transparent; border: none; color: #8B949E;");
                }
            }
        }
        prevTotal = total;
        prevIdle = idle;
    }
#else
    m_memoryLabel->setText("-- MB");
    m_cpuLabel->setText("--%");
#endif
}
