#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QTimer>
#include <QProcess>
#include <QList>

// ============================================================================
// ConfigVersion — config version info
// ============================================================================
struct ConfigVersion
{
    QString id;
    QString device;
    QString timestamp;
    qint64  size = 0;
    QString note;
    QString content;
};

// ============================================================================
// ConfigCenterWidget — Configuration Center
// ============================================================================
class ConfigCenterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigCenterWidget(QWidget* parent = nullptr);
    ~ConfigCenterWidget() override;

private slots:
    void onAddDevice();
    void onBackup();
    void onRestore();
    void onToggleAutoBackup(bool enabled);
    void onVersionSelected();
    void onDiff();
    void onBatchDeploy();
    void onComplianceCheck();
    void onExport();
    void onTemplateChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void updateDeviceList();
    void updateVersionList();
    QString fetchDeviceConfig(const QString& ip);
    QString computeDiff(const QString& oldText, const QString& newText);
    void saveConfigVersion(const QString& device, const QString& config, const QString& note);

    QString generateId();
    QString currentTimestamp();

    // Left panel: device management
    QComboBox*      m_deviceCombo;
    QPushButton*    m_addDeviceBtn;
    QPushButton*    m_backupBtn;
    QPushButton*    m_restoreBtn;
    QCheckBox*      m_autoBackupCheck;
    QComboBox*      m_backupIntervalCombo;

    // Middle panel: version list + config content
    QTableWidget*   m_versionTable;
    QPlainTextEdit* m_configView;

    // Right panel: diff + template management
    QPushButton*    m_diffBtn;
    QPlainTextEdit* m_diffView;
    QComboBox*      m_templateCombo;
    QPlainTextEdit* m_templateEditor;
    QPushButton*    m_batchDeployBtn;
    QPushButton*    m_complianceBtn;
    QPushButton*    m_exportBtn;

    // Timer
    QTimer*         m_autoBackupTimer;

    // Data
    QList<ConfigVersion> m_versions;

    // Built-in templates
    QString m_ciscoTemplate;
    QString m_huaweiVrpTemplate;
    QString m_h3cComwareTemplate;

    // Version table columns
    static constexpr int VER_COL_ID      = 0;
    static constexpr int VER_COL_DEVICE  = 1;
    static constexpr int VER_COL_TIME    = 2;
    static constexpr int VER_COL_SIZE    = 3;
    static constexpr int VER_COL_NOTE    = 4;
};