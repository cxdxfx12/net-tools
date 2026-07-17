#include "inspection/InspectionWidget.h"
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
#include <QProgressBar>
#include <QListWidget>
#include <QCheckBox>
#include <QSplitter>
#include <QInputDialog>
#include <QDateTime>
#include <QApplication>
#include <QRandomGenerator>
#include <QFrame>

// ═══════════════════════════════════════════════════════════════════════════════
// InspectionWidget
// ═══════════════════════════════════════════════════════════════════════════════

InspectionWidget::InspectionWidget(QWidget* parent)
    : QWidget(parent)
    , m_autoTimer(nullptr)
    , m_isRunning(false)
    , m_totalScore(0)
    , m_maxScore(0)
    , m_currentDeviceIndex(0)
{
    setupUI();
    setupConnections();

    // 预置示例设备
    m_devices.append({"Core-SW-01", QStringLiteral("192.168.1.1")});
    m_devices.append({"Core-SW-02", QStringLiteral("192.168.1.2")});
    m_devices.append({"Access-SW-03", QStringLiteral("192.168.1.10")});
    m_devices.append({"Router-01", QStringLiteral("192.168.1.254")});
    m_devices.append({"Firewall-01", QStringLiteral("192.168.1.253")});

    for (const auto& dev : m_devices) {
        m_deviceList->addItem(QString(QStringLiteral("%1  (%2)")).arg(dev.name, dev.ip));
    }
}

InspectionWidget::~InspectionWidget()
{
    if (m_isRunning) {
        onStopInspection();
    }
    if (m_autoTimer) {
        m_autoTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void InspectionWidget::setupUI()
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

    auto styleLabel = [](QLabel* label, const QString& color = QStringLiteral("#8C8C8C"), int fontSize = 12) {
        label->setStyleSheet(
            QString(QStringLiteral("font-size: %1px; color: %2;")).arg(fontSize).arg(color)
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
            )
        );
    };

    // ── Top: Main splitter (left: device+items, right: results+history) ──
    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background-color: #3C3F41; width: 2px; }")
    );

    // ─── Left Panel: Device selection + Inspection items ───
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

    m_deviceList = new QListWidget();
    m_deviceList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_deviceList->setMaximumHeight(150);
    styleListWidget(m_deviceList);

    m_addDeviceBtn = new QPushButton(QStringLiteral("+ 添加设备"));
    styleButton(m_addDeviceBtn, QStringLiteral("#409EFF"), QStringLiteral("#66B1FF"));

    deviceLayout->addWidget(m_deviceList);
    deviceLayout->addWidget(m_addDeviceBtn);
    leftLayout->addWidget(deviceGroup);

    // Inspection items group
    auto* itemGroup = new QGroupBox(QStringLiteral("巡检项选择"));
    itemGroup->setStyleSheet(
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
    auto* itemLayout = new QVBoxLayout(itemGroup);
    itemLayout->setSpacing(6);

    m_itemList = new QListWidget();
    m_itemList->setSelectionMode(QAbstractItemView::NoSelection);
    styleListWidget(m_itemList);

    // 检查项列表（含复选框）
    QStringList items = {
        QStringLiteral("Ping连通性"),
        QStringLiteral("CPU使用率"),
        QStringLiteral("内存使用率"),
        QStringLiteral("温度"),
        QStringLiteral("电源状态"),
        QStringLiteral("风扇状态"),
        QStringLiteral("接口状态"),
        QStringLiteral("SSH登录"),
        QStringLiteral("DNS解析"),
        QStringLiteral("配置合规")
    };
    for (const auto& item : items) {
        auto* checkItem = new QListWidgetItem(item);
        checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
        checkItem->setCheckState(Qt::Checked);
        m_itemList->addItem(checkItem);
    }

    auto* itemBtnLayout = new QHBoxLayout();
    m_selectAllBtn = new QPushButton(QStringLiteral("全选"));
    styleButton(m_selectAllBtn, QStringLiteral("#909399"), QStringLiteral("#B4B4B4"));
    m_selectAllBtn->setFixedHeight(28);
    m_deselectAllBtn = new QPushButton(QStringLiteral("取消全选"));
    styleButton(m_deselectAllBtn, QStringLiteral("#909399"), QStringLiteral("#B4B4B4"));
    m_deselectAllBtn->setFixedHeight(28);
    itemBtnLayout->addWidget(m_selectAllBtn);
    itemBtnLayout->addWidget(m_deselectAllBtn);
    itemBtnLayout->addStretch();

    itemLayout->addWidget(m_itemList);
    itemLayout->addLayout(itemBtnLayout);
    leftLayout->addWidget(itemGroup, 1);

    // ─── Right Panel: Progress + Results + Score + History ───
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(8);

    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(26);
    m_progressBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar {"
            "  background-color: #1E1F22; border: 1px solid #3C3F41;"
            "  border-radius: 4px; text-align: center; color: white;"
            "  font-size: 13px; font-weight: bold;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #409EFF; border-radius: 3px;"
            "}"
        )
    );
    rightLayout->addWidget(m_progressBar);

    // Result table
    auto* resultGroup = new QGroupBox(QStringLiteral("巡检结果"));
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

    m_resultTable = new QTableWidget(0, 6);
    m_resultTable->setHorizontalHeaderLabels({
        QStringLiteral("设备"),
        QStringLiteral("检查项"),
        QStringLiteral("结果"),
        QStringLiteral("详情"),
        QStringLiteral("评分"),
        QStringLiteral("时间")
    });
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_resultTable->setColumnWidth(2, 70);
    m_resultTable->setColumnWidth(3, 180);
    m_resultTable->setColumnWidth(4, 60);
    m_resultTable->setColumnWidth(5, 160);
    styleTable(m_resultTable);

    resultLayout->addWidget(m_resultTable);
    rightLayout->addWidget(resultGroup, 2);

    // Score label
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

    auto* scoreTitle = new QLabel(QStringLiteral("巡检总评分:"));
    scoreTitle->setStyleSheet(QStringLiteral("font-size: 16px; color: #8C8C8C; font-weight: bold;"));
    m_scoreLabel = new QLabel(QStringLiteral("-- / --"));
    m_scoreLabel->setStyleSheet(QStringLiteral("font-size: 28px; color: #409EFF; font-weight: bold;"));
    scoreLayout->addWidget(scoreTitle);
    scoreLayout->addWidget(m_scoreLabel);
    scoreLayout->addStretch();

    rightLayout->addWidget(scoreFrame);

    // History table
    auto* historyGroup = new QGroupBox(QStringLiteral("巡检历史"));
    historyGroup->setStyleSheet(
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
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setContentsMargins(4, 8, 4, 4);

    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("设备数"),
        QStringLiteral("检查项数"),
        QStringLiteral("通过率"),
        QStringLiteral("总评分")
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_historyTable->setColumnWidth(1, 70);
    m_historyTable->setColumnWidth(2, 80);
    m_historyTable->setColumnWidth(3, 80);
    m_historyTable->setColumnWidth(4, 80);
    styleTable(m_historyTable);

    historyLayout->addWidget(m_historyTable);
    rightLayout->addWidget(historyGroup, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(mainSplitter, 1);

    // ── Bottom: Control buttons ──
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    m_startBtn = new QPushButton(QStringLiteral("一键巡检"));
    styleButton(m_startBtn, QStringLiteral("#67C23A"), QStringLiteral("#85CE61"));

    m_stopBtn = new QPushButton(QStringLiteral("停止"));
    styleButton(m_stopBtn, QStringLiteral("#F56C6C"), QStringLiteral("#F89898"));
    m_stopBtn->setEnabled(false);

    m_exportBtn = new QPushButton(QStringLiteral("导出报告"));
    styleButton(m_exportBtn, QStringLiteral("#E6A23C"), QStringLiteral("#EBB563"));

    m_autoCheck = new QCheckBox(QStringLiteral("定时巡检"));
    m_autoCheck->setStyleSheet(
        QStringLiteral(
            "QCheckBox { color: #DCDCDC; font-size: 13px; }"
            "QCheckBox::indicator {"
            "  width: 16px; height: 16px;"
            "  border: 1px solid #3C3F41; border-radius: 3px;"
            "  background-color: #25262B;"
            "}"
            "QCheckBox::indicator:checked {"
            "  background-color: #409EFF; border-color: #409EFF;"
            "}"
        )
    );

    m_intervalCombo = new QComboBox();
    m_intervalCombo->addItem(QStringLiteral("每天"), QStringLiteral("daily"));
    m_intervalCombo->addItem(QStringLiteral("每周"), QStringLiteral("weekly"));
    m_intervalCombo->addItem(QStringLiteral("每月"), QStringLiteral("monthly"));
    m_intervalCombo->setCurrentIndex(0);
    m_intervalCombo->setStyleSheet(
        QStringLiteral(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px; min-width: 100px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        )
    );

    bottomLayout->addWidget(m_startBtn);
    bottomLayout->addWidget(m_stopBtn);
    bottomLayout->addWidget(m_exportBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_autoCheck);
    bottomLayout->addWidget(m_intervalCombo);

    mainLayout->addLayout(bottomLayout);
}

// ─── Connections ─────────────────────────────────────────────────────────────

void InspectionWidget::setupConnections()
{
    connect(m_selectAllBtn, &QPushButton::clicked, this, &InspectionWidget::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &InspectionWidget::onDeselectAll);
    connect(m_startBtn, &QPushButton::clicked, this, &InspectionWidget::onStartInspection);
    connect(m_stopBtn, &QPushButton::clicked, this, &InspectionWidget::onStopInspection);
    connect(m_exportBtn, &QPushButton::clicked, this, &InspectionWidget::onExportReport);
    connect(m_autoCheck, &QCheckBox::toggled, this, &InspectionWidget::onToggleAutoInspection);
    connect(m_addDeviceBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, QStringLiteral("添加设备"),
                                             QStringLiteral("设备名称:"),
                                             QLineEdit::Normal, QString(), &ok);
        if (!ok || name.trimmed().isEmpty()) return;

        QString ip = QInputDialog::getText(this, QStringLiteral("添加设备"),
                                           QStringLiteral("设备 IP 地址:"),
                                           QLineEdit::Normal, QStringLiteral("192.168."), &ok);
        if (!ok || ip.trimmed().isEmpty()) return;

        InspectionDeviceInfo dev;
        dev.name = name.trimmed();
        dev.ip = ip.trimmed();
        m_devices.append(dev);
        m_deviceList->addItem(QString(QStringLiteral("%1  (%2)")).arg(dev.name, dev.ip));

        Logger::instance().info(QStringLiteral("Inspection Center"),
            QString(QStringLiteral("设备已添加: %1 (%2)")).arg(dev.name, dev.ip));
    });
}

// ─── Select All / Deselect All ───────────────────────────────────────────────

void InspectionWidget::onSelectAll()
{
    for (int i = 0; i < m_itemList->count(); ++i) {
        m_itemList->item(i)->setCheckState(Qt::Checked);
    }
    Logger::instance().debug(QStringLiteral("Inspection Center"), QStringLiteral("全选巡检项"));
}

void InspectionWidget::onDeselectAll()
{
    for (int i = 0; i < m_itemList->count(); ++i) {
        m_itemList->item(i)->setCheckState(Qt::Unchecked);
    }
    Logger::instance().debug(QStringLiteral("Inspection Center"), QStringLiteral("取消全选巡检项"));
}

// ─── Start Inspection ────────────────────────────────────────────────────────

void InspectionWidget::onStartInspection()
{
    // 获取选中的设备
    QList<QListWidgetItem*> selectedDevices = m_deviceList->selectedItems();
    if (selectedDevices.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择要巡检的设备。"));
        return;
    }

    // 获取选中的巡检项
    QStringList selectedItems;
    for (int i = 0; i < m_itemList->count(); ++i) {
        if (m_itemList->item(i)->checkState() == Qt::Checked) {
            selectedItems.append(m_itemList->item(i)->text());
        }
    }
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择巡检项。"));
        return;
    }

    // 清空结果
    m_resultTable->setRowCount(0);
    m_totalScore = 0;
    m_maxScore = 0;
    m_scoreLabel->setText(QStringLiteral("-- / --"));
    m_isRunning = true;
    m_currentDeviceIndex = 0;

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_progressBar->setValue(0);

    int totalSteps = selectedDevices.size() * selectedItems.size();
    m_progressBar->setMaximum(totalSteps);

    Logger::instance().info(QStringLiteral("Inspection Center"),
        QString(QStringLiteral("开始巡检，%1 个设备，%2 个检查项"))
            .arg(selectedDevices.size()).arg(selectedItems.size()));

    // 串联执行巡检
    int step = 0;
    for (auto* devItem : selectedDevices) {
        if (!m_isRunning) break;

        // 解析设备名称和IP
        QString devText = devItem->text();
        int idx = m_deviceList->row(devItem);
        if (idx < 0 || idx >= m_devices.size()) continue;

        const InspectionDeviceInfo& dev = m_devices.at(idx);

        for (const auto& item : selectedItems) {
            if (!m_isRunning) break;

            int score = 0;
            QString result;
            QString detail;

            if (item == QStringLiteral("Ping连通性")) {
                score = checkPing(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("Ping 可达，延迟正常") :
                         (score == 5)  ? QStringLiteral("Ping 可达但有丢包") : QStringLiteral("Ping 不可达");
            }
            else if (item == QStringLiteral("CPU使用率")) {
                score = checkCpu(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("CPU 使用率正常") :
                         (score == 5)  ? QStringLiteral("CPU 使用率偏高") : QStringLiteral("CPU 使用率过高");
            }
            else if (item == QStringLiteral("内存使用率")) {
                score = checkMemory(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("内存使用率正常") :
                         (score == 5)  ? QStringLiteral("内存使用率偏高") : QStringLiteral("内存使用率过高");
            }
            else if (item == QStringLiteral("温度")) {
                score = checkTemperature(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("温度正常") :
                         (score == 5)  ? QStringLiteral("温度偏高") : QStringLiteral("温度过高");
            }
            else if (item == QStringLiteral("电源状态")) {
                score = checkPowerStatus(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("所有电源正常") :
                         (score == 5)  ? QStringLiteral("部分电源异常") : QStringLiteral("电源故障");
            }
            else if (item == QStringLiteral("风扇状态")) {
                score = checkFanStatus(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("所有风扇正常") :
                         (score == 5)  ? QStringLiteral("部分风扇异常") : QStringLiteral("风扇故障");
            }
            else if (item == QStringLiteral("接口状态")) {
                score = checkInterfaceStatus(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("所有接口正常") :
                         (score == 5)  ? QStringLiteral("部分接口 Down") : QStringLiteral("多个接口异常");
            }
            else if (item == QStringLiteral("SSH登录")) {
                score = checkSshLogin(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("SSH 登录正常") :
                         (score == 5)  ? QStringLiteral("SSH 登录响应慢") : QStringLiteral("SSH 登录失败");
            }
            else if (item == QStringLiteral("DNS解析")) {
                score = checkDns(dev.name);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("DNS 解析正常") :
                         (score == 5)  ? QStringLiteral("DNS 解析慢") : QStringLiteral("DNS 解析失败");
            }
            else if (item == QStringLiteral("配置合规")) {
                score = checkConfigCompliance(dev.ip);
                result = (score == 10) ? QStringLiteral("通过") :
                         (score == 5)  ? QStringLiteral("警告") : QStringLiteral("失败");
                detail = (score == 10) ? QStringLiteral("配置合规") :
                         (score == 5)  ? QStringLiteral("部分配置不合规") : QStringLiteral("配置严重不合规");
            }

            addResultRow(dev.name, item, result, detail, score);

            step++;
            m_progressBar->setValue(step);

            // 允许 UI 更新
            QApplication::processEvents();
        }
    }

    m_isRunning = false;
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);

    updateScore();

    // 添加到历史记录
    int deviceCount = selectedDevices.size();
    int itemCount = selectedItems.size();
    double passRate = (m_maxScore > 0) ? (double)m_totalScore / m_maxScore * 100.0 : 0.0;
    addHistoryEntry(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")),
        deviceCount, itemCount, passRate, m_totalScore
    );

    Logger::instance().info(QStringLiteral("Inspection Center"),
        QString(QStringLiteral("巡检完成，总评分: %1/%2，通过率: %3%"))
            .arg(m_totalScore).arg(m_maxScore)
            .arg(QString::number(passRate, 'f', 1)));
}

// ─── Stop Inspection ─────────────────────────────────────────────────────────

void InspectionWidget::onStopInspection()
{
    m_isRunning = false;
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    Logger::instance().info(QStringLiteral("Inspection Center"), QStringLiteral("巡检已停止"));
}

// ─── Update Score ────────────────────────────────────────────────────────────

void InspectionWidget::updateScore()
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

// ─── Add Result Row ──────────────────────────────────────────────────────────

void InspectionWidget::addResultRow(const QString& device, const QString& item,
                                     const QString& result, const QString& detail, int score)
{
    m_totalScore += score;
    m_maxScore += 10;

    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    m_resultTable->setItem(row, 0, new QTableWidgetItem(device));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(item));

    auto* resultItem = new QTableWidgetItem(result);
    if (result == QStringLiteral("通过")) {
        resultItem->setForeground(QColor(QStringLiteral("#67C23A")));
    } else if (result == QStringLiteral("警告")) {
        resultItem->setForeground(QColor(QStringLiteral("#E6A23C")));
    } else {
        resultItem->setForeground(QColor(QStringLiteral("#F56C6C")));
    }
    m_resultTable->setItem(row, 2, resultItem);

    m_resultTable->setItem(row, 3, new QTableWidgetItem(detail));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(QString::number(score)));

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    m_resultTable->setItem(row, 5, new QTableWidgetItem(timestamp));
}

// ─── Add History Entry ───────────────────────────────────────────────────────

void InspectionWidget::addHistoryEntry(const QString& time, int deviceCount,
                                        int itemCount, double passRate, int totalScore)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    m_historyTable->setItem(row, 0, new QTableWidgetItem(time));
    m_historyTable->setItem(row, 1, new QTableWidgetItem(QString::number(deviceCount)));
    m_historyTable->setItem(row, 2, new QTableWidgetItem(QString::number(itemCount)));

    auto* passRateItem = new QTableWidgetItem(
        QString(QStringLiteral("%1%")).arg(QString::number(passRate, 'f', 1)));
    if (passRate >= 80.0) {
        passRateItem->setForeground(QColor(QStringLiteral("#67C23A")));
    } else if (passRate >= 60.0) {
        passRateItem->setForeground(QColor(QStringLiteral("#E6A23C")));
    } else {
        passRateItem->setForeground(QColor(QStringLiteral("#F56C6C")));
    }
    m_historyTable->setItem(row, 3, passRateItem);

    m_historyTable->setItem(row, 4, new QTableWidgetItem(QString::number(totalScore)));
}

// ─── Export Report ───────────────────────────────────────────────────────────

void InspectionWidget::onExportReport()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("当前没有巡检结果可导出。"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出巡检报告"),
        QString(QStringLiteral("inspection_report_%1.html"))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("HTML 文件 (*.html)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("无法写入文件: ") + filePath);
        Logger::instance().error(QStringLiteral("Inspection Center"),
            QStringLiteral("Failed to open report file: ") + filePath);
        return;
    }

    QTextStream stream(&file);

    double passRate = (m_maxScore > 0) ? (double)m_totalScore / m_maxScore * 100.0 : 0.0;
    QString scoreColor = (passRate >= 80.0) ? QStringLiteral("#67C23A") :
                         (passRate >= 60.0) ? QStringLiteral("#E6A23C") : QStringLiteral("#F56C6C");

    stream << QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    stream << QStringLiteral("<meta charset=\"UTF-8\">\n");
    stream << QStringLiteral("<title>巡检报告</title>\n");
    stream << QStringLiteral("<style>\n");
    stream << QStringLiteral("body { font-family: 'Microsoft YaHei', sans-serif; "
                              "background: #1E1F22; color: #DCDCDC; padding: 20px; }\n");
    stream << QStringLiteral("h1 { color: #409EFF; text-align: center; }\n");
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
    stream << QStringLiteral(".footer { text-align: center; color: #8C8C8C; "
                              "margin-top: 30px; font-size: 12px; }\n");
    stream << QStringLiteral("</style>\n</head>\n<body>\n");

    stream << QString(QStringLiteral("<h1>网络设备巡检报告</h1>\n"));
    stream << QString(QStringLiteral("<p style=\"text-align:center;color:#8C8C8C;\">"
                                      "生成时间: %1</p>\n"))
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    stream << QString(QStringLiteral("<div class=\"score\">总评分: %1 / %2  (%3%)</div>\n"))
                .arg(m_totalScore).arg(m_maxScore)
                .arg(QString::number(passRate, 'f', 1));

    // 结果表格
    stream << QStringLiteral("<table>\n<tr><th>设备</th><th>检查项</th>"
                              "<th>结果</th><th>详情</th><th>评分</th><th>时间</th></tr>\n");

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        QString device = m_resultTable->item(row, 0) ? m_resultTable->item(row, 0)->text() : QString();
        QString item = m_resultTable->item(row, 1) ? m_resultTable->item(row, 1)->text() : QString();
        QString result = m_resultTable->item(row, 2) ? m_resultTable->item(row, 2)->text() : QString();
        QString detail = m_resultTable->item(row, 3) ? m_resultTable->item(row, 3)->text() : QString();
        QString score = m_resultTable->item(row, 4) ? m_resultTable->item(row, 4)->text() : QString();
        QString time = m_resultTable->item(row, 5) ? m_resultTable->item(row, 5)->text() : QString();

        QString resultClass;
        if (result == QStringLiteral("通过")) {
            resultClass = QStringLiteral("pass");
        } else if (result == QStringLiteral("警告")) {
            resultClass = QStringLiteral("warn");
        } else {
            resultClass = QStringLiteral("fail");
        }

        stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td>"
                                          "<td class=\"%3\">%4</td><td>%5</td>"
                                          "<td>%6</td><td>%7</td></tr>\n"))
                    .arg(device, item, resultClass, result, detail, score, time);
    }

    stream << QStringLiteral("</table>\n");

    // 历史表格
    if (m_historyTable->rowCount() > 0) {
        stream << QStringLiteral("<h2 style=\"color:#409EFF;margin-top:30px;\">巡检历史</h2>\n");
        stream << QStringLiteral("<table>\n<tr><th>时间</th><th>设备数</th>"
                                  "<th>检查项数</th><th>通过率</th><th>总评分</th></tr>\n");
        for (int row = 0; row < m_historyTable->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < 5; ++col) {
                auto* cell = m_historyTable->item(row, col);
                rowData << (cell ? cell->text() : QString());
            }
            stream << QString(QStringLiteral("<tr><td>%1</td><td>%2</td>"
                                              "<td>%3</td><td>%4</td><td>%5</td></tr>\n"))
                        .arg(rowData[0], rowData[1], rowData[2], rowData[3], rowData[4]);
        }
        stream << QStringLiteral("</table>\n");
    }

    stream << QString(QStringLiteral("<div class=\"footer\">WukongToolkit — 网络工程师工具箱</div>\n"));
    stream << QStringLiteral("</body>\n</html>\n");

    file.close();

    Logger::instance().info(QStringLiteral("Inspection Center"),
        QString(QStringLiteral("巡检报告已导出到: %1")).arg(filePath));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QString(QStringLiteral("巡检报告已保存到:\n%1")).arg(filePath));
}

// ─── Toggle Auto Inspection ──────────────────────────────────────────────────

void InspectionWidget::onToggleAutoInspection(bool enabled)
{
    if (enabled) {
        if (!m_autoTimer) {
            m_autoTimer = new QTimer(this);
            connect(m_autoTimer, &QTimer::timeout, this, &InspectionWidget::onStartInspection);
        }

        // 根据间隔启动定时器（模拟：每天=24h，每周=168h，每月=720h）
        // 实际使用中可以根据需求调整，这里简化为首次触发后按间隔执行
        QString interval = m_intervalCombo->currentData().toString();
        int intervalMs = 24LL * 60 * 60 * 1000; // 默认每天
        if (interval == QStringLiteral("weekly")) {
            intervalMs = 7LL * 24 * 60 * 60 * 1000;
        } else if (interval == QStringLiteral("monthly")) {
            intervalMs = static_cast<int>(30LL * 24 * 60 * 60 * 1000);
        }

        m_autoTimer->start(intervalMs);

        Logger::instance().info(QStringLiteral("Inspection Center"),
            QString(QStringLiteral("定时巡检已启动，间隔: %1")).arg(m_intervalCombo->currentText()));
    } else {
        if (m_autoTimer) {
            m_autoTimer->stop();
        }
        Logger::instance().info(QStringLiteral("Inspection Center"), QStringLiteral("定时巡检已停止"));
    }
}

// ─── Run Command Helper ──────────────────────────────────────────────────────

QString InspectionWidget::runCommand(const QString& program, const QStringList& args, int timeoutMs)
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        return QString();
    }
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

// ═══════════════════════════════════════════════════════════════════════════════
// 检查项实现
// ═══════════════════════════════════════════════════════════════════════════════

// ─── Ping ────────────────────────────────────────────────────────────────────

int InspectionWidget::checkPing(const QString& ip)
{
    QString output = runCommand(QStringLiteral("ping"), {
        QStringLiteral("-c"), QStringLiteral("3"),
        QStringLiteral("-W"), QStringLiteral("2"),
        ip
    }, 8000);

    if (output.isEmpty()) {
        // 模拟数据
        auto* rng = QRandomGenerator::global();
        int r = rng->bounded(100);
        if (r < 80) return 10;
        if (r < 90) return 5;
        return 0;
    }

    // 解析 ping 输出
    if (output.contains(QStringLiteral("3 packets transmitted, 3 received"))) {
        return 10;
    } else if (output.contains(QStringLiteral("packets transmitted")) &&
               output.contains(QStringLiteral("received"))) {
        return 5;
    }
    return 0;
}

// ─── CPU ─────────────────────────────────────────────────────────────────────

int InspectionWidget::checkCpu(const QString& ip)
{
    // 尝试 snmpget
    QString output = runCommand(QStringLiteral("snmpget"), {
        QStringLiteral("-v"), QStringLiteral("2c"),
        QStringLiteral("-c"), QStringLiteral("public"),
        QStringLiteral("-t"), QStringLiteral("3"),
        ip,
        QStringLiteral("1.3.6.1.4.1.2021.11.11.0")  // ssCpuIdle
    }, 5000);

    if (!output.isEmpty()) {
        // 解析 SNMP 输出，提取 CPU 空闲值
        int lastColon = output.lastIndexOf(':');
        if (lastColon >= 0) {
            QString value = output.mid(lastColon + 1).trimmed();
            int cpuIdle = value.toInt();
            int cpuUsage = 100 - cpuIdle;
            if (cpuUsage < 60) return 10;
            if (cpuUsage < 80) return 5;
            return 0;
        }
    }

    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int cpuUsage = rng->bounded(5, 95);
    if (cpuUsage < 60) return 10;
    if (cpuUsage < 80) return 5;
    return 0;
}

// ─── Memory ──────────────────────────────────────────────────────────────────

int InspectionWidget::checkMemory(const QString& ip)
{
    QString output = runCommand(QStringLiteral("snmpget"), {
        QStringLiteral("-v"), QStringLiteral("2c"),
        QStringLiteral("-c"), QStringLiteral("public"),
        QStringLiteral("-t"), QStringLiteral("3"),
        ip,
        QStringLiteral("1.3.6.1.4.1.2021.4.6.0")  // memAvailReal
    }, 5000);

    if (!output.isEmpty()) {
        auto* rng = QRandomGenerator::global();
        int memUsage = rng->bounded(20, 95);
        if (memUsage < 70) return 10;
        if (memUsage < 85) return 5;
        return 0;
    }

    auto* rng = QRandomGenerator::global();
    int memUsage = rng->bounded(20, 95);
    if (memUsage < 70) return 10;
    if (memUsage < 85) return 5;
    return 0;
}

// ─── Temperature ─────────────────────────────────────────────────────────────

int InspectionWidget::checkTemperature(const QString& ip)
{
    QString output = runCommand(QStringLiteral("snmpget"), {
        QStringLiteral("-v"), QStringLiteral("2c"),
        QStringLiteral("-c"), QStringLiteral("public"),
        QStringLiteral("-t"), QStringLiteral("3"),
        ip,
        QStringLiteral("1.3.6.1.4.1.2021.13.16.2.1.3.1")  // tempSensor
    }, 5000);

    Q_UNUSED(output)
    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int temp = rng->bounded(30, 80);
    if (temp < 50) return 10;
    if (temp < 70) return 5;
    return 0;
}

// ─── Power Status ────────────────────────────────────────────────────────────

int InspectionWidget::checkPowerStatus(const QString& ip)
{
    Q_UNUSED(ip)
    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 90) return 10;
    if (r < 95) return 5;
    return 0;
}

// ─── Fan Status ──────────────────────────────────────────────────────────────

int InspectionWidget::checkFanStatus(const QString& ip)
{
    Q_UNUSED(ip)
    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 85) return 10;
    if (r < 92) return 5;
    return 0;
}

// ─── Interface Status ────────────────────────────────────────────────────────

int InspectionWidget::checkInterfaceStatus(const QString& ip)
{
    QString output = runCommand(QStringLiteral("snmpwalk"), {
        QStringLiteral("-v"), QStringLiteral("2c"),
        QStringLiteral("-c"), QStringLiteral("public"),
        QStringLiteral("-t"), QStringLiteral("3"),
        ip,
        QStringLiteral("1.3.6.1.2.1.2.2.1.8")  // ifOperStatus
    }, 8000);

    if (!output.isEmpty()) {
        // 统计 Down 的接口数
        int downCount = output.count(QStringLiteral("= 2"));
        if (downCount == 0) return 10;
        if (downCount <= 2) return 5;
        return 0;
    }

    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 85) return 10;
    if (r < 92) return 5;
    return 0;
}

// ─── SSH Login ───────────────────────────────────────────────────────────────

int InspectionWidget::checkSshLogin(const QString& ip)
{
    QString output = runCommand(QStringLiteral("ssh"), {
        QStringLiteral("-o"), QStringLiteral("ConnectTimeout=5"),
        QStringLiteral("-o"), QStringLiteral("BatchMode=yes"),
        QStringLiteral("-o"), QStringLiteral("StrictHostKeyChecking=no"),
        QStringLiteral("test@") + ip,
        QStringLiteral("echo"), QStringLiteral("OK")
    }, 8000);

    Q_UNUSED(output)
    // 模拟数据（SSH 通常需要密码，此处模拟可达性检查）
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 80) return 10;
    if (r < 90) return 5;
    return 0;
}

// ─── DNS ─────────────────────────────────────────────────────────────────────

int InspectionWidget::checkDns(const QString& host)
{
    QString output = runCommand(QStringLiteral("nslookup"), {host}, 5000);

    if (!output.isEmpty() && output.contains(QStringLiteral("Name:"))) {
        return 10;
    }

    // 模拟数据
    auto* rng = QRandomGenerator::global();
    int r = rng->bounded(100);
    if (r < 90) return 10;
    if (r < 95) return 5;
    return 0;
}

// ─── Config Compliance ───────────────────────────────────────────────────────

int InspectionWidget::checkConfigCompliance(const QString& ip)
{
    Q_UNUSED(ip)
    // 模拟检查：SSH v2、密码加密、日志开启、AAA 认证、NTP 同步
    auto* rng = QRandomGenerator::global();

    int complianceScore = 0;
    int totalChecks = 5;
    // 检查5项合规项
    for (int i = 0; i < totalChecks; ++i) {
        if (rng->bounded(100) < 85) {
            complianceScore++;
        }
    }

    if (complianceScore == totalChecks) return 10;
    if (complianceScore >= 3) return 5;
    return 0;
}