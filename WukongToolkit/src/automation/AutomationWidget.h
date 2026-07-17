#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>
#include <QTimeEdit>
#include <QTableWidget>
#include <QProcess>
#include <QTimer>
#include <QList>

// ============================================================================
// 脚本执行历史记录结构
// ============================================================================
struct ScriptHistory
{
    QString time;
    QString scriptName;
    QString device;
    QString result;
    QString duration;
};

// ============================================================================
// AutomationWidget — Automation Center (第57章)
// ============================================================================
class AutomationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AutomationWidget(QWidget* parent = nullptr);
    ~AutomationWidget() override;

private slots:
    void onExecute();
    void onStop();
    void onSaveScript();
    void onLoadScript();
    void onDeleteScript();
    void onPresetSelected(QListWidgetItem* item);
    void onToggleSchedule(bool enabled);
    void onExport();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupConnections();
    void loadPresetScripts();
    void runPythonScript(const QString& script);
    void runShellScript(const QString& script);
    void finalizeExecution(bool success, const QString& durationStr);

    // --- 脚本编辑器 ---
    QPlainTextEdit* m_scriptEditor;
    QComboBox*      m_languageCombo;

    // --- 目标设备 ---
    QComboBox*      m_deviceCombo;

    // --- 执行控制 ---
    QPushButton*    m_executeBtn;
    QPushButton*    m_stopBtn;

    // --- 输出区 ---
    QPlainTextEdit* m_outputArea;

    // --- 预置脚本列表 ---
    QListWidget*    m_presetList;

    // --- 脚本管理 ---
    QPushButton*    m_saveBtn;
    QPushButton*    m_loadBtn;
    QPushButton*    m_deleteBtn;

    // --- 任务调度 ---
    QCheckBox*      m_scheduleCheck;
    QTimeEdit*      m_scheduleTime;

    // --- 执行历史 ---
    QTableWidget*   m_historyTable;

    // --- 导出 ---
    QPushButton*    m_exportBtn;

    // --- 进程 & 定时器 ---
    QProcess*       m_process;
    QTimer*         m_scheduleTimer;

    // --- 历史记录 ---
    QList<ScriptHistory> m_history;

    // --- 运行状态 ---
    bool m_isRunning;
    QDateTime m_executionStartTime;
    QString m_tempFilePath;
};