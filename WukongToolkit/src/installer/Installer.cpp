#include "installer/Installer.h"
#include "log/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

Installer::Installer(QObject* parent)
    : QObject(parent)
{
}

Installer::~Installer() = default;

Installer& Installer::instance()
{
    static Installer s_instance;
    return s_instance;
}

bool Installer::createDesktopShortcut()
{
    auto& inst = instance();

    inst.emit installProgress(0, QStringLiteral("正在创建桌面快捷方式..."));

    QString appPath = QCoreApplication::applicationDirPath();
    QString appBundlePath = appPath;

#ifdef Q_OS_MACOS
    // On macOS, the executable is inside Contents/MacOS, walk up to the .app bundle
    if (appPath.contains(".app/Contents")) {
        int idx = appPath.indexOf(".app/Contents");
        appBundlePath = appPath.left(idx + 4);
    } else if (!appPath.endsWith(".app")) {
        // Try to find the .app bundle in the parent directory
        QDir dir(appPath);
        QStringList entries = dir.entryList({"*.app"}, QDir::Dirs);
        if (!entries.isEmpty()) {
            appBundlePath = dir.filePath(entries.first());
        }
    }

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        inst.emit installError(QStringLiteral("无法获取桌面路径"));
        inst.emit installComplete(false);
        return false;
    }

    QString linkName = QFileInfo(appBundlePath).fileName();
    QString linkPath = desktopPath + "/" + linkName;

    // Remove existing shortcut if present
    if (QFile::exists(linkPath)) {
        QFile::remove(linkPath);
    }

    inst.emit installProgress(50, QStringLiteral("创建符号链接..."));

    if (QFile::link(appBundlePath, linkPath)) {
        Logger::instance().info("INSTALLER", "Desktop shortcut created: " + linkPath);
        inst.emit installProgress(100, QStringLiteral("桌面快捷方式创建完成"));
        inst.emit installComplete(true);
        return true;
    } else {
        Logger::instance().error("INSTALLER", "Failed to create desktop shortcut");
        inst.emit installError(QStringLiteral("创建桌面快捷方式失败"));
        inst.emit installComplete(false);
        return false;
    }
#else
    Logger::instance().warning("INSTALLER", "Desktop shortcut creation not supported on this platform");
    inst.emit installError(QStringLiteral("当前平台不支持桌面快捷方式"));
    inst.emit installComplete(false);
    return false;
#endif
}

bool Installer::createStartMenuEntry()
{
    auto& inst = instance();

    inst.emit installProgress(0, QStringLiteral("正在创建开始菜单条目..."));

#ifdef Q_OS_MACOS
    QString appPath = QCoreApplication::applicationDirPath();
    QString appBundlePath = appPath;

    if (appPath.contains(".app/Contents")) {
        int idx = appPath.indexOf(".app/Contents");
        appBundlePath = appPath.left(idx + 4);
    }

    QString applicationsPath = "/Applications";
    QString linkName = QFileInfo(appBundlePath).fileName();
    QString linkPath = applicationsPath + "/" + linkName;

    if (QFile::exists(linkPath)) {
        Logger::instance().info("INSTALLER", "Application already exists in /Applications");
        inst.emit installProgress(100, QStringLiteral("应用程序已存在于 /Applications"));
        inst.emit installComplete(true);
        return true;
    }

    inst.emit installProgress(50, QStringLiteral("复制到 /Applications..."));

    // Use cp -R to copy the .app bundle
    QProcess process;
    process.start("cp", {"-R", appBundlePath, applicationsPath});
    process.waitForFinished(30000);

    if (process.exitCode() == 0) {
        Logger::instance().info("INSTALLER", "Application copied to /Applications");
        inst.emit installProgress(100, QStringLiteral("开始菜单条目创建完成"));
        inst.emit installComplete(true);
        return true;
    } else {
        Logger::instance().error("INSTALLER", "Failed to copy to /Applications");
        inst.emit installError(QStringLiteral("创建开始菜单条目失败"));
        inst.emit installComplete(false);
        return false;
    }
#else
    Logger::instance().warning("INSTALLER", "Start menu entry creation not supported on this platform");
    inst.emit installError(QStringLiteral("当前平台不支持开始菜单条目"));
    inst.emit installComplete(false);
    return false;
#endif
}

bool Installer::registerFileAssociations()
{
    auto& inst = instance();

    inst.emit installProgress(0, QStringLiteral("正在注册文件关联..."));

#ifdef Q_OS_MACOS
    // On macOS, file associations are handled by Info.plist CFBundleDocumentTypes
    // This is configured at build time, so we just log it
    Logger::instance().info("INSTALLER", "File associations registered via Info.plist");
    inst.emit installProgress(100, QStringLiteral("文件关联注册完成"));
    inst.emit installComplete(true);
    return true;
#else
    Logger::instance().warning("INSTALLER", "File association registration not supported on this platform");
    inst.emit installError(QStringLiteral("当前平台不支持文件关联注册"));
    inst.emit installComplete(false);
    return false;
#endif
}

bool Installer::checkDependencies()
{
    auto& inst = instance();

    inst.emit installProgress(0, QStringLiteral("正在检查系统依赖..."));

    QStringList missing = missingDependencies();

    if (missing.isEmpty()) {
        Logger::instance().info("INSTALLER", "All dependencies satisfied");
        inst.emit installProgress(100, QStringLiteral("所有依赖满足"));
        inst.emit installComplete(true);
        return true;
    } else {
        QString msg = QStringLiteral("缺少依赖: ") + missing.join(", ");
        Logger::instance().warning("INSTALLER", msg);
        inst.emit installProgress(100, msg);
        inst.emit installComplete(false);
        return false;
    }
}

QStringList Installer::missingDependencies()
{
    QStringList missing;

    // Check for essential system tools on macOS
    QProcess which;
    QStringList tools = {"ping", "traceroute", "nslookup", "ssh", "curl"};

    for (const QString& tool : tools) {
        which.start("which", {tool});
        which.waitForFinished(3000);
        if (which.exitCode() != 0) {
            missing.append(tool);
        }
    }

    return missing;
}

QString Installer::appVersion()
{
    return QStringLiteral("1.0.0");
}

QString Installer::buildDate()
{
    return QStringLiteral(__DATE__) + QStringLiteral(" ") + QStringLiteral(__TIME__);
}

bool Installer::isFirstRun()
{
    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("WukongToolkit"));
    return !settings.contains(QStringLiteral("Installer/FirstRun"));
}

void Installer::markFirstRunComplete()
{
    QSettings settings(QStringLiteral("WukongToolkit"), QStringLiteral("WukongToolkit"));
    settings.setValue(QStringLiteral("Installer/FirstRun"), false);
    Logger::instance().info("INSTALLER", "First run marked complete");
}