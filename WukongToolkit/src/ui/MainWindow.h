#pragma once

#include <QMainWindow>
#include <QTreeWidget>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QMap>
#include <QEvent>
#include <functional>

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

    // 打开工具Tab或切换到已有Tab（复用 + 限制最多3个工具Tab）
    using WidgetFactory = std::function<QWidget*()>;
    void openOrSwitchToTab(const QString& title, WidgetFactory factory);

    bool eventFilter(QObject* obj, QEvent* event) override;

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