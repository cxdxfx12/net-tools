#include "settings/SettingsWidget.h"
#include "ui/ThemeManager.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QFontComboBox>
#include <QApplication>
#include <QFontDatabase>
#include <QDate>

// ═══════════════════════════════════════════════════════════════════════════════
// SettingsWidget — 系统设置 (第61章)
// ═══════════════════════════════════════════════════════════════════════════════

static const int CATEGORY_GENERAL     = 0;
static const int CATEGORY_APPEARANCE  = 1;
static const int CATEGORY_NETWORK     = 2;
static const int CATEGORY_SSH         = 3;
static const int CATEGORY_SNMP        = 4;
static const int CATEGORY_NOTIFY      = 5;
static const int CATEGORY_SECURITY    = 6;
static const int CATEGORY_ABOUT       = 7;

// ─── 默认值常量 ────────────────────────────────────────────────────────────────

static const char* DEFAULT_LANGUAGE          = "zh_CN";
static const bool  DEFAULT_CHECK_UPDATE      = true;
static const bool  DEFAULT_LOAD_SESSION      = false;
static const int   DEFAULT_AUTO_SAVE         = 5;    // 分钟
static const char* DEFAULT_THEME             = "dark";
static const char* DEFAULT_FONT              = "";
static const int   DEFAULT_FONT_SIZE         = 12;
static const char* DEFAULT_TERMINAL_FONT     = "Courier New";
static const char* DEFAULT_HIGHLIGHT         = "Monokai";
static const int   DEFAULT_PING_TIMEOUT      = 2000; // ms
static const int   DEFAULT_PING_COUNT        = 4;
static const char* DEFAULT_SCAN_RANGE        = "1-1024";
static const int   DEFAULT_MAX_THREADS       = 50;
static const int   DEFAULT_SSH_PORT          = 22;
static const char* DEFAULT_SSH_USER          = "admin";
static const int   DEFAULT_KEEPALIVE         = 60;   // 秒
static const int   DEFAULT_SSH_TIMEOUT       = 10;   // 秒
static const char* DEFAULT_KEY_DIR           = "";
static const char* DEFAULT_SNMP_VERSION      = "v2c";
static const char* DEFAULT_COMMUNITY         = "public";
static const int   DEFAULT_SNMP_RETRY        = 3;
static const int   DEFAULT_SNMP_TIMEOUT      = 3000; // ms
static const bool  DEFAULT_SOUND_NOTIFY      = true;
static const bool  DEFAULT_POPUP_NOTIFY      = true;
static const bool  DEFAULT_EMAIL_NOTIFY      = false;
static const char* DEFAULT_MAIL_SERVER       = "";
static const char* DEFAULT_RECIPIENT         = "";
static const int   DEFAULT_SESSION_TIMEOUT   = 30;   // 分钟
static const bool  DEFAULT_AUTO_LOCK         = false;
static const bool  DEFAULT_PASSWORD_PROTECT  = false;
static const int   DEFAULT_LOG_RETENTION     = 30;   // 天

// ═══════════════════════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════════════════════

SettingsWidget::SettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_settings(new QSettings(QStringLiteral("WukongToolkit"), QStringLiteral("Settings"), this))
{
    setupUI();
    setupConnections();
    loadSettings();
}

SettingsWidget::~SettingsWidget()
{
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void SettingsWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 暗色主题样式 ──
    auto styleCombo = [](QComboBox* combo) {
        combo->setStyleSheet(
            QStringLiteral(
                "QComboBox {"
                "  background: #25262B; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; padding: 4px 8px;"
                "  border-radius: 3px; font-size: 13px;"
                "}"
                "QComboBox::drop-down { border: none; }"
                "QComboBox QAbstractItemView {"
                "  background: #25262B; color: #DCDCDC;"
                "  selection-background-color: #3C3F41;"
                "}"
            )
        );
    };

    auto styleSpin = [](QSpinBox* spin) {
        spin->setStyleSheet(
            QStringLiteral(
                "QSpinBox {"
                "  background: #25262B; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; padding: 4px 8px;"
                "  border-radius: 3px; font-size: 13px;"
                "}"
            )
        );
    };

    auto styleLineEdit = [](QLineEdit* edit) {
        edit->setStyleSheet(
            QStringLiteral(
                "QLineEdit {"
                "  background: #25262B; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; padding: 4px 8px;"
                "  border-radius: 3px; font-size: 13px;"
                "}"
            )
        );
    };

    auto styleCheckBox = [](QCheckBox* check) {
        check->setStyleSheet(
            QStringLiteral(
                "QCheckBox {"
                "  color: #DCDCDC; font-size: 13px;"
                "  spacing: 8px;"
                "}"
                "QCheckBox::indicator {"
                "  width: 16px; height: 16px;"
                "}"
            )
        );
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString(QStringLiteral(
                "QPushButton {"
                "  background-color: %1; color: white;"
                "  border: none; padding: 8px 20px; border-radius: 4px;"
                "  font-size: 13px; font-weight: bold;"
                "}"
                "QPushButton:hover { background-color: %2; }"
                "QPushButton:disabled { background-color: #5C5C5C; }"
            )).arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleGroupBox = [](QGroupBox* group) {
        group->setStyleSheet(
            QStringLiteral(
                "QGroupBox {"
                "  color: #409EFF; font-size: 13px; font-weight: bold;"
                "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
                "  padding-top: 16px;"
                "}"
                "QGroupBox::title {"
                "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
                "}"
            )
        );
    };

    auto styleLabel = [](QLabel* label) {
        label->setStyleSheet(
            QStringLiteral("color: #DCDCDC; font-size: 13px;")
        );
    };

    // ── 水平分割器 ──
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: #3C3F41; width: 2px; }")
    );

    // ─── 左侧分类列表 ───
    m_categoryList = new QListWidget();
    m_categoryList->setFixedWidth(160);
    m_categoryList->setStyleSheet(
        QStringLiteral(
            "QListWidget {"
            "  background-color: #1E1F22; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; font-size: 14px;"
            "}"
            "QListWidget::item { padding: 10px 12px; }"
            "QListWidget::item:selected { background-color: #3C3F41; color: #409EFF; }"
            "QListWidget::item:hover { background-color: #30323A; }"
        )
    );
    m_categoryList->addItem(QStringLiteral("通用"));
    m_categoryList->addItem(QStringLiteral("外观"));
    m_categoryList->addItem(QStringLiteral("网络"));
    m_categoryList->addItem(QStringLiteral("SSH"));
    m_categoryList->addItem(QStringLiteral("SNMP"));
    m_categoryList->addItem(QStringLiteral("通知"));
    m_categoryList->addItem(QStringLiteral("安全"));
    m_categoryList->addItem(QStringLiteral("关于"));
    m_categoryList->setCurrentRow(0);

    // ─── 右侧设置区 QStackedWidget ───
    m_settingsStack = new QStackedWidget();
    m_settingsStack->setStyleSheet(
        QStringLiteral("QStackedWidget { background-color: #1E1F22; border: 1px solid #3C3F41; }")
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 0: 通用
    // ═══════════════════════════════════════════════════════════════════════════
    m_generalPage = new QWidget();
    auto* generalLayout = new QVBoxLayout(m_generalPage);
    generalLayout->setContentsMargins(16, 16, 16, 16);
    generalLayout->setSpacing(12);

    auto* generalGroup = new QGroupBox(QStringLiteral("通用设置"));
    styleGroupBox(generalGroup);
    auto* generalForm = new QFormLayout(generalGroup);
    generalForm->setSpacing(10);
    generalForm->setContentsMargins(12, 20, 12, 12);

    m_languageCombo = new QComboBox();
    m_languageCombo->addItem(QStringLiteral("简体中文"), QStringLiteral("zh_CN"));
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en_US"));
    styleCombo(m_languageCombo);
    auto* langLabel = new QLabel(QStringLiteral("语言:"));
    styleLabel(langLabel);
    generalForm->addRow(langLabel, m_languageCombo);

    m_checkUpdateCheck = new QCheckBox(QStringLiteral("启动时检查更新"));
    styleCheckBox(m_checkUpdateCheck);
    generalForm->addRow(QString(), m_checkUpdateCheck);

    m_loadSessionCheck = new QCheckBox(QStringLiteral("启动时加载上次会话"));
    styleCheckBox(m_loadSessionCheck);
    generalForm->addRow(QString(), m_loadSessionCheck);

    m_autoSaveSpin = new QSpinBox();
    m_autoSaveSpin->setRange(1, 120);
    m_autoSaveSpin->setSuffix(QStringLiteral(" 分钟"));
    m_autoSaveSpin->setFixedWidth(140);
    styleSpin(m_autoSaveSpin);
    auto* autoSaveLabel = new QLabel(QStringLiteral("自动保存间隔:"));
    styleLabel(autoSaveLabel);
    generalForm->addRow(autoSaveLabel, m_autoSaveSpin);

    generalLayout->addWidget(generalGroup);
    generalLayout->addStretch();
    m_settingsStack->addWidget(m_generalPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 1: 外观
    // ═══════════════════════════════════════════════════════════════════════════
    m_appearancePage = new QWidget();
    auto* appearanceLayout = new QVBoxLayout(m_appearancePage);
    appearanceLayout->setContentsMargins(16, 16, 16, 16);
    appearanceLayout->setSpacing(12);

    auto* appearanceGroup = new QGroupBox(QStringLiteral("外观设置"));
    styleGroupBox(appearanceGroup);
    auto* appearanceForm = new QFormLayout(appearanceGroup);
    appearanceForm->setSpacing(10);
    appearanceForm->setContentsMargins(12, 20, 12, 12);

    m_themeCombo = new QComboBox();
    m_themeCombo->addItem(QStringLiteral("暗色"), QStringLiteral("dark"));
    m_themeCombo->addItem(QStringLiteral("亮色"), QStringLiteral("light"));
    m_themeCombo->addItem(QStringLiteral("自定义"), QStringLiteral("custom"));
    styleCombo(m_themeCombo);
    auto* themeLabel = new QLabel(QStringLiteral("主题:"));
    styleLabel(themeLabel);
    appearanceForm->addRow(themeLabel, m_themeCombo);

    m_fontCombo = new QFontComboBox();
    m_fontCombo->setStyleSheet(
        QStringLiteral(
            "QFontComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QFontComboBox::drop-down { border: none; }"
            "QFontComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        )
    );
    auto* fontLabel = new QLabel(QStringLiteral("字体:"));
    styleLabel(fontLabel);
    appearanceForm->addRow(fontLabel, m_fontCombo);

    m_fontSizeSpin = new QSpinBox();
    m_fontSizeSpin->setRange(8, 36);
    m_fontSizeSpin->setFixedWidth(100);
    styleSpin(m_fontSizeSpin);
    auto* fontSizeLabel = new QLabel(QStringLiteral("字号:"));
    styleLabel(fontSizeLabel);
    appearanceForm->addRow(fontSizeLabel, m_fontSizeSpin);

    m_terminalFontCombo = new QFontComboBox();
    m_terminalFontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    m_terminalFontCombo->setStyleSheet(
        QStringLiteral(
            "QFontComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QFontComboBox::drop-down { border: none; }"
            "QFontComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        )
    );
    auto* terminalFontLabel = new QLabel(QStringLiteral("终端字体:"));
    styleLabel(terminalFontLabel);
    appearanceForm->addRow(terminalFontLabel, m_terminalFontCombo);

    m_highlightCombo = new QComboBox();
    m_highlightCombo->addItem(QStringLiteral("Monokai"), QStringLiteral("Monokai"));
    m_highlightCombo->addItem(QStringLiteral("Solarized Dark"), QStringLiteral("Solarized Dark"));
    m_highlightCombo->addItem(QStringLiteral("Solarized Light"), QStringLiteral("Solarized Light"));
    m_highlightCombo->addItem(QStringLiteral("One Dark"), QStringLiteral("One Dark"));
    m_highlightCombo->addItem(QStringLiteral("Dracula"), QStringLiteral("Dracula"));
    m_highlightCombo->addItem(QStringLiteral("Nord"), QStringLiteral("Nord"));
    m_highlightCombo->addItem(QStringLiteral("Gruvbox"), QStringLiteral("Gruvbox"));
    styleCombo(m_highlightCombo);
    auto* highlightLabel = new QLabel(QStringLiteral("高亮配色:"));
    styleLabel(highlightLabel);
    appearanceForm->addRow(highlightLabel, m_highlightCombo);

    appearanceLayout->addWidget(appearanceGroup);
    appearanceLayout->addStretch();
    m_settingsStack->addWidget(m_appearancePage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 2: 网络
    // ═══════════════════════════════════════════════════════════════════════════
    m_networkPage = new QWidget();
    auto* networkLayout = new QVBoxLayout(m_networkPage);
    networkLayout->setContentsMargins(16, 16, 16, 16);
    networkLayout->setSpacing(12);

    auto* networkGroup = new QGroupBox(QStringLiteral("网络设置"));
    styleGroupBox(networkGroup);
    auto* networkForm = new QFormLayout(networkGroup);
    networkForm->setSpacing(10);
    networkForm->setContentsMargins(12, 20, 12, 12);

    m_pingTimeoutSpin = new QSpinBox();
    m_pingTimeoutSpin->setRange(100, 60000);
    m_pingTimeoutSpin->setSingleStep(100);
    m_pingTimeoutSpin->setSuffix(QStringLiteral(" ms"));
    m_pingTimeoutSpin->setFixedWidth(140);
    styleSpin(m_pingTimeoutSpin);
    auto* pingTimeoutLabel = new QLabel(QStringLiteral("默认 Ping 超时:"));
    styleLabel(pingTimeoutLabel);
    networkForm->addRow(pingTimeoutLabel, m_pingTimeoutSpin);

    m_pingCountSpin = new QSpinBox();
    m_pingCountSpin->setRange(1, 100);
    m_pingCountSpin->setSuffix(QStringLiteral(" 次"));
    m_pingCountSpin->setFixedWidth(120);
    styleSpin(m_pingCountSpin);
    auto* pingCountLabel = new QLabel(QStringLiteral("默认 Ping 次数:"));
    styleLabel(pingCountLabel);
    networkForm->addRow(pingCountLabel, m_pingCountSpin);

    m_scanRangeEdit = new QLineEdit();
    m_scanRangeEdit->setPlaceholderText(QStringLiteral("例如: 1-1024, 22,80,443"));
    m_scanRangeEdit->setFixedWidth(260);
    styleLineEdit(m_scanRangeEdit);
    auto* scanRangeLabel = new QLabel(QStringLiteral("端口扫描范围:"));
    styleLabel(scanRangeLabel);
    networkForm->addRow(scanRangeLabel, m_scanRangeEdit);

    m_maxThreadsSpin = new QSpinBox();
    m_maxThreadsSpin->setRange(1, 500);
    m_maxThreadsSpin->setFixedWidth(120);
    styleSpin(m_maxThreadsSpin);
    auto* maxThreadsLabel = new QLabel(QStringLiteral("最大线程数:"));
    styleLabel(maxThreadsLabel);
    networkForm->addRow(maxThreadsLabel, m_maxThreadsSpin);

    networkLayout->addWidget(networkGroup);
    networkLayout->addStretch();
    m_settingsStack->addWidget(m_networkPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 3: SSH
    // ═══════════════════════════════════════════════════════════════════════════
    m_sshPage = new QWidget();
    auto* sshLayout = new QVBoxLayout(m_sshPage);
    sshLayout->setContentsMargins(16, 16, 16, 16);
    sshLayout->setSpacing(12);

    auto* sshGroup = new QGroupBox(QStringLiteral("SSH 设置"));
    styleGroupBox(sshGroup);
    auto* sshForm = new QFormLayout(sshGroup);
    sshForm->setSpacing(10);
    sshForm->setContentsMargins(12, 20, 12, 12);

    m_sshPortSpin = new QSpinBox();
    m_sshPortSpin->setRange(1, 65535);
    m_sshPortSpin->setFixedWidth(120);
    styleSpin(m_sshPortSpin);
    auto* sshPortLabel = new QLabel(QStringLiteral("默认端口:"));
    styleLabel(sshPortLabel);
    sshForm->addRow(sshPortLabel, m_sshPortSpin);

    m_sshUserEdit = new QLineEdit();
    m_sshUserEdit->setFixedWidth(200);
    styleLineEdit(m_sshUserEdit);
    auto* sshUserLabel = new QLabel(QStringLiteral("默认用户名:"));
    styleLabel(sshUserLabel);
    sshForm->addRow(sshUserLabel, m_sshUserEdit);

    m_keepaliveSpin = new QSpinBox();
    m_keepaliveSpin->setRange(0, 3600);
    m_keepaliveSpin->setSuffix(QStringLiteral(" 秒"));
    m_keepaliveSpin->setFixedWidth(140);
    m_keepaliveSpin->setSpecialValueText(QStringLiteral("禁用"));
    styleSpin(m_keepaliveSpin);
    auto* keepaliveLabel = new QLabel(QStringLiteral("KeepAlive 间隔:"));
    styleLabel(keepaliveLabel);
    sshForm->addRow(keepaliveLabel, m_keepaliveSpin);

    m_sshTimeoutSpin = new QSpinBox();
    m_sshTimeoutSpin->setRange(1, 120);
    m_sshTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    m_sshTimeoutSpin->setFixedWidth(120);
    styleSpin(m_sshTimeoutSpin);
    auto* sshTimeoutLabel = new QLabel(QStringLiteral("连接超时:"));
    styleLabel(sshTimeoutLabel);
    sshForm->addRow(sshTimeoutLabel, m_sshTimeoutSpin);

    // 私钥目录行
    auto* keyDirLayout = new QHBoxLayout();
    m_keyDirEdit = new QLineEdit();
    m_keyDirEdit->setPlaceholderText(QStringLiteral("选择私钥目录..."));
    m_keyDirEdit->setFixedWidth(260);
    styleLineEdit(m_keyDirEdit);
    keyDirLayout->addWidget(m_keyDirEdit);

    m_browseKeyBtn = new QPushButton(QStringLiteral("浏览..."));
    styleButton(m_browseKeyBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));
    m_browseKeyBtn->setFixedWidth(70);
    keyDirLayout->addWidget(m_browseKeyBtn);
    keyDirLayout->addStretch();

    auto* keyDirLabel = new QLabel(QStringLiteral("私钥目录:"));
    styleLabel(keyDirLabel);
    sshForm->addRow(keyDirLabel, keyDirLayout);

    sshLayout->addWidget(sshGroup);
    sshLayout->addStretch();
    m_settingsStack->addWidget(m_sshPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 4: SNMP
    // ═══════════════════════════════════════════════════════════════════════════
    m_snmpPage = new QWidget();
    auto* snmpLayout = new QVBoxLayout(m_snmpPage);
    snmpLayout->setContentsMargins(16, 16, 16, 16);
    snmpLayout->setSpacing(12);

    auto* snmpGroup = new QGroupBox(QStringLiteral("SNMP 设置"));
    styleGroupBox(snmpGroup);
    auto* snmpForm = new QFormLayout(snmpGroup);
    snmpForm->setSpacing(10);
    snmpForm->setContentsMargins(12, 20, 12, 12);

    m_snmpVersionCombo = new QComboBox();
    m_snmpVersionCombo->addItem(QStringLiteral("v1"), QStringLiteral("v1"));
    m_snmpVersionCombo->addItem(QStringLiteral("v2c"), QStringLiteral("v2c"));
    m_snmpVersionCombo->addItem(QStringLiteral("v3"), QStringLiteral("v3"));
    styleCombo(m_snmpVersionCombo);
    auto* snmpVersionLabel = new QLabel(QStringLiteral("默认版本:"));
    styleLabel(snmpVersionLabel);
    snmpForm->addRow(snmpVersionLabel, m_snmpVersionCombo);

    m_communityEdit = new QLineEdit();
    m_communityEdit->setFixedWidth(200);
    styleLineEdit(m_communityEdit);
    auto* communityLabel = new QLabel(QStringLiteral("默认 Community:"));
    styleLabel(communityLabel);
    snmpForm->addRow(communityLabel, m_communityEdit);

    m_snmpRetrySpin = new QSpinBox();
    m_snmpRetrySpin->setRange(0, 10);
    m_snmpRetrySpin->setSuffix(QStringLiteral(" 次"));
    m_snmpRetrySpin->setFixedWidth(120);
    styleSpin(m_snmpRetrySpin);
    auto* snmpRetryLabel = new QLabel(QStringLiteral("重试次数:"));
    styleLabel(snmpRetryLabel);
    snmpForm->addRow(snmpRetryLabel, m_snmpRetrySpin);

    m_snmpTimeoutSpin = new QSpinBox();
    m_snmpTimeoutSpin->setRange(100, 60000);
    m_snmpTimeoutSpin->setSingleStep(100);
    m_snmpTimeoutSpin->setSuffix(QStringLiteral(" ms"));
    m_snmpTimeoutSpin->setFixedWidth(140);
    styleSpin(m_snmpTimeoutSpin);
    auto* snmpTimeoutLabel = new QLabel(QStringLiteral("超时:"));
    styleLabel(snmpTimeoutLabel);
    snmpForm->addRow(snmpTimeoutLabel, m_snmpTimeoutSpin);

    snmpLayout->addWidget(snmpGroup);
    snmpLayout->addStretch();
    m_settingsStack->addWidget(m_snmpPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 5: 通知
    // ═══════════════════════════════════════════════════════════════════════════
    m_notificationPage = new QWidget();
    auto* notifyLayout = new QVBoxLayout(m_notificationPage);
    notifyLayout->setContentsMargins(16, 16, 16, 16);
    notifyLayout->setSpacing(12);

    auto* notifyGroup = new QGroupBox(QStringLiteral("通知设置"));
    styleGroupBox(notifyGroup);
    auto* notifyForm = new QFormLayout(notifyGroup);
    notifyForm->setSpacing(10);
    notifyForm->setContentsMargins(12, 20, 12, 12);

    m_soundNotifyCheck = new QCheckBox(QStringLiteral("声音提醒"));
    styleCheckBox(m_soundNotifyCheck);
    notifyForm->addRow(QString(), m_soundNotifyCheck);

    m_popupNotifyCheck = new QCheckBox(QStringLiteral("弹窗提醒"));
    styleCheckBox(m_popupNotifyCheck);
    notifyForm->addRow(QString(), m_popupNotifyCheck);

    m_emailNotifyCheck = new QCheckBox(QStringLiteral("邮件通知"));
    styleCheckBox(m_emailNotifyCheck);
    notifyForm->addRow(QString(), m_emailNotifyCheck);

    m_mailServerEdit = new QLineEdit();
    m_mailServerEdit->setPlaceholderText(QStringLiteral("smtp.example.com:25"));
    m_mailServerEdit->setFixedWidth(260);
    styleLineEdit(m_mailServerEdit);
    auto* mailServerLabel = new QLabel(QStringLiteral("邮件服务器:"));
    styleLabel(mailServerLabel);
    notifyForm->addRow(mailServerLabel, m_mailServerEdit);

    m_recipientEdit = new QLineEdit();
    m_recipientEdit->setPlaceholderText(QStringLiteral("admin@example.com"));
    m_recipientEdit->setFixedWidth(260);
    styleLineEdit(m_recipientEdit);
    auto* recipientLabel = new QLabel(QStringLiteral("收件人:"));
    styleLabel(recipientLabel);
    notifyForm->addRow(recipientLabel, m_recipientEdit);

    notifyLayout->addWidget(notifyGroup);
    notifyLayout->addStretch();
    m_settingsStack->addWidget(m_notificationPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 6: 安全
    // ═══════════════════════════════════════════════════════════════════════════
    m_securityPage = new QWidget();
    auto* securityLayout = new QVBoxLayout(m_securityPage);
    securityLayout->setContentsMargins(16, 16, 16, 16);
    securityLayout->setSpacing(12);

    auto* securityGroup = new QGroupBox(QStringLiteral("安全设置"));
    styleGroupBox(securityGroup);
    auto* securityForm = new QFormLayout(securityGroup);
    securityForm->setSpacing(10);
    securityForm->setContentsMargins(12, 20, 12, 12);

    m_sessionTimeoutSpin = new QSpinBox();
    m_sessionTimeoutSpin->setRange(1, 1440);
    m_sessionTimeoutSpin->setSuffix(QStringLiteral(" 分钟"));
    m_sessionTimeoutSpin->setFixedWidth(140);
    styleSpin(m_sessionTimeoutSpin);
    auto* sessionTimeoutLabel = new QLabel(QStringLiteral("会话超时:"));
    styleLabel(sessionTimeoutLabel);
    securityForm->addRow(sessionTimeoutLabel, m_sessionTimeoutSpin);

    m_autoLockCheck = new QCheckBox(QStringLiteral("自动锁定"));
    styleCheckBox(m_autoLockCheck);
    securityForm->addRow(QString(), m_autoLockCheck);

    m_passwordProtectCheck = new QCheckBox(QStringLiteral("密码保护"));
    styleCheckBox(m_passwordProtectCheck);
    securityForm->addRow(QString(), m_passwordProtectCheck);

    m_logRetentionSpin = new QSpinBox();
    m_logRetentionSpin->setRange(1, 365);
    m_logRetentionSpin->setSuffix(QStringLiteral(" 天"));
    m_logRetentionSpin->setFixedWidth(120);
    styleSpin(m_logRetentionSpin);
    auto* logRetentionLabel = new QLabel(QStringLiteral("日志保留天数:"));
    styleLabel(logRetentionLabel);
    securityForm->addRow(logRetentionLabel, m_logRetentionSpin);

    securityLayout->addWidget(securityGroup);
    securityLayout->addStretch();
    m_settingsStack->addWidget(m_securityPage);

    // ═══════════════════════════════════════════════════════════════════════════
    // 页面 7: 关于
    // ═══════════════════════════════════════════════════════════════════════════
    m_aboutPage = new QWidget();
    auto* aboutLayout = new QVBoxLayout(m_aboutPage);
    aboutLayout->setContentsMargins(16, 16, 16, 16);
    aboutLayout->setSpacing(12);

    auto* aboutGroup = new QGroupBox(QStringLiteral("关于"));
    styleGroupBox(aboutGroup);
    auto* aboutForm = new QFormLayout(aboutGroup);
    aboutForm->setSpacing(10);
    aboutForm->setContentsMargins(12, 20, 12, 12);

    m_versionLabel = new QLabel(QStringLiteral("1.0.0"));
    m_versionLabel->setStyleSheet(QStringLiteral("color: #409EFF; font-size: 16px; font-weight: bold;"));
    auto* versionTitleLabel = new QLabel(QStringLiteral("版本号:"));
    styleLabel(versionTitleLabel);
    aboutForm->addRow(versionTitleLabel, m_versionLabel);

    m_buildDateLabel = new QLabel(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    m_buildDateLabel->setStyleSheet(QStringLiteral("color: #DCDCDC; font-size: 13px;"));
    auto* buildDateTitleLabel = new QLabel(QStringLiteral("构建日期:"));
    styleLabel(buildDateTitleLabel);
    aboutForm->addRow(buildDateTitleLabel, m_buildDateLabel);

    m_qtVersionLabel = new QLabel(QString::fromLatin1(qVersion()));
    m_qtVersionLabel->setStyleSheet(QStringLiteral("color: #DCDCDC; font-size: 13px;"));
    auto* qtVersionTitleLabel = new QLabel(QStringLiteral("Qt 版本:"));
    styleLabel(qtVersionTitleLabel);
    aboutForm->addRow(qtVersionTitleLabel, m_qtVersionLabel);

    aboutLayout->addWidget(aboutGroup);

    // 开源组件列表
    auto* ossGroup = new QGroupBox(QStringLiteral("开源组件"));
    styleGroupBox(ossGroup);
    auto* ossLayout = new QVBoxLayout(ossGroup);
    ossLayout->setContentsMargins(12, 20, 12, 12);

    m_ossListEdit = new QPlainTextEdit();
    m_ossListEdit->setReadOnly(true);
    m_ossListEdit->setStyleSheet(
        QStringLiteral(
            "QPlainTextEdit {"
            "  background-color: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; font-size: 12px;"
            "  font-family: 'Courier New', monospace;"
            "}"
        )
    );
    m_ossListEdit->setPlainText(QStringLiteral(
        "Qt 6.x              — LGPL v3\n"
        "libssh2             — BSD\n"
        "OpenSSL             — Apache 2.0\n"
        "Terminal (based on QTermWidget) — GPL\n"
        "nlohmann/json       — MIT\n"
    ));
    ossLayout->addWidget(m_ossListEdit);

    aboutLayout->addWidget(ossGroup, 1);
    m_settingsStack->addWidget(m_aboutPage);

    // ─── 组装分割器 ───
    splitter->addWidget(m_categoryList);
    splitter->addWidget(m_settingsStack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // ─── 底部按钮栏 ───
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    m_saveBtn = new QPushButton(QStringLiteral("保存"));
    styleButton(m_saveBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));

    m_resetBtn = new QPushButton(QStringLiteral("重置"));
    styleButton(m_resetBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));

    m_restoreDefaultsBtn = new QPushButton(QStringLiteral("恢复默认"));
    styleButton(m_restoreDefaultsBtn, QStringLiteral("#F56C6C"), QStringLiteral("#F89898"));

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_restoreDefaultsBtn);
    bottomLayout->addWidget(m_resetBtn);
    bottomLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(bottomLayout);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void SettingsWidget::setupConnections()
{
    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsWidget::onCategoryChanged);

    connect(m_saveBtn, &QPushButton::clicked,
            this, &SettingsWidget::onSave);
    connect(m_resetBtn, &QPushButton::clicked,
            this, &SettingsWidget::onReset);
    connect(m_restoreDefaultsBtn, &QPushButton::clicked,
            this, &SettingsWidget::onRestoreDefaults);

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        QString theme = m_themeCombo->itemData(idx).toString();
        if (theme == QStringLiteral("dark")) {
            ThemeManager::instance().applyDarkTheme();
        } else if (theme == QStringLiteral("light")) {
            ThemeManager::instance().applyLightTheme();
        }
    });

    connect(m_browseKeyBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, QStringLiteral("选择私钥目录"), m_keyDirEdit->text());
        if (!dir.isEmpty()) {
            m_keyDirEdit->setText(dir);
        }
    });
}

// ─── Category Changed ────────────────────────────────────────────────────────

void SettingsWidget::onCategoryChanged(int index)
{
    if (index >= 0 && index < m_settingsStack->count()) {
        m_settingsStack->setCurrentIndex(index);
    }
}

// ─── Load Settings ───────────────────────────────────────────────────────────

void SettingsWidget::loadSettings()
{
    m_settings->beginGroup(QStringLiteral("General"));
    int langIdx = m_languageCombo->findData(
        m_settings->value(QStringLiteral("language"), QString::fromLatin1(DEFAULT_LANGUAGE)));
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);
    m_checkUpdateCheck->setChecked(
        m_settings->value(QStringLiteral("checkUpdate"), DEFAULT_CHECK_UPDATE).toBool());
    m_loadSessionCheck->setChecked(
        m_settings->value(QStringLiteral("loadSession"), DEFAULT_LOAD_SESSION).toBool());
    m_autoSaveSpin->setValue(
        m_settings->value(QStringLiteral("autoSaveInterval"), DEFAULT_AUTO_SAVE).toInt());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Appearance"));
    int themeIdx = m_themeCombo->findData(
        m_settings->value(QStringLiteral("theme"), QString::fromLatin1(DEFAULT_THEME)));
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);
    QString font = m_settings->value(QStringLiteral("font"), QString::fromLatin1(DEFAULT_FONT)).toString();
    if (!font.isEmpty()) {
        m_fontCombo->setCurrentFont(QFont(font));
    }
    m_fontSizeSpin->setValue(
        m_settings->value(QStringLiteral("fontSize"), DEFAULT_FONT_SIZE).toInt());
    QString termFont = m_settings->value(QStringLiteral("terminalFont"), QString::fromLatin1(DEFAULT_TERMINAL_FONT)).toString();
    if (!termFont.isEmpty()) {
        m_terminalFontCombo->setCurrentFont(QFont(termFont));
    }
    int hlIdx = m_highlightCombo->findData(
        m_settings->value(QStringLiteral("highlight"), QString::fromLatin1(DEFAULT_HIGHLIGHT)));
    if (hlIdx >= 0) m_highlightCombo->setCurrentIndex(hlIdx);
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Network"));
    m_pingTimeoutSpin->setValue(
        m_settings->value(QStringLiteral("pingTimeout"), DEFAULT_PING_TIMEOUT).toInt());
    m_pingCountSpin->setValue(
        m_settings->value(QStringLiteral("pingCount"), DEFAULT_PING_COUNT).toInt());
    m_scanRangeEdit->setText(
        m_settings->value(QStringLiteral("scanRange"), QString::fromLatin1(DEFAULT_SCAN_RANGE)).toString());
    m_maxThreadsSpin->setValue(
        m_settings->value(QStringLiteral("maxThreads"), DEFAULT_MAX_THREADS).toInt());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("SSH"));
    m_sshPortSpin->setValue(
        m_settings->value(QStringLiteral("port"), DEFAULT_SSH_PORT).toInt());
    m_sshUserEdit->setText(
        m_settings->value(QStringLiteral("username"), QString::fromLatin1(DEFAULT_SSH_USER)).toString());
    m_keepaliveSpin->setValue(
        m_settings->value(QStringLiteral("keepalive"), DEFAULT_KEEPALIVE).toInt());
    m_sshTimeoutSpin->setValue(
        m_settings->value(QStringLiteral("timeout"), DEFAULT_SSH_TIMEOUT).toInt());
    m_keyDirEdit->setText(
        m_settings->value(QStringLiteral("keyDir"), QString::fromLatin1(DEFAULT_KEY_DIR)).toString());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("SNMP"));
    int snmpVerIdx = m_snmpVersionCombo->findData(
        m_settings->value(QStringLiteral("version"), QString::fromLatin1(DEFAULT_SNMP_VERSION)));
    if (snmpVerIdx >= 0) m_snmpVersionCombo->setCurrentIndex(snmpVerIdx);
    m_communityEdit->setText(
        m_settings->value(QStringLiteral("community"), QString::fromLatin1(DEFAULT_COMMUNITY)).toString());
    m_snmpRetrySpin->setValue(
        m_settings->value(QStringLiteral("retry"), DEFAULT_SNMP_RETRY).toInt());
    m_snmpTimeoutSpin->setValue(
        m_settings->value(QStringLiteral("timeout"), DEFAULT_SNMP_TIMEOUT).toInt());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Notification"));
    m_soundNotifyCheck->setChecked(
        m_settings->value(QStringLiteral("sound"), DEFAULT_SOUND_NOTIFY).toBool());
    m_popupNotifyCheck->setChecked(
        m_settings->value(QStringLiteral("popup"), DEFAULT_POPUP_NOTIFY).toBool());
    m_emailNotifyCheck->setChecked(
        m_settings->value(QStringLiteral("email"), DEFAULT_EMAIL_NOTIFY).toBool());
    m_mailServerEdit->setText(
        m_settings->value(QStringLiteral("mailServer"), QString::fromLatin1(DEFAULT_MAIL_SERVER)).toString());
    m_recipientEdit->setText(
        m_settings->value(QStringLiteral("recipient"), QString::fromLatin1(DEFAULT_RECIPIENT)).toString());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Security"));
    m_sessionTimeoutSpin->setValue(
        m_settings->value(QStringLiteral("sessionTimeout"), DEFAULT_SESSION_TIMEOUT).toInt());
    m_autoLockCheck->setChecked(
        m_settings->value(QStringLiteral("autoLock"), DEFAULT_AUTO_LOCK).toBool());
    m_passwordProtectCheck->setChecked(
        m_settings->value(QStringLiteral("passwordProtect"), DEFAULT_PASSWORD_PROTECT).toBool());
    m_logRetentionSpin->setValue(
        m_settings->value(QStringLiteral("logRetention"), DEFAULT_LOG_RETENTION).toInt());
    m_settings->endGroup();

    Logger::instance().debug(QStringLiteral("Settings"),
        QStringLiteral("设置已加载"));
}

// ─── Save Settings ───────────────────────────────────────────────────────────

void SettingsWidget::saveSettings()
{
    m_settings->beginGroup(QStringLiteral("General"));
    m_settings->setValue(QStringLiteral("language"), m_languageCombo->currentData());
    m_settings->setValue(QStringLiteral("checkUpdate"), m_checkUpdateCheck->isChecked());
    m_settings->setValue(QStringLiteral("loadSession"), m_loadSessionCheck->isChecked());
    m_settings->setValue(QStringLiteral("autoSaveInterval"), m_autoSaveSpin->value());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Appearance"));
    m_settings->setValue(QStringLiteral("theme"), m_themeCombo->currentData());
    m_settings->setValue(QStringLiteral("font"), m_fontCombo->currentFont().family());
    m_settings->setValue(QStringLiteral("fontSize"), m_fontSizeSpin->value());
    m_settings->setValue(QStringLiteral("terminalFont"), m_terminalFontCombo->currentFont().family());
    m_settings->setValue(QStringLiteral("highlight"), m_highlightCombo->currentData());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Network"));
    m_settings->setValue(QStringLiteral("pingTimeout"), m_pingTimeoutSpin->value());
    m_settings->setValue(QStringLiteral("pingCount"), m_pingCountSpin->value());
    m_settings->setValue(QStringLiteral("scanRange"), m_scanRangeEdit->text());
    m_settings->setValue(QStringLiteral("maxThreads"), m_maxThreadsSpin->value());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("SSH"));
    m_settings->setValue(QStringLiteral("port"), m_sshPortSpin->value());
    m_settings->setValue(QStringLiteral("username"), m_sshUserEdit->text());
    m_settings->setValue(QStringLiteral("keepalive"), m_keepaliveSpin->value());
    m_settings->setValue(QStringLiteral("timeout"), m_sshTimeoutSpin->value());
    m_settings->setValue(QStringLiteral("keyDir"), m_keyDirEdit->text());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("SNMP"));
    m_settings->setValue(QStringLiteral("version"), m_snmpVersionCombo->currentData());
    m_settings->setValue(QStringLiteral("community"), m_communityEdit->text());
    m_settings->setValue(QStringLiteral("retry"), m_snmpRetrySpin->value());
    m_settings->setValue(QStringLiteral("timeout"), m_snmpTimeoutSpin->value());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Notification"));
    m_settings->setValue(QStringLiteral("sound"), m_soundNotifyCheck->isChecked());
    m_settings->setValue(QStringLiteral("popup"), m_popupNotifyCheck->isChecked());
    m_settings->setValue(QStringLiteral("email"), m_emailNotifyCheck->isChecked());
    m_settings->setValue(QStringLiteral("mailServer"), m_mailServerEdit->text());
    m_settings->setValue(QStringLiteral("recipient"), m_recipientEdit->text());
    m_settings->endGroup();

    m_settings->beginGroup(QStringLiteral("Security"));
    m_settings->setValue(QStringLiteral("sessionTimeout"), m_sessionTimeoutSpin->value());
    m_settings->setValue(QStringLiteral("autoLock"), m_autoLockCheck->isChecked());
    m_settings->setValue(QStringLiteral("passwordProtect"), m_passwordProtectCheck->isChecked());
    m_settings->setValue(QStringLiteral("logRetention"), m_logRetentionSpin->value());
    m_settings->endGroup();

    m_settings->sync();
}

// ─── On Save ─────────────────────────────────────────────────────────────────

void SettingsWidget::onSave()
{
    saveSettings();
    Logger::instance().info(QStringLiteral("Settings"),
        QStringLiteral("设置已保存"));
    QMessageBox::information(this, QStringLiteral("保存成功"),
        QStringLiteral("系统设置已保存。"));
}

// ─── On Reset ────────────────────────────────────────────────────────────────

void SettingsWidget::onReset()
{
    loadSettings();
    Logger::instance().info(QStringLiteral("Settings"),
        QStringLiteral("设置已重置为上次保存的值"));
    QMessageBox::information(this, QStringLiteral("重置"),
        QStringLiteral("设置已重置为上次保存的值。"));
}

// ─── On Restore Defaults ─────────────────────────────────────────────────────

void SettingsWidget::onRestoreDefaults()
{
    auto reply = QMessageBox::question(this, QStringLiteral("恢复默认设置"),
        QStringLiteral("确定要恢复所有设置为默认值吗？\n此操作不可撤销。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // 恢复所有默认值
    int langIdx = m_languageCombo->findData(QString::fromLatin1(DEFAULT_LANGUAGE));
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);
    m_checkUpdateCheck->setChecked(DEFAULT_CHECK_UPDATE);
    m_loadSessionCheck->setChecked(DEFAULT_LOAD_SESSION);
    m_autoSaveSpin->setValue(DEFAULT_AUTO_SAVE);

    int themeIdx = m_themeCombo->findData(QString::fromLatin1(DEFAULT_THEME));
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);
    m_fontSizeSpin->setValue(DEFAULT_FONT_SIZE);
    int hlIdx = m_highlightCombo->findData(QString::fromLatin1(DEFAULT_HIGHLIGHT));
    if (hlIdx >= 0) m_highlightCombo->setCurrentIndex(hlIdx);

    m_pingTimeoutSpin->setValue(DEFAULT_PING_TIMEOUT);
    m_pingCountSpin->setValue(DEFAULT_PING_COUNT);
    m_scanRangeEdit->setText(QString::fromLatin1(DEFAULT_SCAN_RANGE));
    m_maxThreadsSpin->setValue(DEFAULT_MAX_THREADS);

    m_sshPortSpin->setValue(DEFAULT_SSH_PORT);
    m_sshUserEdit->setText(QString::fromLatin1(DEFAULT_SSH_USER));
    m_keepaliveSpin->setValue(DEFAULT_KEEPALIVE);
    m_sshTimeoutSpin->setValue(DEFAULT_SSH_TIMEOUT);
    m_keyDirEdit->clear();

    int snmpVerIdx = m_snmpVersionCombo->findData(QString::fromLatin1(DEFAULT_SNMP_VERSION));
    if (snmpVerIdx >= 0) m_snmpVersionCombo->setCurrentIndex(snmpVerIdx);
    m_communityEdit->setText(QString::fromLatin1(DEFAULT_COMMUNITY));
    m_snmpRetrySpin->setValue(DEFAULT_SNMP_RETRY);
    m_snmpTimeoutSpin->setValue(DEFAULT_SNMP_TIMEOUT);

    m_soundNotifyCheck->setChecked(DEFAULT_SOUND_NOTIFY);
    m_popupNotifyCheck->setChecked(DEFAULT_POPUP_NOTIFY);
    m_emailNotifyCheck->setChecked(DEFAULT_EMAIL_NOTIFY);
    m_mailServerEdit->clear();
    m_recipientEdit->clear();

    m_sessionTimeoutSpin->setValue(DEFAULT_SESSION_TIMEOUT);
    m_autoLockCheck->setChecked(DEFAULT_AUTO_LOCK);
    m_passwordProtectCheck->setChecked(DEFAULT_PASSWORD_PROTECT);
    m_logRetentionSpin->setValue(DEFAULT_LOG_RETENTION);

    m_settings->clear();
    m_settings->sync();
    saveSettings();

    Logger::instance().info(QStringLiteral("Settings"),
        QStringLiteral("所有设置已恢复为默认值"));
    QMessageBox::information(this, QStringLiteral("恢复默认"),
        QStringLiteral("所有设置已恢复为默认值。"));
}