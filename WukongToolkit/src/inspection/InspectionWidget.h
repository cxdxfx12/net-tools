#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <QStringList>
#include <QList>

// ============================================================================
// 设备信息结构
// ============================================================================
struct InspectionDeviceInfo
{
    QString name;
    QString ip;
};

// ============================================================================
// InspectionWidget — 巡检中心 (第42章)
// ============================================================================
class InspectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InspectionWidget(QWidget* parent = nullptr);
    ~InspectionWidget() override;

private slots:
    void onSelectAll();
    void onDeselectAll();
    void onStartInspection();
    void onStopInspection();
    void onExportReport();
    void onToggleAutoInspection(bool enabled);

private:
    void setupUI();
    void setupConnections();
    void runInspectionForDevice(const QString& device, const QStringList& items);
    void updateScore();
    void addResultRow(const QString& device, const QString& item,
                      const QString& result, const QString& detail, int score);
    void addHistoryEntry(const QString& time, int deviceCount, int itemCount,
                         double passRate, int totalScore);
    int checkPing(const QString& ip);
    int checkCpu(const QString& ip);
    int checkMemory(const QString& ip);
    int checkTemperature(const QString& ip);
    int checkPowerStatus(const QString& ip);
    int checkFanStatus(const QString& ip);
    int checkInterfaceStatus(const QString& ip);
    int checkSshLogin(const QString& ip);
    int checkDns(const QString& host);
    int checkConfigCompliance(const QString& ip);
    QString runCommand(const QString& program, const QStringList& args, int timeoutMs = 5000);

    // --- 设备数据 ---
    QList<InspectionDeviceInfo> m_devices;

    // --- 定时器 ---
    QTimer* m_autoTimer;

    // --- 运行状态 ---
    bool m_isRunning;
    int m_totalScore;
    int m_maxScore;
    int m_currentDeviceIndex;

    // --- 设备选择区 ---
    QListWidget*  m_deviceList;
    QPushButton*  m_addDeviceBtn;

    // --- 巡检项选择区 ---
    QListWidget*  m_itemList;
    QPushButton*  m_selectAllBtn;
    QPushButton*  m_deselectAllBtn;

    // --- 控制按钮 ---
    QPushButton*  m_startBtn;
    QPushButton*  m_stopBtn;

    // --- 进度条 ---
    QProgressBar* m_progressBar;

    // --- 巡检结果表格 ---
    QTableWidget* m_resultTable;

    // --- 总评分 ---
    QLabel*       m_scoreLabel;

    // --- 巡检历史表格 ---
    QTableWidget* m_historyTable;

    // --- 导出/定时 ---
    QPushButton*  m_exportBtn;
    QCheckBox*    m_autoCheck;
    QComboBox*    m_intervalCombo;
};