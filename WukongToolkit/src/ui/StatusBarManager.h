#pragma once

#include <QObject>
#include <QStatusBar>
#include <QLabel>
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
    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    QLabel* m_threadLabel;
    QLabel* m_pluginLabel;
    QLabel* m_versionLabel;
    QLabel* m_timeLabel;
    QTimer* m_timer;

    int m_pluginCount = 0;
    QString m_version;
};