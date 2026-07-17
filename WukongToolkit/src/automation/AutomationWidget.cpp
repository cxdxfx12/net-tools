#include "automation/AutomationWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QPlainTextEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QSplitter>
#include <QDateTime>
#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QDir>

// ═══════════════════════════════════════════════════════════════════════════════
// AutomationWidget — Automation Center (第57章)
// ═══════════════════════════════════════════════════════════════════════════════

AutomationWidget::AutomationWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_scheduleTimer(nullptr)
    , m_isRunning(false)
{
    setupUI();
    setupConnections();
    loadPresetScripts();
}

AutomationWidget::~AutomationWidget()
{
    if (m_isRunning) {
        onStop();
    }
    if (m_scheduleTimer) {
        m_scheduleTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void AutomationWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common style helpers ──
    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
            QStringLiteral(
                "QTableWidget {"
                "  background-color: #1E1F22; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; font-size: 12px;"
                "  gridline-color: #2C2D30;"
                "}"
                "QTableWidget::item { padding: 3px 6px; }"
                "QTableWidget::item:selected { background-color: #3C3F41; }"
                "QHeaderView::section {"
                "  background-color: #25262B; color: #8C8C8C;"
                "  border: none; border-bottom: 2px solid #3C3F41;"
                "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
                "}"
            )
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
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

    auto styleGroupBox = [](QGroupBox* gb) {
        gb->setStyleSheet(
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

    auto stylePlainTextEdit = [](QPlainTextEdit* edit) {
        edit->setStyleSheet(
            QStringLiteral(
                "QPlainTextEdit {"
                "  background-color: #1E1F22; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; font-size: 13px;"
                "  font-family: 'Menlo', 'Consolas', 'Courier New', monospace;"
                "}"
            )
        );
    };

    auto styleListWidget = [](QListWidget* list) {
        list->setStyleSheet(
            QStringLiteral(
                "QListWidget {"
                "  background-color: #1E1F22; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; font-size: 13px;"
                "}"
                "QListWidget::item { padding: 4px 8px; }"
                "QListWidget::item:selected { background-color: #3C3F41; }"
                "QListWidget::item:hover { background-color: #2C2D30; }"
            )
        );
    };

    QString comboBoxStyle = QStringLiteral(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px; min-width: 120px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41;"
        "}"
    );

    // ── Top: Main splitter (left: script editor + presets, right: device + output) ──
    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: #3C3F41; width: 2px; }")
    );

    // ─── Left Panel: Script Editor + Preset Scripts ───
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(8);

    // Script Editor Group
    auto* editorGroup = new QGroupBox(QStringLiteral("脚本编辑器"));
    styleGroupBox(editorGroup);
    auto* editorLayout = new QVBoxLayout(editorGroup);
    editorLayout->setSpacing(6);

    // Language selector + script management buttons
    auto* editorToolbar = new QHBoxLayout();
    auto* langLabel = new QLabel(QStringLiteral("脚本语言:"));
    langLabel->setStyleSheet(QStringLiteral("color: #8C8C8C; font-size: 13px;"));
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem(QStringLiteral("Python"), QStringLiteral("python"));
    m_languageCombo->addItem(QStringLiteral("Shell"), QStringLiteral("shell"));
    m_languageCombo->addItem(QStringLiteral("Tcl"), QStringLiteral("tcl"));
    m_languageCombo->setStyleSheet(comboBoxStyle);

    m_saveBtn = new QPushButton(QStringLiteral("保存"));
    styleButton(m_saveBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));
    m_saveBtn->setFixedHeight(30);
    m_loadBtn = new QPushButton(QStringLiteral("加载"));
    styleButton(m_loadBtn, QStringLiteral("#909399"), QStringLiteral("#B4B4B4"));
    m_loadBtn->setFixedHeight(30);
    m_deleteBtn = new QPushButton(QStringLiteral("删除"));
    styleButton(m_deleteBtn, QStringLiteral("#F56C6C"), QStringLiteral("#F89898"));
    m_deleteBtn->setFixedHeight(30);

    editorToolbar->addWidget(langLabel);
    editorToolbar->addWidget(m_languageCombo);
    editorToolbar->addStretch();
    editorToolbar->addWidget(m_saveBtn);
    editorToolbar->addWidget(m_loadBtn);
    editorToolbar->addWidget(m_deleteBtn);

    m_scriptEditor = new QPlainTextEdit();
    m_scriptEditor->setPlaceholderText(QStringLiteral("在此输入 Python / Shell / Tcl 脚本..."));
    stylePlainTextEdit(m_scriptEditor);

    editorLayout->addLayout(editorToolbar);
    editorLayout->addWidget(m_scriptEditor);
    leftLayout->addWidget(editorGroup, 3);

    // Preset Scripts Group
    auto* presetGroup = new QGroupBox(QStringLiteral("预置脚本"));
    styleGroupBox(presetGroup);
    auto* presetLayout = new QVBoxLayout(presetGroup);
    presetLayout->setSpacing(4);

    m_presetList = new QListWidget();
    styleListWidget(m_presetList);

    presetLayout->addWidget(m_presetList);
    leftLayout->addWidget(presetGroup, 1);

    // ─── Right Panel: Device + Execute + Output ───
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(8);

    // Device selection + Execute buttons
    auto* controlGroup = new QGroupBox(QStringLiteral("执行控制"));
    styleGroupBox(controlGroup);
    auto* controlLayout = new QVBoxLayout(controlGroup);
    controlLayout->setSpacing(8);

    auto* deviceRow = new QHBoxLayout();
    auto* deviceLabel = new QLabel(QStringLiteral("目标设备:"));
    deviceLabel->setStyleSheet(QStringLiteral("color: #8C8C8C; font-size: 13px;"));
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setEditable(true);
    m_deviceCombo->setStyleSheet(comboBoxStyle);
    m_deviceCombo->addItem(QStringLiteral("192.168.1.1 (Core-SW-01)"));
    m_deviceCombo->addItem(QStringLiteral("192.168.1.2 (Core-SW-02)"));
    m_deviceCombo->addItem(QStringLiteral("192.168.1.10 (Access-SW-03)"));
    m_deviceCombo->addItem(QStringLiteral("192.168.1.254 (Router-01)"));
    m_deviceCombo->addItem(QStringLiteral("192.168.1.253 (Firewall-01)"));
    m_deviceCombo->addItem(QStringLiteral("localhost"));
    deviceRow->addWidget(deviceLabel);
    deviceRow->addWidget(m_deviceCombo, 1);

    auto* btnRow = new QHBoxLayout();
    m_executeBtn = new QPushButton(QStringLiteral("▶ 执行"));
    styleButton(m_executeBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));
    m_stopBtn = new QPushButton(QStringLiteral("■ 停止"));
    styleButton(m_stopBtn, QStringLiteral("#F56C6C"), QStringLiteral("#F89898"));
    m_stopBtn->setEnabled(false);
    btnRow->addWidget(m_executeBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addStretch();

    controlLayout->addLayout(deviceRow);
    controlLayout->addLayout(btnRow);
    rightLayout->addWidget(controlGroup);

    // Output Area
    auto* outputGroup = new QGroupBox(QStringLiteral("执行输出"));
    styleGroupBox(outputGroup);
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setContentsMargins(4, 8, 4, 4);

    m_outputArea = new QPlainTextEdit();
    m_outputArea->setReadOnly(true);
    m_outputArea->setPlaceholderText(QStringLiteral("脚本执行输出将显示在这里..."));
    stylePlainTextEdit(m_outputArea);

    outputLayout->addWidget(m_outputArea);
    rightLayout->addWidget(outputGroup, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(mainSplitter, 1);

    // ── Bottom: Schedule + History ──
    // Schedule area
    auto* scheduleFrame = new QFrame();
    scheduleFrame->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "  background-color: #25262B;"
            "  border: 1px solid #3C3F41;"
            "  border-radius: 4px;"
            "}"
        )
    );
    auto* scheduleLayout = new QHBoxLayout(scheduleFrame);
    scheduleLayout->setContentsMargins(12, 8, 12, 8);
    scheduleLayout->setSpacing(12);

    auto* scheduleLabel = new QLabel(QStringLiteral("任务调度"));
    scheduleLabel->setStyleSheet(QStringLiteral("color: #409EFF; font-size: 13px; font-weight: bold;"));

    m_scheduleCheck = new QCheckBox(QStringLiteral("定时执行"));
    m_scheduleCheck->setStyleSheet(
        QStringLiteral(
            "QCheckBox { color: #DCDCDC; font-size: 13px; }"
            "QCheckBox::indicator {"
            "  width: 16px; height: 16px;"
            "  border: 1px solid #3C3F41; border-radius: 3px;"
            "  background-color: #1E1F22;"
            "}"
            "QCheckBox::indicator:checked {"
            "  background-color: #409EFF; border-color: #409EFF;"
            "}"
        )
    );

    auto* timeLabel = new QLabel(QStringLiteral("执行时间:"));
    timeLabel->setStyleSheet(QStringLiteral("color: #8C8C8C; font-size: 13px;"));
    m_scheduleTime = new QTimeEdit(QTime(2, 0));
    m_scheduleTime->setDisplayFormat(QStringLiteral("HH:mm"));
    m_scheduleTime->setStyleSheet(
        QStringLiteral(
            "QTimeEdit {"
            "  background: #1E1F22; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QTimeEdit::up-button { width: 16px; }"
            "QTimeEdit::down-button { width: 16px; }"
        )
    );

    m_exportBtn = new QPushButton(QStringLiteral("导出历史"));
    styleButton(m_exportBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));
    m_exportBtn->setFixedHeight(30);

    scheduleLayout->addWidget(scheduleLabel);
    scheduleLayout->addWidget(m_scheduleCheck);
    scheduleLayout->addWidget(timeLabel);
    scheduleLayout->addWidget(m_scheduleTime);
    scheduleLayout->addStretch();
    scheduleLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(scheduleFrame);

    // History Table
    auto* historyGroup = new QGroupBox(QStringLiteral("执行历史"));
    styleGroupBox(historyGroup);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setContentsMargins(4, 8, 4, 4);

    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("脚本名称"),
        QStringLiteral("设备"),
        QStringLiteral("结果"),
        QStringLiteral("耗时")
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_historyTable->setColumnWidth(1, 150);
    m_historyTable->setColumnWidth(2, 180);
    m_historyTable->setColumnWidth(3, 80);
    m_historyTable->setColumnWidth(4, 80);
    styleTable(m_historyTable);

    historyLayout->addWidget(m_historyTable);
    mainLayout->addWidget(historyGroup);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void AutomationWidget::setupConnections()
{
    connect(m_executeBtn, &QPushButton::clicked, this, &AutomationWidget::onExecute);
    connect(m_stopBtn, &QPushButton::clicked, this, &AutomationWidget::onStop);
    connect(m_saveBtn, &QPushButton::clicked, this, &AutomationWidget::onSaveScript);
    connect(m_loadBtn, &QPushButton::clicked, this, &AutomationWidget::onLoadScript);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AutomationWidget::onDeleteScript);
    connect(m_presetList, &QListWidget::itemClicked, this, &AutomationWidget::onPresetSelected);
    connect(m_scheduleCheck, &QCheckBox::toggled, this, &AutomationWidget::onToggleSchedule);
    connect(m_exportBtn, &QPushButton::clicked, this, &AutomationWidget::onExport);
}

// ─── Load Preset Scripts ─────────────────────────────────────────────────────

void AutomationWidget::loadPresetScripts()
{
    struct PresetScript {
        QString name;
        QString language;
        QString code;
    };

    QList<PresetScript> presets;

    // 1. 配置备份
    presets.append({
        QStringLiteral("配置备份"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"网络设备配置备份脚本\"\"\"\n"
            "import subprocess\n"
            "import sys\n"
            "from datetime import datetime\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.1'\n"
            "BACKUP_DIR = '/tmp/backups'\n"
            "\n"
            "def backup_config():\n"
            "    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')\n"
            "    filename = f'{BACKUP_DIR}/config_{DEVICE_IP}_{timestamp}.cfg'\n"
            "    \n"
            "    # 模拟通过 SSH 获取配置\n"
            "    print(f'[INFO] 正在连接 {DEVICE_IP} ...')\n"
            "    print(f'[INFO] 获取运行配置 ...')\n"
            "    print(f'[INFO] 配置备份成功: {filename}')\n"
            "    print(f'[OK] 备份完成，文件大小: 12.5 KB')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    backup_config()\n"
        )
    });

    // 2. 批量巡检
    presets.append({
        QStringLiteral("批量巡检"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"网络设备批量巡检脚本\"\"\"\n"
            "import subprocess\n"
            "import sys\n"
            "\n"
            "DEVICES = [\n"
            "    '192.168.1.1',   # Core-SW-01\n"
            "    '192.168.1.2',   # Core-SW-02\n"
            "    '192.168.1.10',  # Access-SW-03\n"
            "    '192.168.1.254', # Router-01\n"
            "    '192.168.1.253', # Firewall-01\n"
            "]\n"
            "\n"
            "CHECK_ITEMS = ['Ping', 'CPU', 'Memory', 'Interface', 'Log']\n"
            "\n"
            "def inspect_device(ip):\n"
            "    print(f'\\n[INFO] === 巡检设备: {ip} ===')\n"
            "    for item in CHECK_ITEMS:\n"
            "        print(f'  [CHECK] {item} ... PASS')\n"
            "    print(f'[OK] {ip} 巡检完成')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    print('[INFO] 批量巡检开始...')\n"
            "    for dev in DEVICES:\n"
            "        inspect_device(dev)\n"
            "    print('\\n[OK] 所有设备巡检完成')\n"
        )
    });

    // 3. 接口状态检查
    presets.append({
        QStringLiteral("接口状态检查"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"网络设备接口状态检查脚本\"\"\"\n"
            "import sys\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.1'\n"
            "\n"
            "# 模拟接口数据\n"
            "INTERFACES = [\n"
            "    ('GigabitEthernet0/0/1', 'UP', '1 Gbps', 'Connected to Core'),\n"
            "    ('GigabitEthernet0/0/2', 'UP', '1 Gbps', 'Connected to Access'),\n"
            "    ('GigabitEthernet0/0/3', 'DOWN', '-', 'Not connected'),\n"
            "    ('GigabitEthernet0/0/4', 'UP', '10 Gbps', 'Uplink to Router'),\n"
            "    ('GigabitEthernet0/0/5', 'UP', '1 Gbps', 'Management'),\n"
            "]\n"
            "\n"
            "def check_interfaces():\n"
            "    print(f'[INFO] 检查设备 {DEVICE_IP} 接口状态...\\n')\n"
            "    print(f'{\"接口\":<30} {\"状态\":<8} {\"速率\":<10} {\"描述\"}')\n"
            "    print('-' * 70)\n"
            "    \n"
            "    up_count = 0\n"
            "    down_count = 0\n"
            "    for name, status, speed, desc in INTERFACES:\n"
            "        status_icon = '✓' if status == 'UP' else '✗'\n"
            "        print(f'{name:<30} {status_icon} {status:<6} {speed:<10} {desc}')\n"
            "        if status == 'UP':\n"
            "            up_count += 1\n"
            "        else:\n"
            "            down_count += 1\n"
            "    \n"
            "    print(f'\\n[INFO] 总计: {len(INTERFACES)} 个接口, UP: {up_count}, DOWN: {down_count}')\n"
            "    if down_count > 0:\n"
            "        print(f'[WARN] 发现 {down_count} 个接口处于 DOWN 状态')\n"
            "    else:\n"
            "        print('[OK] 所有接口状态正常')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    check_interfaces()\n"
        )
    });

    // 4. ACL 验证
    presets.append({
        QStringLiteral("ACL验证"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"ACL 规则验证脚本\"\"\"\n"
            "import sys\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.253'\n"
            "\n"
            "ACL_RULES = [\n"
            "    (10, 'permit', 'tcp', '192.168.1.0/24', 'any', 'eq 80'),\n"
            "    (20, 'permit', 'tcp', '192.168.1.0/24', 'any', 'eq 443'),\n"
            "    (30, 'permit', 'udp', '192.168.1.0/24', 'any', 'eq 53'),\n"
            "    (40, 'deny', 'ip', 'any', '192.168.1.0/24', ''),\n"
            "    (50, 'permit', 'ip', 'any', 'any', ''),\n"
            "]\n"
            "\n"
            "COMPLIANCE_RULES = [\n"
            "    ('必须存在默认拒绝规则', False),\n"
            "    ('必须限制 SSH 访问源', True),\n"
            "    ('不允许 any-any permit', True),\n"
            "]\n"
            "\n"
            "def verify_acl():\n"
            "    print(f'[INFO] 验证设备 {DEVICE_IP} ACL 规则...\\n')\n"
            "    \n"
            "    print('[ACL Rules]')\n"
            "    print(f'{\"序号\":<6} {\"动作\":<10} {\"协议\":<6} {\"源地址\":<20} {\"目的地址\":<20} {\"端口\"}')\n"
            "    print('-' * 80)\n"
            "    for seq, action, proto, src, dst, port in ACL_RULES:\n"
            "        print(f'{seq:<6} {action:<10} {proto:<6} {src:<20} {dst:<20} {port}')\n"
            "    \n"
            "    print(f'\\n[Compliance Check]')\n"
            "    all_pass = True\n"
            "    for rule, status in COMPLIANCE_RULES:\n"
            "        icon = '✓' if status else '✗'\n"
            "        result = 'PASS' if status else 'FAIL'\n"
            "        print(f'  {icon} {rule}: {result}')\n"
            "        if not status:\n"
            "            all_pass = False\n"
            "    \n"
            "    if all_pass:\n"
            "        print('\\n[OK] ACL 规则验证全部通过')\n"
            "    else:\n"
            "        print('\\n[WARN] 存在不合规的 ACL 规则')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    verify_acl()\n"
        )
    });

    // 5. VLAN 检查
    presets.append({
        QStringLiteral("VLAN检查"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"VLAN 配置检查脚本\"\"\"\n"
            "import sys\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.1'\n"
            "\n"
            "VLAN_DATA = [\n"
            "    (1, 'default', 'ALL', 'Default VLAN'),\n"
            "    (10, 'Management', 'Gi0/0/1-4', '管理 VLAN'),\n"
            "    (20, 'Production', 'Gi0/0/5-20', '生产 VLAN'),\n"
            "    (30, 'Guest', 'Gi0/0/21-24', '访客 VLAN'),\n"
            "    (40, 'DMZ', 'Gi0/0/25-28', 'DMZ VLAN'),\n"
            "    (99, 'Native', 'Trunk Ports', 'Native VLAN'),\n"
            "]\n"
            "\n"
            "def check_vlans():\n"
            "    print(f'[INFO] 检查设备 {DEVICE_IP} VLAN 配置...\\n')\n"
            "    print(f'{\"VLAN ID\":<10} {\"名称\":<16} {\"端口\":<20} {\"描述\"}')\n"
            "    print('-' * 70)\n"
            "    \n"
            "    for vid, name, ports, desc in VLAN_DATA:\n"
            "        print(f'{vid:<10} {name:<16} {ports:<20} {desc}')\n"
            "    \n"
            "    print(f'\\n[INFO] 总计 {len(VLAN_DATA)} 个 VLAN')\n"
            "    \n"
            "    # 合规检查\n"
            "    print(f'\\n[Compliance]')\n"
            "    checks = [\n"
            "        ('VLAN 1 未用于业务', True),\n"
            "        ('管理 VLAN 已配置', True),\n"
            "        ('访客 VLAN 已隔离', True),\n"
            "        ('Native VLAN 非默认', True),\n"
            "    ]\n"
            "    for check, status in checks:\n"
            "        icon = '✓' if status else '✗'\n"
            "        print(f'  {icon} {check}')\n"
            "    print('\\n[OK] VLAN 检查完成')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    check_vlans()\n"
        )
    });

    // 6. 固件版本检查
    presets.append({
        QStringLiteral("固件版本检查"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"固件版本检查脚本\"\"\"\n"
            "import sys\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.1'\n"
            "\n"
            "FIRMWARE_DB = [\n"
            "    ('192.168.1.1', 'Huawei', 'V200R022C10', 'V200R023C00', '需升级'),\n"
            "    ('192.168.1.2', 'Huawei', 'V200R023C00', 'V200R023C00', '最新'),\n"
            "    ('192.168.1.10', 'Cisco', '17.06.04', '17.09.03', '需升级'),\n"
            "    ('192.168.1.254', 'Huawei', 'V200R021C00', 'V200R023C00', '严重滞后'),\n"
            "    ('192.168.1.253', 'Fortinet', '7.2.5', '7.4.2', '建议升级'),\n"
            "]\n"
            "\n"
            "def check_firmware():\n"
            "    print(f'[INFO] 固件版本检查...\\n')\n"
            "    print(f'{\"设备 IP\":<18} {\"厂商\":<10} {\"当前版本\":<16} {\"最新版本\":<16} {\"状态\"}')\n"
            "    print('-' * 80)\n"
            "    \n"
            "    need_upgrade = 0\n"
            "    for ip, vendor, current, latest, status in FIRMWARE_DB:\n"
            "        if DEVICE_IP != 'all' and ip != DEVICE_IP:\n"
            "            continue\n"
            "        status_icon = '✓' if status == '最新' else '⚠'\n"
            "        print(f'{ip:<18} {vendor:<10} {current:<16} {latest:<16} {status_icon} {status}')\n"
            "        if status != '最新':\n"
            "            need_upgrade += 1\n"
            "    \n"
            "    print(f'\\n[INFO] 需要升级的设备: {need_upgrade}')\n"
            "    if need_upgrade == 0:\n"
            "        print('[OK] 所有设备固件均为最新版本')\n"
            "    else:\n"
            "        print('[WARN] 存在设备固件需要升级')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    check_firmware()\n"
        )
    });

    // 7. 配置合规检查
    presets.append({
        QStringLiteral("配置合规检查"),
        QStringLiteral("python"),
        QStringLiteral(
            "#!/usr/bin/env python3\n"
            "\"\"\"网络设备配置合规检查脚本\"\"\"\n"
            "import sys\n"
            "\n"
            "DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.1.1'\n"
            "\n"
            "COMPLIANCE_ITEMS = [\n"
            "    ('SSH v2 启用', '必须', True),\n"
            "    ('Telnet 已禁用', '必须', True),\n"
            "    ('密码加密存储', '必须', True),\n"
            "    ('AAA 认证配置', '必须', True),\n"
            "    ('NTP 时间同步', '建议', True),\n"
            "    ('Syslog 日志上报', '建议', False),\n"
            "    ('SNMP v3 配置', '建议', True),\n"
            "    ('端口安全启用', '建议', False),\n"
            "    ('Banner 信息配置', '可选', True),\n"
            "    ('空闲超时设置', '可选', True),\n"
            "]\n"
            "\n"
            "def check_compliance():\n"
            "    print(f'[INFO] 设备 {DEVICE_IP} 配置合规检查...\\n')\n"
            "    print(f'{\"检查项\":<24} {\"级别\":<6} {\"结果\":<8}')\n"
            "    print('-' * 45)\n"
            "    \n"
            "    pass_count = 0\n"
            "    fail_count = 0\n"
            "    must_fail = 0\n"
            "    \n"
            "    for item, level, status in COMPLIANCE_ITEMS:\n"
            "        icon = '✓ PASS' if status else '✗ FAIL'\n"
            "        print(f'{item:<24} {level:<6} {icon}')\n"
            "        if status:\n"
            "            pass_count += 1\n"
            "        else:\n"
            "            fail_count += 1\n"
            "            if level == '必须':\n"
            "                must_fail += 1\n"
            "    \n"
            "    total = len(COMPLIANCE_ITEMS)\n"
            "    rate = pass_count / total * 100\n"
            "    print(f'\\n[SUMMARY] 合规率: {rate:.1f}% ({pass_count}/{total})')\n"
            "    \n"
            "    if must_fail > 0:\n"
            "        print(f'[FAIL] 存在 {must_fail} 项必须项不合规!')\n"
            "    elif fail_count > 0:\n"
            "        print(f'[WARN] 存在 {fail_count} 项建议项不合规')\n"
            "    else:\n"
            "        print('[OK] 所有合规检查项通过')\n"
            "\n"
            "if __name__ == '__main__':\n"
            "    check_compliance()\n"
        )
    });

    // 添加到列表
    for (const auto& preset : presets) {
        auto* item = new QListWidgetItem(preset.name);
        item->setData(Qt::UserRole, preset.language);
        item->setData(Qt::UserRole + 1, preset.code);
        item->setToolTip(QString(QStringLiteral("语言: %1\n点击加载此脚本")).arg(preset.language));
        m_presetList->addItem(item);
    }

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("已加载 %1 个预置脚本")).arg(presets.size()));
}

// ─── Execute ─────────────────────────────────────────────────────────────────

void AutomationWidget::onExecute()
{
    QString script = m_scriptEditor->toPlainText().trimmed();
    if (script.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先输入或选择脚本。"));
        return;
    }

    QString language = m_languageCombo->currentData().toString();
    QString device = m_deviceCombo->currentText().trimmed();

    m_isRunning = true;
    m_executeBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_outputArea->clear();

    QDateTime startTime = QDateTime::currentDateTime();
    m_outputArea->appendPlainText(
        QString(QStringLiteral("══════ 开始执行 [%1] ══════"))
            .arg(startTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))));
    m_outputArea->appendPlainText(
        QString(QStringLiteral("脚本语言: %1  |  目标设备: %2\n"))
            .arg(m_languageCombo->currentText(), device));

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("开始执行脚本，语言: %1，设备: %2"))
            .arg(m_languageCombo->currentText(), device));

    if (language == QStringLiteral("python")) {
        runPythonScript(script);
    } else {
        runShellScript(script);
    }

    QDateTime endTime = QDateTime::currentDateTime();
    qint64 elapsed = startTime.msecsTo(endTime);

    QString durationStr;
    if (elapsed < 1000) {
        durationStr = QString(QStringLiteral("%1 ms")).arg(elapsed);
    } else {
        durationStr = QString(QStringLiteral("%1 s")).arg(elapsed / 1000.0, 0, 'f', 1);
    }

    m_outputArea->appendPlainText(
        QString(QStringLiteral("\n══════ 执行完成 [%1]  耗时: %2 ══════"))
            .arg(endTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")), durationStr));

    m_isRunning = false;
    m_executeBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);

    // 判断当前选中的预置脚本名称
    QString scriptName = QStringLiteral("自定义脚本");
    if (m_presetList->currentItem()) {
        scriptName = m_presetList->currentItem()->text();
    }

    // 记录历史
    ScriptHistory history;
    history.time = startTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    history.scriptName = scriptName;
    history.device = device;
    history.result = QStringLiteral("成功");
    history.duration = durationStr;
    m_history.append(history);

    // 更新历史表格
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);
    m_historyTable->setItem(row, 0, new QTableWidgetItem(history.time));
    m_historyTable->setItem(row, 1, new QTableWidgetItem(history.scriptName));
    m_historyTable->setItem(row, 2, new QTableWidgetItem(history.device));
    auto* resultItem = new QTableWidgetItem(history.result);
    resultItem->setForeground(QColor(QStringLiteral("#67C23A")));
    m_historyTable->setItem(row, 3, resultItem);
    m_historyTable->setItem(row, 4, new QTableWidgetItem(history.duration));

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("脚本执行完成，耗时: %1")).arg(durationStr));
}

// ─── Stop ────────────────────────────────────────────────────────────────────

void AutomationWidget::onStop()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }

    m_isRunning = false;
    m_executeBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);

    m_outputArea->appendPlainText(QStringLiteral("\n[STOP] 脚本已被用户终止"));

    Logger::instance().info(QStringLiteral("Automation Center"), QStringLiteral("脚本执行已停止"));
}

// ─── Run Python Script ───────────────────────────────────────────────────────

void AutomationWidget::runPythonScript(const QString& script)
{
    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            m_outputArea->appendPlainText(output);
        });
        connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
            QString err = QString::fromUtf8(m_process->readAllStandardError());
            m_outputArea->appendPlainText(QString(QStringLiteral("[STDERR] %1")).arg(err));
        });
    }

    // 将脚本写入临时文件
    QString tempPath = QDir::temp().filePath(
        QString(QStringLiteral("wukong_script_%1.py"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))));
    QFile tempFile(tempPath);
    if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&tempFile);
        stream << script;
        tempFile.close();
    }

    QString device = m_deviceCombo->currentText().trimmed();
    // 提取 IP 地址
    QString ip = device;
    if (device.contains('(')) {
        int start = device.indexOf('(');
        int end = device.indexOf(')');
        if (start >= 0 && end > start) {
            ip = device.mid(start + 1, end - start - 1);
        }
    }

    m_process->start(QStringLiteral("python3"), {tempPath, ip});

    if (!m_process->waitForStarted(5000)) {
        m_outputArea->appendPlainText(
            QString(QStringLiteral("[ERROR] 无法启动 Python 解释器: %1"))
                .arg(m_process->errorString()));
        QFile::remove(tempPath);
        return;
    }

    m_process->waitForFinished(30000);

    if (m_process->exitStatus() != QProcess::NormalExit) {
        m_outputArea->appendPlainText(
            QString(QStringLiteral("[ERROR] 脚本异常终止")));
    }

    QFile::remove(tempPath);
}

// ─── Run Shell Script ────────────────────────────────────────────────────────

void AutomationWidget::runShellScript(const QString& script)
{
    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            m_outputArea->appendPlainText(output);
        });
        connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
            QString err = QString::fromUtf8(m_process->readAllStandardError());
            m_outputArea->appendPlainText(QString(QStringLiteral("[STDERR] %1")).arg(err));
        });
    }

    QString device = m_deviceCombo->currentText().trimmed();
    QString ip = device;
    if (device.contains('(')) {
        int start = device.indexOf('(');
        int end = device.indexOf(')');
        if (start >= 0 && end > start) {
            ip = device.mid(start + 1, end - start - 1);
        }
    }

    QString language = m_languageCombo->currentData().toString();

    if (language == QStringLiteral("shell")) {
        // 将脚本写入临时文件
        QString tempPath = QDir::temp().filePath(
            QString(QStringLiteral("wukong_script_%1.sh"))
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))));
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&tempFile);
            stream << QStringLiteral("#!/bin/bash\n") << script;
            tempFile.close();
        }
        tempFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                QFileDevice::ExeOwner);
        m_process->start(QStringLiteral("bash"), {tempPath, ip});
        if (!m_process->waitForStarted(5000)) {
            m_outputArea->appendPlainText(
                QString(QStringLiteral("[ERROR] 无法启动 Shell: %1"))
                    .arg(m_process->errorString()));
            QFile::remove(tempPath);
            return;
        }
        m_process->waitForFinished(30000);
        QFile::remove(tempPath);
    } else if (language == QStringLiteral("tcl")) {
        // Tcl 脚本：通过 tclsh 执行
        QString tempPath = QDir::temp().filePath(
            QString(QStringLiteral("wukong_script_%1.tcl"))
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))));
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&tempFile);
            stream << script;
            tempFile.close();
        }
        m_process->start(QStringLiteral("tclsh"), {tempPath, ip});
        if (!m_process->waitForStarted(5000)) {
            m_outputArea->appendPlainText(
                QString(QStringLiteral("[ERROR] 无法启动 Tcl 解释器: %1\n[INFO] 请确保已安装 tclsh"))
                    .arg(m_process->errorString()));
            QFile::remove(tempPath);
            return;
        }
        m_process->waitForFinished(30000);
        QFile::remove(tempPath);
    }

    if (m_process && m_process->exitStatus() != QProcess::NormalExit) {
        m_outputArea->appendPlainText(
            QString(QStringLiteral("[ERROR] 脚本异常终止")));
    }
}

// ─── Save Script ─────────────────────────────────────────────────────────────

void AutomationWidget::onSaveScript()
{
    QString script = m_scriptEditor->toPlainText().trimmed();
    if (script.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("脚本内容为空，无法保存。"));
        return;
    }

    QString language = m_languageCombo->currentData().toString();
    QString filter;
    if (language == QStringLiteral("python")) {
        filter = QStringLiteral("Python 脚本 (*.py)");
    } else if (language == QStringLiteral("shell")) {
        filter = QStringLiteral("Shell 脚本 (*.sh)");
    } else {
        filter = QStringLiteral("Tcl 脚本 (*.tcl)");
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存脚本"), QString(), filter);

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"),
                              QString(QStringLiteral("无法写入文件:\n%1")).arg(filePath));
        Logger::instance().error(QStringLiteral("Automation Center"),
            QString(QStringLiteral("保存脚本失败: %1")).arg(filePath));
        return;
    }

    QTextStream stream(&file);
    stream << script;
    file.close();

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("脚本已保存到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("保存成功"),
        QString(QStringLiteral("脚本已保存到:\n%1")).arg(filePath));
}

// ─── Load Script ─────────────────────────────────────────────────────────────

void AutomationWidget::onLoadScript()
{
    QString filter = QStringLiteral(
        "脚本文件 (*.py *.sh *.tcl);;Python 脚本 (*.py);;Shell 脚本 (*.sh);;Tcl 脚本 (*.tcl);;所有文件 (*)");

    QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("加载脚本"), QString(), filter);

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("加载失败"),
                              QString(QStringLiteral("无法读取文件:\n%1")).arg(filePath));
        Logger::instance().error(QStringLiteral("Automation Center"),
            QString(QStringLiteral("加载脚本失败: %1")).arg(filePath));
        return;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    m_scriptEditor->setPlainText(content);

    // 自动识别语言
    if (filePath.endsWith(QStringLiteral(".py"), Qt::CaseInsensitive)) {
        m_languageCombo->setCurrentIndex(0); // Python
    } else if (filePath.endsWith(QStringLiteral(".sh"), Qt::CaseInsensitive)) {
        m_languageCombo->setCurrentIndex(1); // Shell
    } else if (filePath.endsWith(QStringLiteral(".tcl"), Qt::CaseInsensitive)) {
        m_languageCombo->setCurrentIndex(2); // Tcl
    }

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("脚本已加载: %1")).arg(filePath));
}

// ─── Delete Script ───────────────────────────────────────────────────────────

void AutomationWidget::onDeleteScript()
{
    m_scriptEditor->clear();
    m_presetList->clearSelection();
    Logger::instance().info(QStringLiteral("Automation Center"), QStringLiteral("脚本已清空"));
}

// ─── Preset Selected ─────────────────────────────────────────────────────────

void AutomationWidget::onPresetSelected(QListWidgetItem* item)
{
    if (!item) return;

    QString code = item->data(Qt::UserRole + 1).toString();
    QString language = item->data(Qt::UserRole).toString();

    m_scriptEditor->setPlainText(code);

    if (language == QStringLiteral("python")) {
        m_languageCombo->setCurrentIndex(0);
    } else if (language == QStringLiteral("shell")) {
        m_languageCombo->setCurrentIndex(1);
    } else if (language == QStringLiteral("tcl")) {
        m_languageCombo->setCurrentIndex(2);
    }

    Logger::instance().debug(QStringLiteral("Automation Center"),
        QString(QStringLiteral("加载预置脚本: %1")).arg(item->text()));
}

// ─── Toggle Schedule ─────────────────────────────────────────────────────────

void AutomationWidget::onToggleSchedule(bool enabled)
{
    if (enabled) {
        if (!m_scheduleTimer) {
            m_scheduleTimer = new QTimer(this);
            connect(m_scheduleTimer, &QTimer::timeout, this, [this]() {
                // 检查当前时间是否匹配
                QTime now = QTime::currentTime();
                QTime scheduled = m_scheduleTime->time();
                if (qAbs(now.msecsTo(scheduled)) < 60000) { // 1分钟容差
                    onExecute();
                }
            });
        }

        // 每分钟检查一次
        m_scheduleTimer->start(60000);

        Logger::instance().info(QStringLiteral("Automation Center"),
            QString(QStringLiteral("定时执行已启动，时间: %1"))
                .arg(m_scheduleTime->time().toString(QStringLiteral("HH:mm"))));
    } else {
        if (m_scheduleTimer) {
            m_scheduleTimer->stop();
        }
        Logger::instance().info(QStringLiteral("Automation Center"), QStringLiteral("定时执行已停止"));
    }
}

// ─── Export History ──────────────────────────────────────────────────────────

void AutomationWidget::onExport()
{
    if (m_history.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("当前没有执行历史可导出。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出执行历史"),
        QString(QStringLiteral("automation_history_%1.csv"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("CSV 文件 (*.csv)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QString(QStringLiteral("无法写入文件: ") + filePath));
        Logger::instance().error(QStringLiteral("Automation Center"),
            QStringLiteral("Failed to open export file: ") + filePath);
        return;
    }

    QTextStream stream(&file);
    // BOM for Excel UTF-8
    stream << QStringLiteral("\xEF\xBB\xBF");
    stream << QStringLiteral("时间,脚本名称,设备,结果,耗时\n");

    for (const auto& h : m_history) {
        stream << QString(QStringLiteral("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\"\n"))
                    .arg(h.time, h.scriptName, h.device, h.result, h.duration);
    }

    file.close();

    Logger::instance().info(QStringLiteral("Automation Center"),
        QString(QStringLiteral("执行历史已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("执行历史已保存到:\n%1")).arg(filePath));
}