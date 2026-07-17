#include "plugins/PluginWidget.h"
#include "plugins/PluginLoader.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QSplitter>
#include <QFrame>

// ═══════════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════════

static const char* kDarkBg     = "#0D1117";
static const char* kDarkPanel  = "#161B22";
static const char* kDarkBorder = "#30363D";
static const char* kTextColor  = "#E6EDF3";
static const char* kAccent     = "#4A90D9";
static const char* kGreen      = "#5CB85C";
static const char* kRed        = "#D9534F";

static QString styleBtn(const char* color = kAccent)
{
    return QStringLiteral(
        "QPushButton {"
        "  background: %1; color: #FFF;"
        "  border: none; padding: 6px 16px;"
        "  border-radius: 3px; font-size: 13px;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled { background: #555; color: #888; }"
    ).arg(QString::fromLatin1(color),
          QString::fromLatin1(color));
}

static QString styleTable()
{
    return QStringLiteral(
        "QTableWidget {"
        "  background: %1; color: %2;"
        "  gridline-color: %3; border: 1px solid %3;"
        "  font-size: 13px;"
        "  selection-background-color: #3A6EA5;"
        "}"
        "QTableWidget::item { padding: 4px 8px; }"
        "QHeaderView::section {"
        "  background: %4; color: %2;"
        "  border: none; border-bottom: 1px solid %3;"
        "  padding: 6px 8px; font-weight: bold;"
        "}"
    ).arg(QString::fromLatin1(kDarkPanel),
          QString::fromLatin1(kTextColor),
          QString::fromLatin1(kDarkBorder),
          QString::fromLatin1(kDarkBg));
}

static QString styleLabel(const char* color = kTextColor)
{
    return QStringLiteral(
        "QLabel { color: %1; font-size: 13px; }"
    ).arg(QString::fromLatin1(color));
}

static QString styleGroupBox()
{
    return QStringLiteral(
        "QGroupBox {"
        "  color: %1; font-size: 13px; font-weight: bold;"
        "  border: 1px solid %2; border-radius: 4px;"
        "  margin-top: 12px; padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px; padding: 0 6px;"
        "}"
    ).arg(QString::fromLatin1(kTextColor),
          QString::fromLatin1(kDarkBorder));
}

static QString stylePlainTextEdit()
{
    return QStringLiteral(
        "QPlainTextEdit {"
        "  background: %1; color: %2;"
        "  border: 1px solid %3; border-radius: 3px;"
        "  font-size: 13px; padding: 6px;"
        "}"
    ).arg(QString::fromLatin1(kDarkPanel),
          QString::fromLatin1(kTextColor),
          QString::fromLatin1(kDarkBorder));
}

// ═══════════════════════════════════════════════════════════════════════════════
// PluginWidget
// ═══════════════════════════════════════════════════════════════════════════════

PluginWidget::PluginWidget(QWidget* parent)
    : QWidget(parent)
    , m_pluginTable(nullptr)
    , m_loadBtn(nullptr)
    , m_unloadBtn(nullptr)
    , m_toggleBtn(nullptr)
    , m_installBtn(nullptr)
    , m_pluginDirLabel(nullptr)
    , m_refreshBtn(nullptr)
    , m_pluginDetail(nullptr)
    , m_loader(nullptr)
{
    m_loader = new PluginLoader(this);

    setupUI();
    setupConnections();
    scanPluginDirectory();
}

PluginWidget::~PluginWidget()
{
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void PluginWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ── Top Toolbar ──
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    m_pluginDirLabel = new QLabel(QStringLiteral("插件目录: plugins/"));
    m_pluginDirLabel->setStyleSheet(styleLabel(kAccent));

    m_installBtn = new QPushButton(QStringLiteral("安装插件"));
    m_installBtn->setStyleSheet(styleBtn(kGreen));

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    m_refreshBtn->setStyleSheet(styleBtn());

    toolbarLayout->addWidget(m_pluginDirLabel);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_installBtn);
    toolbarLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(toolbarLayout);

    // ── Middle: Table + Detail (splitter) ──
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(2);
    splitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background: %1; }")
            .arg(QString::fromLatin1(kDarkBorder)));

    // Left: plugin table
    auto* tableGroup = new QGroupBox(QStringLiteral("已安装插件"));
    tableGroup->setStyleSheet(styleGroupBox());
    auto* tableLayout = new QVBoxLayout(tableGroup);

    m_pluginTable = new QTableWidget();
    m_pluginTable->setColumnCount(5);
    m_pluginTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("版本"),
        QStringLiteral("作者"),
        QStringLiteral("状态"),
        QStringLiteral("路径")
    });
    m_pluginTable->setStyleSheet(styleTable());
    m_pluginTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTable->setAlternatingRowColors(true);
    m_pluginTable->horizontalHeader()->setStretchLastSection(true);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_pluginTable->verticalHeader()->setVisible(false);
    m_pluginTable->setMinimumWidth(500);

    tableLayout->addWidget(m_pluginTable);

    // Action buttons for table
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    m_loadBtn = new QPushButton(QStringLiteral("加载"));
    m_loadBtn->setStyleSheet(styleBtn(kGreen));

    m_unloadBtn = new QPushButton(QStringLiteral("卸载"));
    m_unloadBtn->setStyleSheet(styleBtn(kRed));

    m_toggleBtn = new QPushButton(QStringLiteral("启用/禁用"));
    m_toggleBtn->setStyleSheet(styleBtn());

    actionLayout->addWidget(m_loadBtn);
    actionLayout->addWidget(m_unloadBtn);
    actionLayout->addWidget(m_toggleBtn);
    actionLayout->addStretch();

    tableLayout->addLayout(actionLayout);

    splitter->addWidget(tableGroup);

    // Right: detail panel
    auto* detailGroup = new QGroupBox(QStringLiteral("插件详情"));
    detailGroup->setStyleSheet(styleGroupBox());
    auto* detailLayout = new QVBoxLayout(detailGroup);

    m_pluginDetail = new QPlainTextEdit();
    m_pluginDetail->setReadOnly(true);
    m_pluginDetail->setStyleSheet(stylePlainTextEdit());
    m_pluginDetail->setPlaceholderText(QStringLiteral("请选择一个插件查看详细信息..."));
    m_pluginDetail->setMinimumWidth(250);

    detailLayout->addWidget(m_pluginDetail);

    splitter->addWidget(detailGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);

    setStyleSheet(
        QStringLiteral("PluginWidget { background: %1; }")
            .arg(QString::fromLatin1(kDarkBg)));
}

// ─── Connections ─────────────────────────────────────────────────────────────

void PluginWidget::setupConnections()
{
    connect(m_loadBtn, &QPushButton::clicked, this, &PluginWidget::onLoadPlugin);
    connect(m_unloadBtn, &QPushButton::clicked, this, &PluginWidget::onUnloadPlugin);
    connect(m_toggleBtn, &QPushButton::clicked, this, &PluginWidget::onTogglePlugin);
    connect(m_installBtn, &QPushButton::clicked, this, &PluginWidget::onInstallPlugin);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PluginWidget::onRefresh);
    connect(m_pluginTable, &QTableWidget::itemSelectionChanged,
            this, &PluginWidget::onPluginSelected);

    connect(m_loader, &PluginLoader::pluginLoaded, this, [this](const QString& pluginId) {
        Q_UNUSED(pluginId)
        updatePluginTable();
        Logger::instance().info(QStringLiteral("PLUGIN_UI"),
            QStringLiteral("Plugin loaded: %1").arg(pluginId));
    });

    connect(m_loader, &PluginLoader::pluginUnloaded, this, [this](const QString& pluginId) {
        Q_UNUSED(pluginId)
        updatePluginTable();
        Logger::instance().info(QStringLiteral("PLUGIN_UI"),
            QStringLiteral("Plugin unloaded: %1").arg(pluginId));
    });

    connect(m_loader, &PluginLoader::pluginError, this, [this](const QString& error) {
        QMessageBox::warning(this, QStringLiteral("插件错误"), error);
    });
}

// ─── Plugin Directory Scanning ───────────────────────────────────────────────

void PluginWidget::scanPluginDirectory()
{
    m_plugins.clear();

    QString pluginDirPath = QStringLiteral("plugins");
    QDir pluginDir(pluginDirPath);

    if (!pluginDir.exists()) {
        QDir().mkpath(pluginDirPath);
        Logger::instance().info(QStringLiteral("PLUGIN_UI"),
            QStringLiteral("Plugin directory created: %1").arg(pluginDirPath));
    }

    m_pluginDirLabel->setText(
        QStringLiteral("插件目录: %1").arg(QDir::currentPath() + QStringLiteral("/") + pluginDirPath));

    // ── Preset example plugins (simulated) ──

    // 1. 配置备份插件
    PluginItemInfo p1;
    p1.id = QStringLiteral("config_backup");
    p1.name = QStringLiteral("配置备份插件");
    p1.version = QStringLiteral("1.0");
    p1.author = QStringLiteral("Wukong Team");
    p1.path = QStringLiteral("plugins/config_backup/");
    p1.status = QStringLiteral("已启用");
    p1.enabled = true;
    m_plugins.append(p1);

    // 2. 网段扫描插件
    PluginItemInfo p2;
    p2.id = QStringLiteral("subnet_scanner");
    p2.name = QStringLiteral("网段扫描插件");
    p2.version = QStringLiteral("1.2");
    p2.author = QStringLiteral("Wukong Team");
    p2.path = QStringLiteral("plugins/subnet_scanner/");
    p2.status = QStringLiteral("已启用");
    p2.enabled = true;
    m_plugins.append(p2);

    // 3. BGP Monitor
    PluginItemInfo p3;
    p3.id = QStringLiteral("bgp_monitor");
    p3.name = QStringLiteral("BGP Monitor");
    p3.version = QStringLiteral("0.9");
    p3.author = QStringLiteral("Community");
    p3.path = QStringLiteral("plugins/bgp_monitor/");
    p3.status = QStringLiteral("已禁用");
    p3.enabled = false;
    m_plugins.append(p3);

    // 4. 邮件通知插件
    PluginItemInfo p4;
    p4.id = QStringLiteral("email_notify");
    p4.name = QStringLiteral("邮件通知插件");
    p4.version = QStringLiteral("2.0");
    p4.author = QStringLiteral("Wukong Team");
    p4.path = QStringLiteral("plugins/email_notify/");
    p4.status = QStringLiteral("已启用");
    p4.enabled = true;
    m_plugins.append(p4);

    // 5. 企业微信通知
    PluginItemInfo p5;
    p5.id = QStringLiteral("wechat_work");
    p5.name = QStringLiteral("企业微信通知");
    p5.version = QStringLiteral("1.0");
    p5.author = QStringLiteral("Wukong Team");
    p5.path = QStringLiteral("plugins/wechat_work/");
    p5.status = QStringLiteral("已禁用");
    p5.enabled = false;
    m_plugins.append(p5);

    // ── Scan actual plugin directories for plugin.json ──
    if (pluginDir.exists()) {
        QStringList subdirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            QString jsonPath = pluginDir.filePath(subdir + QStringLiteral("/plugin.json"));
            QFileInfo fi(jsonPath);
            if (!fi.exists()) {
                continue;
            }

            QFile file(jsonPath);
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (!doc.isObject()) {
                continue;
            }

            QJsonObject obj = doc.object();

            PluginItemInfo info;
            info.id = obj.value(QStringLiteral("id")).toString(subdir);
            info.name = obj.value(QStringLiteral("name")).toString(subdir);
            info.version = obj.value(QStringLiteral("version")).toString(QStringLiteral("1.0"));
            info.author = obj.value(QStringLiteral("author")).toString(QStringLiteral("Unknown"));
            info.path = pluginDir.absoluteFilePath(subdir);
            info.enabled = true;
            info.status = QStringLiteral("已安装");

            // Check if already exists in preset list (avoid duplicates)
            bool exists = false;
            for (const auto& p : m_plugins) {
                if (p.id == info.id || p.name == info.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_plugins.append(info);
                Logger::instance().info(QStringLiteral("PLUGIN_UI"),
                    QStringLiteral("Found plugin: %1 v%2").arg(info.name, info.version));
            }
        }
    }

    updatePluginTable();
}

// ─── Table Update ────────────────────────────────────────────────────────────

void PluginWidget::updatePluginTable()
{
    m_pluginTable->setRowCount(0);
    m_pluginTable->setRowCount(m_plugins.size());

    for (int row = 0; row < m_plugins.size(); ++row) {
        const PluginItemInfo& info = m_plugins.at(row);

        auto* nameItem = new QTableWidgetItem(info.name);
        auto* verItem  = new QTableWidgetItem(info.version);
        auto* authItem = new QTableWidgetItem(info.author);
        auto* statusItem = new QTableWidgetItem(info.status);
        auto* pathItem = new QTableWidgetItem(info.path);

        // Color-code status
        if (info.enabled) {
            statusItem->setForeground(QColor(QString::fromLatin1(kGreen)));
        } else {
            statusItem->setForeground(QColor(QString::fromLatin1(kRed)));
        }

        nameItem->setForeground(QColor(QString::fromLatin1(kTextColor)));
        verItem->setForeground(QColor(QString::fromLatin1(kTextColor)));
        authItem->setForeground(QColor(QString::fromLatin1(kTextColor)));
        pathItem->setForeground(QColor(QString::fromLatin1("#888")));

        m_pluginTable->setItem(row, 0, nameItem);
        m_pluginTable->setItem(row, 1, verItem);
        m_pluginTable->setItem(row, 2, authItem);
        m_pluginTable->setItem(row, 3, statusItem);
        m_pluginTable->setItem(row, 4, pathItem);

        m_pluginTable->setRowHeight(row, 28);
    }

    Logger::instance().debug(QStringLiteral("PLUGIN_UI"),
        QStringLiteral("Plugin table updated: %1 entries").arg(m_plugins.size()));
}

// ─── Slots ───────────────────────────────────────────────────────────────────

void PluginWidget::onLoadPlugin()
{
    int row = m_pluginTable->currentRow();
    if (row < 0 || row >= m_plugins.size()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先选择一个插件"));
        return;
    }

    const PluginItemInfo& info = m_plugins.at(row);
    if (!info.enabled) {
        QMessageBox::warning(this, QStringLiteral("无法加载"),
            QStringLiteral("插件 \"%1\" 已被禁用，请先启用。").arg(info.name));
        return;
    }

    if (m_loader->isLoaded(info.id)) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("插件 \"%1\" 已经加载。").arg(info.name));
        return;
    }

    // Try to load the actual plugin file if it exists
    QString pluginFilePath = info.path + QStringLiteral("/") + info.id;
#ifdef Q_OS_MAC
    pluginFilePath += QStringLiteral(".dylib");
#elif defined(Q_OS_WIN)
    pluginFilePath += QStringLiteral(".dll");
#else
    pluginFilePath += QStringLiteral(".so");
#endif

    QFileInfo fi(pluginFilePath);
    if (fi.exists()) {
        m_loader->loadPlugin(pluginFilePath);
    } else {
        // Simulated load for preset plugins
        m_loader->loadPlugin(info.path);
    }

    updatePluginTable();
}

void PluginWidget::onUnloadPlugin()
{
    int row = m_pluginTable->currentRow();
    if (row < 0 || row >= m_plugins.size()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先选择一个插件"));
        return;
    }

    const PluginItemInfo& info = m_plugins.at(row);
    if (!m_loader->isLoaded(info.id)) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("插件 \"%1\" 尚未加载。").arg(info.name));
        return;
    }

    m_loader->unloadPlugin(info.id);
    updatePluginTable();
}

void PluginWidget::onTogglePlugin()
{
    int row = m_pluginTable->currentRow();
    if (row < 0 || row >= m_plugins.size()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先选择一个插件"));
        return;
    }

    PluginItemInfo& info = m_plugins[row];
    info.enabled = !info.enabled;
    info.status = info.enabled ? QStringLiteral("已启用") : QStringLiteral("已禁用");

    Logger::instance().info(QStringLiteral("PLUGIN_UI"),
        QStringLiteral("Plugin \"%1\" %2")
            .arg(info.name,
                 info.enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    updatePluginTable();
}

void PluginWidget::onInstallPlugin()
{
    QString filter;
#ifdef Q_OS_MAC
    filter = QStringLiteral("动态库 (*.dylib *.so);;所有文件 (*)");
#elif defined(Q_OS_WIN)
    filter = QStringLiteral("动态库 (*.dll);;所有文件 (*)");
#else
    filter = QStringLiteral("动态库 (*.so);;所有文件 (*)");
#endif

    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择插件文件"),
        QString(),
        filter);

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo fi(filePath);
    QString pluginDirPath = QStringLiteral("plugins");
    QDir pluginDir(pluginDirPath);

    if (!pluginDir.exists()) {
        pluginDir.mkpath(QStringLiteral("."));
    }

    // Create a subdirectory for the plugin
    QString pluginName = fi.completeBaseName();
    QString targetDir = pluginDir.filePath(pluginName);
    QDir().mkpath(targetDir);

    QString targetPath = targetDir + QStringLiteral("/") + fi.fileName();

    if (QFile::exists(targetPath)) {
        auto reply = QMessageBox::question(this,
            QStringLiteral("覆盖确认"),
            QStringLiteral("插件文件已存在，是否覆盖？"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        QFile::remove(targetPath);
    }

    if (QFile::copy(filePath, targetPath)) {
        Logger::instance().info(QStringLiteral("PLUGIN_UI"),
            QStringLiteral("Plugin installed: %1 -> %2").arg(filePath, targetPath));

        // Create a basic plugin.json if none exists
        QString jsonPath = targetDir + QStringLiteral("/plugin.json");
        if (!QFile::exists(jsonPath)) {
            QJsonObject meta;
            meta[QStringLiteral("name")] = pluginName;
            meta[QStringLiteral("version")] = QStringLiteral("1.0");
            meta[QStringLiteral("author")] = QStringLiteral("Unknown");
            meta[QStringLiteral("description")] = QStringLiteral("Installed plugin");
            meta[QStringLiteral("type")] = QStringLiteral("tool");

            QFile jsonFile(jsonPath);
            if (jsonFile.open(QIODevice::WriteOnly)) {
                jsonFile.write(QJsonDocument(meta).toJson(QJsonDocument::Indented));
                jsonFile.close();
            }
        }

        QMessageBox::information(this, QStringLiteral("安装成功"),
            QStringLiteral("插件 \"%1\" 已安装到 %2").arg(pluginName, targetDir));

        scanPluginDirectory();
    } else {
        Logger::instance().error(QStringLiteral("PLUGIN_UI"),
            QStringLiteral("Failed to copy plugin: %1 -> %2").arg(filePath, targetPath));
        QMessageBox::critical(this, QStringLiteral("安装失败"),
            QStringLiteral("无法复制插件文件到目标目录。"));
    }
}

void PluginWidget::onRefresh()
{
    Logger::instance().info(QStringLiteral("PLUGIN_UI"), QStringLiteral("Refreshing plugin list"));
    scanPluginDirectory();
}

void PluginWidget::onPluginSelected()
{
    int row = m_pluginTable->currentRow();
    if (row < 0 || row >= m_plugins.size()) {
        m_pluginDetail->clear();
        return;
    }

    const PluginItemInfo& info = m_plugins.at(row);

    QString detail;
    detail += QStringLiteral("══════════════════════════════\n");
    detail += QStringLiteral("  插件详细信息\n");
    detail += QStringLiteral("══════════════════════════════\n\n");
    detail += QStringLiteral("ID:      %1\n").arg(info.id);
    detail += QStringLiteral("名称:    %1\n").arg(info.name);
    detail += QStringLiteral("版本:    %1\n").arg(info.version);
    detail += QStringLiteral("作者:    %1\n").arg(info.author);
    detail += QStringLiteral("状态:    %1\n").arg(info.status);
    detail += QStringLiteral("路径:    %1\n").arg(info.path);
    detail += QStringLiteral("已加载:  %1\n")
        .arg(m_loader->isLoaded(info.id) ? QStringLiteral("是") : QStringLiteral("否"));

    // Try to read additional info from plugin.json if available
    QString jsonPath = info.path + QStringLiteral("/plugin.json");
    QFileInfo fi(jsonPath);
    if (fi.exists()) {
        QFile file(jsonPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString desc = obj.value(QStringLiteral("description")).toString();
                QString type = obj.value(QStringLiteral("type")).toString();
                if (!desc.isEmpty()) {
                    detail += QStringLiteral("描述:    %1\n").arg(desc);
                }
                if (!type.isEmpty()) {
                    detail += QStringLiteral("类型:    %1\n").arg(type);
                }
            }
        }
    }

    m_pluginDetail->setPlainText(detail);
}