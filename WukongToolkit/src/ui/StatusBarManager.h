#pragma once

#include <QObject>
#include <QStatusBar>
#include <QLabel>
#include <QWidget>
#include <QTimer>

class StatusBarManager : public QObject
{
    Q_OBJECT

public:
    explicit StatusBarManager(QStatusBar* statusBar, QObject* parent = nullptr);

    void startMonitoring();
    void stopMonitoring();

    void setPluginCount(int count);
    void setVersion(const QString& version);

private slots:
    void refresh();

private:
    QStatusBar* m_statusBar;
    QTimer* m_timer;

    // Capsule widgets
    QWidget* m_cpuCapsule;
    QWidget* m_memCapsule;
    QWidget* m_threadCapsule;
    QWidget* m_pluginCapsule;
    QWidget* m_versionCapsule;
    QWidget* m_timeCapsule;

    // Value labels
    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    QLabel* m_threadLabel;
    QLabel* m_pluginLabel;
    QLabel* m_versionLabel;
    QLabel* m_timeLabel;

    int m_pluginCount = 0;
    QString m_version;
};
