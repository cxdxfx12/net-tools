#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QList>

class PluginLoader;

struct PluginItemInfo
{
    QString id;
    QString name;
    QString version;
    QString author;
    QString path;
    QString status;
    bool enabled = true;
};

class PluginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PluginWidget(QWidget* parent = nullptr);
    ~PluginWidget() override;

private slots:
    void onLoadPlugin();
    void onUnloadPlugin();
    void onTogglePlugin();
    void onInstallPlugin();
    void onRefresh();
    void onPluginSelected();

private:
    void setupUI();
    void setupConnections();
    void scanPluginDirectory();
    void updatePluginTable();

    // UI components
    QTableWidget* m_pluginTable;
    QPushButton* m_loadBtn;
    QPushButton* m_unloadBtn;
    QPushButton* m_toggleBtn;
    QPushButton* m_installBtn;
    QLabel* m_pluginDirLabel;
    QPushButton* m_refreshBtn;
    QPlainTextEdit* m_pluginDetail;

    // Data
    QList<PluginItemInfo> m_plugins;
    PluginLoader* m_loader;
};