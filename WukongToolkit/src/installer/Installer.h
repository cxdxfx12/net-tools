#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class Installer : public QObject
{
    Q_OBJECT

public:
    static Installer& instance();

    static bool createDesktopShortcut();
    static bool createStartMenuEntry();
    static bool registerFileAssociations();
    static bool checkDependencies();
    static QStringList missingDependencies();
    static QString appVersion();
    static QString buildDate();
    static bool isFirstRun();
    static void markFirstRunComplete();

signals:
    void installProgress(int percent, const QString& message);
    void installComplete(bool success);
    void installError(const QString& error);

private:
    explicit Installer(QObject* parent = nullptr);
    ~Installer() override;
    Installer(const Installer&) = delete;
    Installer& operator=(const Installer&) = delete;
};