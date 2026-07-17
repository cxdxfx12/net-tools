#pragma once

#include <QWidget>
#include <QSettings>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QFontComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>

// ============================================================================
// SettingsWidget — 系统设置 (第61章)
// ============================================================================
class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);
    ~SettingsWidget() override;

private slots:
    void onSave();
    void onReset();
    void onRestoreDefaults();
    void onCategoryChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void loadSettings();
    void saveSettings();

    // --- 左侧分类列表 ---
    QListWidget*    m_categoryList;

    // --- 右侧设置区 ---
    QStackedWidget* m_settingsStack;

    // ── 通用页面 ──
    QWidget*        m_generalPage;
    QComboBox*      m_languageCombo;
    QCheckBox*      m_checkUpdateCheck;
    QCheckBox*      m_loadSessionCheck;
    QSpinBox*       m_autoSaveSpin;

    // ── 外观页面 ──
    QWidget*        m_appearancePage;
    QComboBox*      m_themeCombo;
    QFontComboBox*  m_fontCombo;
    QSpinBox*       m_fontSizeSpin;
    QFontComboBox*  m_terminalFontCombo;
    QComboBox*      m_highlightCombo;

    // ── 网络页面 ──
    QWidget*        m_networkPage;
    QSpinBox*       m_pingTimeoutSpin;
    QSpinBox*       m_pingCountSpin;
    QLineEdit*      m_scanRangeEdit;
    QSpinBox*       m_maxThreadsSpin;

    // ── SSH 页面 ──
    QWidget*        m_sshPage;
    QSpinBox*       m_sshPortSpin;
    QLineEdit*      m_sshUserEdit;
    QSpinBox*       m_keepaliveSpin;
    QSpinBox*       m_sshTimeoutSpin;
    QLineEdit*      m_keyDirEdit;
    QPushButton*    m_browseKeyBtn;

    // ── SNMP 页面 ──
    QWidget*        m_snmpPage;
    QComboBox*      m_snmpVersionCombo;
    QLineEdit*      m_communityEdit;
    QSpinBox*       m_snmpRetrySpin;
    QSpinBox*       m_snmpTimeoutSpin;

    // ── 通知页面 ──
    QWidget*        m_notificationPage;
    QCheckBox*      m_soundNotifyCheck;
    QCheckBox*      m_popupNotifyCheck;
    QCheckBox*      m_emailNotifyCheck;
    QLineEdit*      m_mailServerEdit;
    QLineEdit*      m_recipientEdit;

    // ── 安全页面 ──
    QWidget*        m_securityPage;
    QSpinBox*       m_sessionTimeoutSpin;
    QCheckBox*      m_autoLockCheck;
    QCheckBox*      m_passwordProtectCheck;
    QSpinBox*       m_logRetentionSpin;

    // ── 关于页面 ──
    QWidget*        m_aboutPage;
    QLabel*         m_versionLabel;
    QLabel*         m_buildDateLabel;
    QLabel*         m_qtVersionLabel;
    QPlainTextEdit* m_ossListEdit;

    // ── 底部按钮 ──
    QPushButton*    m_saveBtn;
    QPushButton*    m_resetBtn;
    QPushButton*    m_restoreDefaultsBtn;

    // --- 数据 ---
    QSettings*      m_settings;
};