#include "app/Application.h"
#include "log/Logger.h"
#include "database/DatabaseManager.h"
#include "config/ConfigManager.h"
#include "plugins/PluginManager.h"
#include "ui/ThemeManager.h"
#include "ui/MainWindow.h"

#include <QDir>
#include <QStandardPaths>

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName("WukongToolkit");
    setApplicationVersion("1.0.0");
    setOrganizationName("Wukong");
    setOrganizationDomain("wukong.toolkit");
}

Application::~Application()
{
    DatabaseManager::instance().close();
    Logger::instance().info("APP", "Application shutdown complete");
}

bool Application::initialize()
{
    // Step 1: Read config / determine paths
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    Logger::instance().setLogDir(dataDir + "/logs");
    Logger::instance().info("APP", "━━━ 悟空网络工程师工具箱 启动 ━━━");
    Logger::instance().info("APP", "Version: " + applicationVersion());
    Logger::instance().info("APP", "Data dir: " + dataDir);

    // Step 2: Initialize database
    QString dbPath = dataDir + "/config.db";
    if (!DatabaseManager::instance().initialize(dbPath)) {
        Logger::instance().error("APP", "Failed to initialize database");
        return false;
    }

    // Step 3: Initialize config
    ConfigManager::instance().setConfigPath(dbPath);
    ConfigManager::instance().loadFromDb();
    Logger::instance().info("APP", "Configuration loaded");

    // Step 4: Initialize plugin manager
    PluginManager::instance().setPluginDir(dataDir + "/plugins");
    PluginManager::instance().scanPlugins();
    Logger::instance().info("APP", "Plugin manager initialized");

    // Step 5: Initialize theme
    ThemeManager::instance().applyDarkTheme();
    Logger::instance().info("APP", "Dark theme applied");

    return true;
}

int Application::run()
{
    if (!initialize()) {
        return 1;
    }

    // Step 6: Initialize MainWindow
    MainWindow mainWindow;
    mainWindow.show();

    // Step 7: Load plugins (deferred)
    PluginManager::instance().scanPlugins();

    // Step 8: Restore layout
    mainWindow.restoreLayout();

    Logger::instance().info("APP", "MainWindow ready, entering event loop");

    // Step 9: Enter event loop
    return exec();
}