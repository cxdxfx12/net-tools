#pragma once

#include <QMainWindow>
#include <QTreeWidget>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QMap>

class StatusBarManager;
class TerminalWidget;
class CommandHistory;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void restoreLayout();
    void saveLayout();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupMenuTree();
    void setupWorkArea();
    void setupLogDock();
    void setupStatusBar();
    void setupConnections();

    void onMenuTreeItemClicked(QTreeWidgetItem* item, int column);
    void onMenuTreeItemDoubleClicked(QTreeWidgetItem* item, int column);

    void appendLog(const QString& time, const QString& level,
                   const QString& module, const QString& message);

    void openSSHDialog();
    void openSSHTab(const QString& host, int port, const QString& user,
                    const QString& password);

    // Menu tree
    QDockWidget* m_menuDock;
    QTreeWidget* m_menuTree;

    // Work area
    QTabWidget* m_workArea;

    // Log dock
    QDockWidget* m_logDock;
    QPlainTextEdit* m_logView;

    // Status bar
    StatusBarManager* m_statusBarManager;

    // SSH session tracking
    struct SSHTabInfo {
        TerminalWidget* terminal;
        CommandHistory* history;
        QString sessionId;
    };
    QMap<QWidget*, SSHTabInfo> m_sshTabs;
};