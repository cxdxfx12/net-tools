#include "modbus/ModbusWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// ModbusWidget 实现
// ============================================================================

ModbusWidget::ModbusWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
{
    setupUI();
    setupConnections();
}

ModbusWidget::~ModbusWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void ModbusWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 连接区 ──
    auto* connGroup = new QGroupBox("Modbus 连接");
    auto* connLayout = new QHBoxLayout(connGroup);
    connLayout->setSpacing(8);

    auto* modeLabel = new QLabel("模式:");
    
    m_modeCombo = new QComboBox();
    m_modeCombo->addItems({"TCP", "RTU", "ASCII"});
    connLayout->addWidget(modeLabel);
    connLayout->addWidget(m_modeCombo);

    auto* hostLabel = new QLabel("主机:");
    
    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("192.168.1.100");
    m_hostEdit->setMinimumWidth(140);
    connLayout->addWidget(hostLabel);
    connLayout->addWidget(m_hostEdit, 1);

    auto* portLabel = new QLabel("端口:");
    
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(502);
    m_portSpin->setFixedWidth(80);
    connLayout->addWidget(portLabel);
    connLayout->addWidget(m_portSpin);

    auto* deviceLabel = new QLabel("串口:");
    
    m_deviceEdit = new QLineEdit();
    m_deviceEdit->setPlaceholderText("/dev/ttyUSB0");
    m_deviceEdit->setMinimumWidth(120);
    m_deviceEdit->setEnabled(false); // 默认 TCP 模式
    connLayout->addWidget(deviceLabel);
    connLayout->addWidget(m_deviceEdit, 1);

    auto* slaveLabel = new QLabel("从站ID:");
    
    m_slaveIdSpin = new QSpinBox();
    m_slaveIdSpin->setRange(0, 247);
    m_slaveIdSpin->setValue(1);
    m_slaveIdSpin->setFixedWidth(70);
    connLayout->addWidget(slaveLabel);
    connLayout->addWidget(m_slaveIdSpin);

    mainLayout->addWidget(connGroup);

    // ── 状态标签 ──
    m_statusLabel = new QLabel("就绪 - Modbus 协议调试工具");
    
    mainLayout->addWidget(m_statusLabel);

    // ── 读取区 ──
    auto* readGroup = new QGroupBox("读取操作 (Read)");
    auto* readLayout = new QVBoxLayout(readGroup);
    readLayout->setSpacing(6);

    auto* readParamRow = new QHBoxLayout();
    readParamRow->setSpacing(8);

    auto* readAddrLabel = new QLabel("起始地址:");
    
    m_readAddrSpin = new QSpinBox();
    m_readAddrSpin->setRange(0, 65535);
    m_readAddrSpin->setValue(0);
    m_readAddrSpin->setFixedWidth(90);
    readParamRow->addWidget(readAddrLabel);
    readParamRow->addWidget(m_readAddrSpin);

    auto* readCountLabel = new QLabel("数量:");
    
    m_readCountSpin = new QSpinBox();
    m_readCountSpin->setRange(1, 125);
    m_readCountSpin->setValue(10);
    m_readCountSpin->setFixedWidth(80);
    readParamRow->addWidget(readCountLabel);
    readParamRow->addWidget(m_readCountSpin);

    readParamRow->addSpacing(20);

    m_readCoilsBtn = new QPushButton("读线圈 (01)");
    readParamRow->addWidget(m_readCoilsBtn);

    m_readDiscreteBtn = new QPushButton("读离散输入 (02)");
    readParamRow->addWidget(m_readDiscreteBtn);

    m_readHoldingBtn = new QPushButton("读保持寄存器 (03)");
    readParamRow->addWidget(m_readHoldingBtn);

    m_readInputBtn = new QPushButton("读输入寄存器 (04)");
    readParamRow->addWidget(m_readInputBtn);

    readLayout->addLayout(readParamRow);
    mainLayout->addWidget(readGroup);

    // ── 写入区 ──
    auto* writeGroup = new QGroupBox("写入操作 (Write)");
    auto* writeLayout = new QVBoxLayout(writeGroup);
    writeLayout->setSpacing(6);

    auto* writeParamRow = new QHBoxLayout();
    writeParamRow->setSpacing(8);

    auto* writeAddrLabel = new QLabel("地址:");
    
    m_writeAddrSpin = new QSpinBox();
    m_writeAddrSpin->setRange(0, 65535);
    m_writeAddrSpin->setValue(0);
    m_writeAddrSpin->setFixedWidth(90);
    writeParamRow->addWidget(writeAddrLabel);
    writeParamRow->addWidget(m_writeAddrSpin);

    auto* writeValueLabel = new QLabel("值:");
    
    m_writeValueEdit = new QLineEdit();
    m_writeValueEdit->setPlaceholderText("0 或 1 (线圈); 0-65535 (寄存器); 多个值用空格分隔");
    writeParamRow->addWidget(writeValueLabel);
    writeParamRow->addWidget(m_writeValueEdit, 1);

    writeParamRow->addSpacing(10);

    m_writeCoilBtn = new QPushButton("写线圈 (05)");
    writeParamRow->addWidget(m_writeCoilBtn);

    m_writeRegBtn = new QPushButton("写寄存器 (06)");
    writeParamRow->addWidget(m_writeRegBtn);

    m_writeMultiRegBtn = new QPushButton("写多寄存器 (16)");
    writeParamRow->addWidget(m_writeMultiRegBtn);

    writeLayout->addLayout(writeParamRow);
    mainLayout->addWidget(writeGroup);

    // ── 结果区 ──
    auto* resultGroup = new QGroupBox("结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);
    resultLayout->setSpacing(4);

    // 工具栏
    auto* resultToolbar = new QHBoxLayout();
    resultToolbar->setSpacing(8);
    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );
    resultToolbar->addWidget(m_exportBtn);
    resultToolbar->addStretch();
    m_clearOutputBtn = new QPushButton("清空");
    m_clearOutputBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    resultToolbar->addWidget(m_clearOutputBtn);
    resultLayout->addLayout(resultToolbar);

    // 数据表格
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(3);
    m_resultTable->setHorizontalHeaderLabels({"地址", "十进制", "十六进制"});
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_resultTable->verticalHeader()->setVisible(false);
    resultLayout->addWidget(m_resultTable, 1);

    // 原始输出
    m_outputText = new QPlainTextEdit();
    m_outputText->setReadOnly(true);
    m_outputText->setMaximumBlockCount(3000);
    m_outputText->setMaximumHeight(120);
    resultLayout->addWidget(m_outputText);

    mainLayout->addWidget(resultGroup, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void ModbusWidget::setupConnections()
{
    // 模式切换
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        bool isTcp = (index == 0);
        m_hostEdit->setEnabled(isTcp);
        m_portSpin->setEnabled(isTcp);
        m_deviceEdit->setEnabled(!isTcp);
        if (isTcp) {
            m_portSpin->setValue(502);
        }
    });

    // 读取按钮
    connect(m_readCoilsBtn, &QPushButton::clicked, this, &ModbusWidget::onReadCoils);
    connect(m_readDiscreteBtn, &QPushButton::clicked, this, &ModbusWidget::onReadDiscreteInputs);
    connect(m_readHoldingBtn, &QPushButton::clicked, this, &ModbusWidget::onReadHoldingRegisters);
    connect(m_readInputBtn, &QPushButton::clicked, this, &ModbusWidget::onReadInputRegisters);

    // 写入按钮
    connect(m_writeCoilBtn, &QPushButton::clicked, this, &ModbusWidget::onWriteSingleCoil);
    connect(m_writeRegBtn, &QPushButton::clicked, this, &ModbusWidget::onWriteSingleRegister);
    connect(m_writeMultiRegBtn, &QPushButton::clicked, this, &ModbusWidget::onWriteMultipleRegisters);

    // 清空/导出
    connect(m_clearOutputBtn, &QPushButton::clicked, this, &ModbusWidget::onClearOutput);
    connect(m_exportBtn, &QPushButton::clicked, this, &ModbusWidget::onExportResults);
}

// ─── Output Helper ─────────────────────────────────────────────────────

void ModbusWidget::appendOutput(const QString& text, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m_outputText->appendHtml(
        QString("<span style='color:#8C8C8C;'>[%1]</span> "
                "<span style='color:%2;'>%3</span>")
            .arg(timestamp, color, text.toHtmlEscaped()));
    QScrollBar* bar = m_outputText->verticalScrollBar();
    bar->setValue(bar->maximum());
}

// ─── Execute Modbus Command ────────────────────────────────────────────

void ModbusWidget::executeModbusCmd(const QStringList& args, const QString& desc)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ModbusWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ModbusWidget::onProcessFinished);

    // 使用 mbpoll 命令行工具
    m_process->start("mbpoll", args);
    appendOutput(desc, "#58A6FF");
    m_statusLabel->setText("执行中: " + desc);
    
}

// ─── Read Operations ───────────────────────────────────────────────────

void ModbusWidget::onReadCoils()
{
    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_readAddrSpin->value()),
                         "-c", QString::number(m_readCountSpin->value())};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("读线圈 起始=%1 数量=%2 从站=%3")
                            .arg(m_readAddrSpin->value())
                            .arg(m_readCountSpin->value())
                            .arg(m_slaveIdSpin->value()));
}

void ModbusWidget::onReadDiscreteInputs()
{
    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_readAddrSpin->value()),
                         "-c", QString::number(m_readCountSpin->value()),
                         "-t", "1"};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("读离散输入 起始=%1 数量=%2")
                            .arg(m_readAddrSpin->value())
                            .arg(m_readCountSpin->value()));
}

void ModbusWidget::onReadHoldingRegisters()
{
    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_readAddrSpin->value()),
                         "-c", QString::number(m_readCountSpin->value()),
                         "-t", "4"};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("读保持寄存器 起始=%1 数量=%2")
                            .arg(m_readAddrSpin->value())
                            .arg(m_readCountSpin->value()));
}

void ModbusWidget::onReadInputRegisters()
{
    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_readAddrSpin->value()),
                         "-c", QString::number(m_readCountSpin->value()),
                         "-t", "3"};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("读输入寄存器 起始=%1 数量=%2")
                            .arg(m_readAddrSpin->value())
                            .arg(m_readCountSpin->value()));
}

// ─── Write Operations ──────────────────────────────────────────────────

void ModbusWidget::onWriteSingleCoil()
{
    QString value = m_writeValueEdit->text().trimmed();
    if (value.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入线圈值 (0 或 1)。");
        return;
    }

    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_writeAddrSpin->value()),
                         "-t", "0", value};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("写线圈 地址=%1 值=%2")
                            .arg(m_writeAddrSpin->value()).arg(value));
    Logger::instance().info("Modbus", QString("写线圈 地址=%1 值=%2")
                                        .arg(m_writeAddrSpin->value()).arg(value));
}

void ModbusWidget::onWriteSingleRegister()
{
    QString value = m_writeValueEdit->text().trimmed();
    if (value.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入寄存器值 (0-65535)。");
        return;
    }

    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_writeAddrSpin->value()),
                         "-t", "4", value};
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("写寄存器 地址=%1 值=%2")
                            .arg(m_writeAddrSpin->value()).arg(value));
    Logger::instance().info("Modbus", QString("写寄存器 地址=%1 值=%2")
                                        .arg(m_writeAddrSpin->value()).arg(value));
}

void ModbusWidget::onWriteMultipleRegisters()
{
    QString valuesStr = m_writeValueEdit->text().trimmed();
    if (valuesStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入要写入的值，多个值用空格分隔。");
        return;
    }

    QStringList values = valuesStr.split(' ', Qt::SkipEmptyParts);
    QStringList args = {"-1", "-a", QString::number(m_slaveIdSpin->value()),
                         "-r", QString::number(m_writeAddrSpin->value()),
                         "-t", "4"};
    args << values;
    if (m_modeCombo->currentIndex() == 0) {
        args << m_hostEdit->text().trimmed() << "-p" << QString::number(m_portSpin->value());
    }
    executeModbusCmd(args, QString("写多寄存器 起始=%1 数量=%2")
                            .arg(m_writeAddrSpin->value()).arg(values.size()));
    Logger::instance().info("Modbus", QString("写多寄存器 起始=%1 数量=%2")
                                        .arg(m_writeAddrSpin->value()).arg(values.size()));
}

// ─── Slot: Process Ready Read ──────────────────────────────────────────

void ModbusWidget::onProcessReadyRead()
{
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardOutput()).trimmed();
    if (!output.isEmpty()) {
        appendOutput(output, "#E6EDF3");
    }
}

// ─── Slot: Process Finished ────────────────────────────────────────────

void ModbusWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    if (!m_process) return;

    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    QString err = QString::fromUtf8(m_process->readAllStandardError()).trimmed();

    if (exitCode == 0 && !output.isEmpty()) {
        populateResultsTable(output, "");
        m_statusLabel->setText("操作成功");
        
    } else {
        if (!err.isEmpty()) {
            appendOutput("错误: " + err, "#F85149");
        }
        m_statusLabel->setText("操作失败");
        
    }

    m_process->deleteLater();
    m_process = nullptr;
}

// ─── Populate Results Table ────────────────────────────────────────────

void ModbusWidget::populateResultsTable(const QString& output, const QString& funcDesc)
{
    Q_UNUSED(funcDesc)
    m_resultTable->setRowCount(0);

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    int addr = m_readAddrSpin->value();

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("mbpoll")) continue;

        // 尝试解析 "地址: 值" 或 "[地址]: 值" 格式
        // 也尝试直接解析数字
        QStringList parts = trimmed.split(QRegularExpression("[\\[\\]:,\\s]+"), Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            bool ok = false;
            int value = part.toInt(&ok);
            if (ok && value >= 0 && value <= 65535) {
                int row = m_resultTable->rowCount();
                m_resultTable->insertRow(row);

                auto* addrItem = new QTableWidgetItem(QString::number(addr++));
                addrItem->setTextAlignment(Qt::AlignCenter);
                m_resultTable->setItem(row, 0, addrItem);

                auto* decItem = new QTableWidgetItem(QString::number(value));
                decItem->setTextAlignment(Qt::AlignCenter);
                m_resultTable->setItem(row, 1, decItem);

                auto* hexItem = new QTableWidgetItem(
                    QString("0x%1").arg(value, 4, 16, QChar('0')).toUpper());
                hexItem->setTextAlignment(Qt::AlignCenter);
                m_resultTable->setItem(row, 2, hexItem);
            }
        }
    }
}

// ─── Slot: Clear Output ────────────────────────────────────────────────

void ModbusWidget::onClearOutput()
{
    m_outputText->clear();
    m_resultTable->setRowCount(0);
}

// ─── Slot: Export Results ──────────────────────────────────────────────

void ModbusWidget::onExportResults()
{
    if (m_resultTable->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出 Modbus 数据",
                                                     "modbus_data.csv",
                                                     "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法打开文件: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "\xEF\xBB\xBF"; // BOM
    stream << "地址,十进制,十六进制\n";

    for (int row = 0; row < m_resultTable->rowCount(); ++row) {
        for (int col = 0; col < m_resultTable->columnCount(); ++col) {
            if (col > 0) stream << ",";
            auto* item = m_resultTable->item(row, col);
            stream << (item ? item->text() : "");
        }
        stream << "\n";
    }
    file.close();

    Logger::instance().info("Modbus", "导出数据: " + filePath);
    QMessageBox::information(this, "导出成功",
                              QString("已导出 %1 条记录到:\n%2")
                                  .arg(m_resultTable->rowCount()).arg(filePath));
}
