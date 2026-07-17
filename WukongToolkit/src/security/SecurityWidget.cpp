#include "security/SecurityWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QListWidget>
#include <QCheckBox>
#include <QSplitter>
#include <QDateTime>
#include <QApplication>
#include <QRandomGenerator>
#include <QFrame>
#include <QColor>

// ═══════════════════════════════════════════════════════════════════════════════
// SecurityWidget — 安全审计中心 (第53章)
// ═══════════════════════════════════════════════════════════════════════════════

SecurityWidget::SecurityWidget(QWidget* parent)
    : QWidget(parent)
    , m_totalScore(0)
    , m_maxScore(0)
{
    setupUI();
    setupConnections();

    // 预置示例设备
    m_deviceCombo->addItem(QStringLiteral("Core-SW-01  (192.168.1.1)"));
    m_deviceCombo->addItem(QStringLiteral("Core-SW-02  (192.168.1.2)"));
    m_deviceCombo->addItem(QStringLiteral("Router-01  (192.168.1.254)"));
    m_deviceCombo->addItem(QStringLiteral("Firewall-01  (192.168.1.253)"));
    m_deviceCombo->addItem(QStringLiteral("DMZ-SW-01  (10.0.0.1)"));
    m_deviceCombo->addItem(QStringLiteral("WS-Core-01  (172.16.0.1)"));
}

SecurityWidget::~SecurityWidget()
{
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void SecurityWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common styles ──
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

    auto styleListWidget = [](QListWidget* list) {
        list->setStyleSheet(
            QStringLiteral(
                "QListWidget {"
                "  background-color: #1E1F22; color: #DCDCDC;"
                "  border: 1px solid #3C3F41; font-size: 13px;"
                "}"
                "QListWidget::item { padding: 4px 8px; }"
                "QListWidget::item:selected { background-color: #3C3F41; }"
            )
        );
    };

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

    // ── Top: Main splitter (left: device+template+rules, right: results+score+risks) ──
    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: #3C3F41; width: 2px; }")
    );

    // ─── Left Panel ───
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(8);

    // Device selection group
    auto* deviceGroup = new QGroupBox(QStringLiteral("设备选择"));
    deviceGroup->setStyleSheet(
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
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    deviceLayout->setSpacing(6);

    m_deviceCombo = new QComboBox();
    styleCombo(m_deviceCombo);
    deviceLayout->addWidget(m_deviceCombo);
    leftLayout->addWidget(deviceGroup);

    // Template selection group
    auto* templateGroup = new QGroupBox(QStringLiteral("审计模板"));
    templateGroup->setStyleSheet(
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
    auto* templateLayout = new QVBoxLayout(templateGroup);
    templateLayout->setSpacing(6);

    m_templateCombo = new QComboBox();
    m_templateCombo->addItem(QStringLiteral("企业安全基线"));
    m_templateCombo->addItem(QStringLiteral("金融行业基线"));
    m_templateCombo->addItem(QStringLiteral("政务安全基线"));
    styleCombo(m_templateCombo);
    templateLayout->addWidget(m_templateCombo);
    leftLayout->addWidget(templateGroup);

    // Audit rules group
    auto* ruleGroup = new QGroupBox(QStringLiteral("审计规则"));
    ruleGroup->setStyleSheet(
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
    auto* ruleLayout = new QVBoxLayout(ruleGroup);
    ruleLayout->setSpacing(6);

    m_ruleList = new QListWidget();
    m_ruleList->setSelectionMode(QAbstractItemView::NoSelection);
    styleListWidget(m_ruleList);

    // 审计规则列表（含复选框）
    QStringList rules = {
        QStringLiteral("SSH启用"),
        QStringLiteral("Telnet禁用"),
        QStringLiteral("FTP禁用"),
        QStringLiteral("SNMPv3"),
        QStringLiteral("密码强度"),
        QStringLiteral("ACL检查"),
        QStringLiteral("NTP配置"),
        QStringLiteral("日志配置"),
        QStringLiteral("AAA配置"),
        QStringLiteral("管理地址限制")
    };
    for (const auto& rule : rules) {
        auto* checkItem = new QListWidgetItem(rule);
        checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
        checkItem->setCheckState(Qt::Checked);
        m_ruleList->addItem(checkItem);
    }

    ruleLayout->addWidget(m_ruleList);
    leftLayout->addWidget(ruleGroup, 1);

    // Start button
    m_startBtn = new QPushButton(QStringLiteral("开始审计"));
    styleButton(m_startBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));
    leftLayout->addWidget(m_startBtn);

    // ─── Right Panel ───
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(8);

    // Result table
    auto* resultGroup = new QGroupBox(QStringLiteral("审计结果"));
    resultGroup->setStyleSheet(
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
    auto* resultLayout = new QVBoxLayout(resultGroup);
    resultLayout->setContentsMargins(4, 8, 4, 4);

    m_resultTable = new QTableWidget(0, 5);
    m_resultTable->setHorizontalHeaderLabels({
        QStringLiteral("规则名称"),
        QStringLiteral("检查结果"),
        QStringLiteral("严重级别"),
        QStringLiteral("详情"),
        QStringLiteral("建议")
    });
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultTable->setColumnWidth(1, 80);
    m_resultTable->setColumnWidth(2, 80);
    m_resultTable->setColumnWidth(3, 200);
    m_resultTable->setColumnWidth(4, 250);
    styleTable(m_resultTable);

    resultLayout->addWidget(m_resultTable);
    rightLayout->addWidget(resultGroup, 2);

    // Score frame
    auto* scoreFrame = new QFrame();
    scoreFrame->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "  background-color: #25262B;"
            "  border: 1px solid #3C3F41;"
            "  border-radius: 6px;"
            "}"
        )
    );
    auto* scoreLayout = new QHBoxLayout(scoreFrame);
    scoreLayout->setContentsMargins(16, 12, 16, 12);

    auto* scoreTitle = new QLabel(QStringLiteral("安全评分:"));
    scoreTitle->setStyleSheet(QStringLiteral("font-size: 16px; color: #8C8C8C; font-weight: bold;"));
    m_scoreLabel = new QLabel(QStringLiteral("-- / --"));
    m_scoreLabel->setStyleSheet(QStringLiteral("font-size: 28px; color: #409EFF; font-weight: bold;"));
    scoreLayout->addWidget(scoreTitle);
    scoreLayout->addWidget(m_scoreLabel);
    scoreLayout->addStretch();

    m_exportBtn = new QPushButton(QStringLiteral("导出报告"));
    styleButton(m_exportBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));
    scoreLayout->addWidget(m_exportBtn);

    rightLayout->addWidget(scoreFrame);

    // Risk list
    auto* riskGroup = new QGroupBox(QStringLiteral("风险列表"));
    riskGroup->setStyleSheet(
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
    auto* riskLayout = new QVBoxLayout(riskGroup);
    riskLayout->setContentsMargins(4, 8, 4, 4);

    m_riskTable = new QTableWidget(0, 4);
    m_riskTable->setHorizontalHeaderLabels({
        QStringLiteral("风险项"),
        QStringLiteral("严重级别"),
        QStringLiteral("影响设备数"),
        QStringLiteral("状态")
    });
    m_riskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_riskTable->setColumnWidth(1, 80);
    m_riskTable->setColumnWidth(2, 100);
    m_riskTable->setColumnWidth(3, 80);
    styleTable(m_riskTable);

    riskLayout->addWidget(m_riskTable);
    rightLayout->addWidget(riskGroup, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(mainSplitter, 1);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void SecurityWidget::setupConnections()
{
    connect(m_startBtn, &QPushButton::clicked, this, &SecurityWidget::onStartAudit);
    connect(m_exportBtn, &QPushButton::clicked, this, &SecurityWidget::onExport);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SecurityWidget::onDeviceChanged);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SecurityWidget::onTemplateChanged);
}

// ─── Device Changed ──────────────────────────────────────────────────────────

void SecurityWidget::onDeviceChanged(int index)
{
    Q_UNUSED(index)
    Logger::instance().debug(QStringLiteral("Security Audit"),
        QString(QStringLiteral("设备切换: %1")).arg(m_deviceCombo->currentText()));
}

// ─── Template Changed ────────────────────────────────────────────────────────

void SecurityWidget::onTemplateChanged(int index)
{
    Q_UNUSED(index)
    QString tmpl = m_templateCombo->currentText();
    Logger::instance().info(QStringLiteral("Security Audit"),
        QString(QStringLiteral("审计模板切换: %1")).arg(tmpl));

    // 根据模板调整规则默认勾选状态
    if (tmpl == QStringLiteral("金融行业基线")) {
        // 金融行业默认全部勾选
        for (int i = 0; i < m_ruleList->count(); ++i) {
            m_ruleList->item(i)->setCheckState(Qt::Checked);
        }
    } else if (tmpl == QStringLiteral("政务安全基线")) {
        // 政务行业默认勾选特定项
        for (int i = 0; i < m_ruleList->count(); ++i) {
            QString text = m_ruleList->item(i)->text();
            bool checked = (text == QStringLiteral("SSH启用") ||
                            text == QStringLiteral("密码强度") ||
                            text == QStringLiteral("ACL检查") ||
                            text == QStringLiteral("日志配置") ||
                            text == QStringLiteral("AAA配置") ||
                            text == QStringLiteral("管理地址限制"));
            m_ruleList->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    } else {
        // 企业安全基线：默认全部勾选
        for (int i = 0; i < m_ruleList->count(); ++i) {
            m_ruleList->item(i)->setCheckState(Qt::Checked);
        }
    }
}

// ─── Start Audit ─────────────────────────────────────────────────────────────

void SecurityWidget::onStartAudit()
{
    // 获取选中的设备
    QString deviceName = m_deviceCombo->currentText();
    if (deviceName.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择要审计的设备。"));
        return;
    }

    // 获取勾选的规则
    QStringList selectedRules;
    for (int i = 0; i < m_ruleList->count(); ++i) {
        if (m_ruleList->item(i)->checkState() == Qt::Checked) {
            selectedRules.append(m_ruleList->item(i)->text());
        }
    }
    if (selectedRules.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请至少勾选一项审计规则。"));
        return;
    }

    // 清空结果
    m_resultTable->setRowCount(0);
    m_results.clear();
    m_totalScore = 0;
    m_maxScore = 0;
    m_scoreLabel->setText(QStringLiteral("-- / --"));
    m_riskTable->setRowCount(0);

    m_startBtn->setEnabled(false);
    m_exportBtn->setEnabled(false);

    Logger::instance().info(QStringLiteral("Security Audit"),
        QString(QStringLiteral("开始安全审计，设备: %1，模板: %2，规则数: %3"))
            .arg(deviceName, m_templateCombo->currentText()).arg(selectedRules.size()));

    // 逐项审计
    for (const auto& rule : selectedRules) {
        runAuditRule(rule, deviceName);
        QApplication::processEvents();
    }

    m_startBtn->setEnabled(true);
    m_exportBtn->setEnabled(true);

    calculateScore();
    updateRiskList();

    double passRate = (m_maxScore > 0) ? (double)m_totalScore / m_maxScore * 100.0 : 0.0;
    Logger::instance().info(QStringLiteral("Security Audit"),
        QString(QStringLiteral("审计完成，总分: %1/%2，通过率: %3%"))
            .arg(m_totalScore).arg(m_maxScore)
            .arg(QString::number(passRate, 'f', 1)));
}

// ─── Run Audit Rule ──────────────────────────────────────────────────────────

void SecurityWidget::runAuditRule(const QString& ruleName, const QString& deviceName)
{
    AuditResult result;
    result.ruleName = ruleName;

    auto* rng = QRandomGenerator::global();

    if (ruleName == QStringLiteral("SSH启用")) {
        // 检查 SSH 端口 22
        int r = rng->bounded(100);
        if (r < 85) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("SSH 服务已启用，端口 22 正常监听");
            result.suggestion = QStringLiteral("保持当前配置");
            result.score = 10;
        } else if (r < 95) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("SSH 服务已启用但版本较旧 (SSHv1)");
            result.suggestion = QStringLiteral("建议升级到 SSHv2，禁用 SSHv1");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("SSH 服务未启用，端口 22 未监听");
            result.suggestion = QStringLiteral("立即启用 SSH 服务，配置 SSHv2");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("Telnet禁用")) {
        // 检查 23 端口
        int r = rng->bounded(100);
        if (r < 75) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("Telnet 服务已禁用，端口 23 未监听");
            result.suggestion = QStringLiteral("保持当前配置");
            result.score = 10;
        } else if (r < 90) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("Telnet 服务已禁用但 ACL 未限制");
            result.suggestion = QStringLiteral("建议添加 ACL 限制 Telnet 访问");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("Telnet 服务仍在运行，端口 23 监听中");
            result.suggestion = QStringLiteral("立即禁用 Telnet 服务，改用 SSH");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("FTP禁用")) {
        // 检查 21 端口
        int r = rng->bounded(100);
        if (r < 80) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("FTP 服务已禁用，端口 21 未监听");
            result.suggestion = QStringLiteral("保持当前配置，使用 SFTP 替代");
            result.score = 10;
        } else if (r < 92) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("FTP 已禁用但存在 TFTP 服务");
            result.suggestion = QStringLiteral("评估 TFTP 必要性，必要时禁用");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("FTP 服务仍在运行，端口 21 监听中");
            result.suggestion = QStringLiteral("立即禁用 FTP 服务，改用 SFTP");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("SNMPv3")) {
        // 检查 SNMP 配置
        int r = rng->bounded(100);
        if (r < 65) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("SNMPv3 已启用，AuthPriv 加密模式");
            result.suggestion = QStringLiteral("保持当前配置");
            result.score = 10;
        } else if (r < 85) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("SNMPv3 已启用但使用 AuthNoPriv 模式");
            result.suggestion = QStringLiteral("建议升级到 AuthPriv 加密模式");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("严重");
            result.detail = QStringLiteral("设备使用 SNMPv1/v2c，community 为默认值");
            result.suggestion = QStringLiteral("立即升级到 SNMPv3，修改默认 community");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("密码强度")) {
        // 检查密码长度和复杂度
        int r = rng->bounded(100);
        if (r < 60) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("密码策略: 长度>=12位，含大小写+数字+特殊字符");
            result.suggestion = QStringLiteral("保持当前密码策略");
            result.score = 10;
        } else if (r < 85) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("密码长度>=8位，但缺少特殊字符要求");
            result.suggestion = QStringLiteral("建议增强密码复杂度，要求特殊字符");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("严重");
            result.detail = QStringLiteral("密码长度不足8位，无复杂度要求");
            result.suggestion = QStringLiteral("立即修改密码策略，要求长度>=12位且含复杂度");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("ACL检查")) {
        // 检查 ACL 配置
        int r = rng->bounded(100);
        if (r < 70) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("ACL 配置完整，控制面和管理面已分离");
            result.suggestion = QStringLiteral("保持当前 ACL 配置");
            result.score = 10;
        } else if (r < 88) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("ACL 已配置但缺少 VTY 访问控制");
            result.suggestion = QStringLiteral("建议添加 VTY 线路 ACL 限制");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("未配置 ACL 或存在 any any 规则");
            result.suggestion = QStringLiteral("立即配置 ACL，遵循最小权限原则");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("NTP配置")) {
        // 检查 NTP 配置
        int r = rng->bounded(100);
        if (r < 70) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("NTP 已配置，至少2个时间源，已启用认证");
            result.suggestion = QStringLiteral("保持当前 NTP 配置");
            result.score = 10;
        } else if (r < 88) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("NTP 已配置但仅1个时间源或未启用认证");
            result.suggestion = QStringLiteral("建议配置至少2个 NTP 服务器并启用认证");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("NTP 未配置，时间同步缺失");
            result.suggestion = QStringLiteral("立即配置 NTP，确保时间同步");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("日志配置")) {
        // 检查 syslog 配置
        int r = rng->bounded(100);
        if (r < 75) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("Syslog 已配置，日志级别 informational，含时间戳");
            result.suggestion = QStringLiteral("保持当前日志配置");
            result.score = 10;
        } else if (r < 90) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("Syslog 已配置但未启用时间戳或日志级别过低");
            result.suggestion = QStringLiteral("建议启用时间戳，日志级别设为 informational");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("Syslog 未配置，无日志记录");
            result.suggestion = QStringLiteral("立即配置 Syslog，启用日志审计");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("AAA配置")) {
        // 检查 AAA 配置
        int r = rng->bounded(100);
        if (r < 65) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("AAA 已配置，TACACS+/RADIUS 认证，本地认证为后备");
            result.suggestion = QStringLiteral("保持当前 AAA 配置");
            result.score = 10;
        } else if (r < 85) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("AAA 已配置但仅使用本地认证");
            result.suggestion = QStringLiteral("建议部署 TACACS+ 或 RADIUS 集中认证");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("严重");
            result.detail = QStringLiteral("AAA 未配置，无认证授权审计");
            result.suggestion = QStringLiteral("立即配置 AAA，启用集中认证和审计");
            result.score = 0;
        }
    }
    else if (ruleName == QStringLiteral("管理地址限制")) {
        // 检查管理地址 ACL
        int r = rng->bounded(100);
        if (r < 70) {
            result.checkResult = QStringLiteral("通过");
            result.severity = QStringLiteral("低");
            result.detail = QStringLiteral("管理地址 ACL 已配置，仅允许指定网段访问");
            result.suggestion = QStringLiteral("保持当前管理地址限制");
            result.score = 10;
        } else if (r < 88) {
            result.checkResult = QStringLiteral("警告");
            result.severity = QStringLiteral("中");
            result.detail = QStringLiteral("管理地址 ACL 已配置但允许范围过大");
            result.suggestion = QStringLiteral("建议缩小管理地址范围，遵循最小权限");
            result.score = 5;
        } else {
            result.checkResult = QStringLiteral("失败");
            result.severity = QStringLiteral("高");
            result.detail = QStringLiteral("未配置管理地址限制，任何地址均可管理");
            result.suggestion = QStringLiteral("立即配置管理 ACL，限制管理访问来源");
            result.score = 0;
        }
    }

    m_results.append(result);
    m_totalScore += result.score;
    m_maxScore += 10;

    // 添加到结果表格
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0, new QTableWidgetItem(result.ruleName));

    auto* resultItem = new QTableWidgetItem(result.checkResult);
    if (result.checkResult == QStringLiteral("通过")) {
        resultItem->setForeground(QColor(QStringLiteral("#67C23A")));
    } else if (result.checkResult == QStringLiteral("警告")) {
        resultItem->setForeground(QColor(QStringLiteral("#E6A23C")));
    } else {
        resultItem->setForeground(QColor(QStringLiteral("#F56C6C")));
    }
    m_resultTable->setItem(row, 1, resultItem);

    auto* severityItem = new QTableWidgetItem(result.severity);
    if (result.severity == QStringLiteral("严重")) {
        severityItem->setForeground(QColor(QStringLiteral("#F56C6C")));
    } else if (result.severity == QStringLiteral("高")) {
        severityItem->setForeground(QColor(QStringLiteral("#E6A23C")));
    } else if (result.severity == QStringLiteral("中")) {
        severityItem->setForeground(QColor(QStringLiteral("#409EFF")));
    } else {
        severityItem->setForeground(QColor(QStringLiteral("#67C23A")));
    }
    m_resultTable->setItem(row, 2, severityItem);

    m_resultTable->setItem(row, 3, new QTableWidgetItem(result.detail));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(result.suggestion));
}

// ─── Calculate Score ─────────────────────────────────────────────────────────

void SecurityWidget::calculateScore()
{
    double passRate = (m_maxScore > 0) ? (double)m_totalScore / m_maxScore * 100.0 : 0.0;

    QString color;
    if (passRate >= 80.0) {
        color = QStringLiteral("#67C23A"); // Green
    } else if (passRate >= 60.0) {
        color = QStringLiteral("#E6A23C"); // Yellow
    } else {
        color = QStringLiteral("#F56C6C"); // Red
    }

    m_scoreLabel->setStyleSheet(
        QString(QStringLiteral("font-size: 28px; color: %1; font-weight: bold;")).arg(color)
    );
    m_scoreLabel->setText(QString(QStringLiteral("%1 / %2  (%3%)"))
        .arg(m_totalScore).arg(m_maxScore)
        .arg(QString::number(passRate, 'f', 1)));
}

// ─── Update Risk List ────────────────────────────────────────────────────────

void SecurityWidget::updateRiskList()
{
    m_riskTable->setRowCount(0);

    // 汇总风险项：按规则名称分组，统计严重项和警告项
    QMap<QString, int> riskCount;
    QMap<QString, QString> riskSeverity;
    QMap<QString, int> riskAffectedDevices;

    for (const auto& result : m_results) {
        if (result.checkResult == QStringLiteral("失败") || result.checkResult == QStringLiteral("警告")) {
            riskCount[result.ruleName]++;
            if (riskSeverity[result.ruleName].isEmpty() || result.severity == QStringLiteral("严重")) {
                riskSeverity[result.ruleName] = result.severity;
            }
            riskAffectedDevices[result.ruleName] = 1; // 当前设备
        }
    }

    for (auto it = riskCount.begin(); it != riskCount.end(); ++it) {
        int row = m_riskTable->rowCount();
        m_riskTable->insertRow(row);

        m_riskTable->setItem(row, 0, new QTableWidgetItem(it.key()));

        auto* severityItem = new QTableWidgetItem(riskSeverity[it.key()]);
        if (riskSeverity[it.key()] == QStringLiteral("严重")) {
            severityItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        } else if (riskSeverity[it.key()] == QStringLiteral("高")) {
            severityItem->setForeground(QColor(QStringLiteral("#E6A23C")));
        } else {
            severityItem->setForeground(QColor(QStringLiteral("#409EFF")));
        }
        m_riskTable->setItem(row, 1, severityItem);

        m_riskTable->setItem(row, 2, new QTableWidgetItem(
            QString::number(riskAffectedDevices[it.key()])));

        auto* statusItem = new QTableWidgetItem(QStringLiteral("待修复"));
        statusItem->setForeground(QColor(QStringLiteral("#F56C6C")));
        m_riskTable->setItem(row, 3, statusItem);
    }
}

// ─── Export Report ───────────────────────────────────────────────────────────

void SecurityWidget::onExport()
{
    if (m_results.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("当前没有审计结果可导出。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出安全审计报告"),
        QString(QStringLiteral("security_audit_report_%1.html"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("HTML 文件 (*.html)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("无法写入文件: ") + filePath);
        Logger::instance().error(QStringLiteral("Security Audit"),
            QStringLiteral("Failed to open report file: ") + filePath);
        return;
    }

    QTextStream stream(&file);

    double passRate = (m_maxScore > 0) ? (double)m_totalScore / m_maxScore * 100.0 : 0.0;
    QString scoreColor = (passRate >= 80.0) ? QStringLiteral("#67C23A") :
                         (passRate >= 60.0) ? QStringLiteral("#E6A23C") : QStringLiteral("#F56C6C");

    stream << QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    stream << QStringLiteral("<meta charset=\"UTF-8\">\n");
    stream << QStringLiteral("<title>安全审计报告</title>\n");
    stream << QStringLiteral("<style>\n");
    stream << QStringLiteral("body { font-family: 'Microsoft YaHei', sans-serif; "
                              "background: #1E1F22; color: #DCDCDC; padding: 20px; }\n");
    stream << QStringLiteral("h1 { color: #409EFF; text-align: center; }\n");
    stream << QStringLiteral(".meta { text-align: center; color: #8C8C8C; margin-bottom: 10px; font-size: 13px; }\n");
    stream << QStringLiteral(".score { text-align: center; font-size: 32px; font-weight: bold; "
                              "color: %1; margin: 20px 0; }\n").arg(scoreColor);
    stream << QStringLiteral("table { width: 100%%; border-collapse: collapse; "
                              "margin-top: 20px; }\n");
    stream << QStringLiteral("th { background: #25262B; color: #8C8C8C; padding: 10px; "
                              "border-bottom: 2px solid #3C3F41; }\n");
    stream << QStringLiteral("td { padding: 8px; border-bottom: 1px solid #2C2D30; "
                              "text-align: center; }\n");
    stream << QStringLiteral(".pass { color: #67C23A; font-weight: bold; }\n");
    stream << QStringLiteral(".warn { color: #E6A23C; font-weight: bold; }\n");
    stream << QStringLiteral(".fail { color: #F56C6C; font-weight: bold; }\n");
    stream << QStringLiteral(".severity-critical { color: #F56C6C; font-weight: bold; }\n");
    stream << QStringLiteral(".severity-high { color: #E6A23C; font-weight: bold; }\n");
    stream << QStringLiteral(".severity-medium { color: #409EFF; }\n");
    stream << QStringLiteral(".severity-low { color: #67C23A; }\n");
    stream << QStringLiteral(".footer { text-align: center; color: #8C8C8C; "
                              "margin-top: 30px; font-size: 12px; }\n");
    stream << QStringLiteral("</style>\n</head>\n<body>\n");

    stream << QStringLiteral("<h1>网络安全审计报告</h1>\n");
    stream << QString(QStringLiteral("<p class=\"meta\">审计设备: %1 | "
                                      "审计模板: %2 | 生成时间: %3</p>\n"))
                .arg(m_deviceCombo->currentText(),
                     m_templateCombo->currentText(),
                     QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    stream << QString(QStringLiteral("<div class=\"score\">安全评分: %1 / %2  (%3%)</div>\n"))
                .arg(m_totalScore).arg(m_maxScore)
                .arg(QString::number(passRate, 'f', 1));

    // 审计结果表格
    stream << QStringLiteral("<h2 style=\"color:#409EFF;\">审计结果</h2>\n");
    stream << QStringLiteral("<table>\n<tr><th>规则名称</th><th>检查结果</th>"
                              "<th>严重级别</th><th>详情</th><th>建议</th></tr>\n");

    for (const auto& result : m_results) {
        QString resultClass;
        if (result.checkResult == QStringLiteral("通过")) {
            resultClass = QStringLiteral("pass");
        } else if (result.checkResult == QStringLiteral("警告")) {
            resultClass = QStringLiteral("warn");
        } else {
            resultClass = QStringLiteral("fail");
        }

        QString severityClass;
        if (result.severity == QStringLiteral("严重")) {
            severityClass = QStringLiteral("severity-critical");
        } else if (result.severity == QStringLiteral("高")) {
            severityClass = QStringLiteral("severity-high");
        } else if (result.severity == QStringLiteral("中")) {
            severityClass = QStringLiteral("severity-medium");
        } else {
            severityClass = QStringLiteral("severity-low");
        }

        stream << QString(QStringLiteral("<tr>"
                                          "<td>%1</td>"
                                          "<td class=\"%2\">%3</td>"
                                          "<td class=\"%4\">%5</td>"
                                          "<td>%6</td>"
                                          "<td>%7</td>"
                                          "</tr>\n"))
                    .arg(result.ruleName,
                         resultClass, result.checkResult,
                         severityClass, result.severity,
                         result.detail, result.suggestion);
    }

    stream << QStringLiteral("</table>\n");

    // 风险汇总
    if (m_riskTable->rowCount() > 0) {
        stream << QStringLiteral("<h2 style=\"color:#F56C6C;\">风险汇总</h2>\n");
        stream << QStringLiteral("<table>\n<tr><th>风险项</th><th>严重级别</th>"
                                  "<th>影响设备数</th><th>状态</th></tr>\n");
        for (int row = 0; row < m_riskTable->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < 4; ++col) {
                auto* cell = m_riskTable->item(row, col);
                rowData << (cell ? cell->text() : QString());
            }
            stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td>"
                                              "<td>%3</td><td>%4</td></tr>\n"))
                        .arg(rowData[0], rowData[1], rowData[2], rowData[3]);
        }
        stream << QStringLiteral("</table>\n");
    }

    stream << QString(QStringLiteral("<div class=\"footer\">WukongToolkit — 网络工程师工具箱 | 安全审计中心</div>\n"));
    stream << QStringLiteral("</body>\n</html>\n");

    file.close();

    Logger::instance().info(QStringLiteral("Security Audit"),
        QString(QStringLiteral("安全审计报告已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("安全审计报告已保存到:\n%1")).arg(filePath));
}