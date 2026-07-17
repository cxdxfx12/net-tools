#pragma once

#include <QObject>
#include <QString>

class DeployConfig : public QObject
{
    Q_OBJECT

public:
    static DeployConfig& instance();

    bool loadConfig(const QString& path);
    bool saveConfig();

    QString serverUrl() const;
    int serverPort() const;
    QString updateChannel() const;
    bool autoUpdate() const;

private:
    explicit DeployConfig(QObject* parent = nullptr);
    ~DeployConfig() override;
    DeployConfig(const DeployConfig&) = delete;
    DeployConfig& operator=(const DeployConfig&) = delete;

    QString m_serverUrl;
    int m_serverPort;
    QString m_updateChannel;
    bool m_autoUpdate;
};