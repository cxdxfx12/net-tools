#include "serial/SerialWidget.h"
#include "terminal/TerminalWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFileInfo>

// ============================================================================
// SerialWidget 实现
// ============================================================================

SerialWidget::SerialWidget(QWidget* parent)
    : QWidget(parent)
    , m_serialPort(new QSerialPort(this))
    , m_pollTimer(new QTimer(this))
    , m_portOpen(false)
{
    setupUI();
    setupConnections();

    // 定期刷新可用端口列表
    m_pollTimer->setInterval(2000);
    m_pollTimer->start();
}

SerialWidget::~SerialWidget()
{
    m_pollTimer->stop();
    if (m_portOpen) {
        closePort();
    }
}

bool SerialWidget::isOpen() const
{
    return m_portOpen;
}

// ============================================================================
// UI Setup
// ============================================================================

void SerialWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 串口配置面板 ──
    auto* configGroup = new QGroupBox("串口配置");
    auto* configLayout = new QHBoxLayout(configGroup);
    configLayout->setSpacing(10);

    auto* leftForm = new QFormLayout();
    leftForm->setSpacing(4);

    // 端口选择
    m_portCombo = new QComboBox();
    m_portCombo->setMinimumWidth(160);
    m_portCombo->setEditable(true);
    m_portCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    leftForm->addRow("端口:", m_portCombo);

    // 波特率
    m_baudRateCombo = new QComboBox();
    m_baudRateCombo->setEditable(true);
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200",
                             "230400", "460800", "921600"};
    m_baudRateCombo->addItems(baudRates);
    m_baudRateCombo->setCurrentText("115200");
    m_baudRateCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    leftForm->addRow("波特率:", m_baudRateCombo);

    // 数据位
    m_dataBitsCombo = new QComboBox();
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    m_dataBitsCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    leftForm->addRow("数据位:", m_dataBitsCombo);

    auto* rightForm = new QFormLayout();
    rightForm->setSpacing(4);

    // 停止位
    m_stopBitsCombo = new QComboBox();
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    m_stopBitsCombo->setCurrentText("1");
    m_stopBitsCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    rightForm->addRow("停止位:", m_stopBitsCombo);

    // 校验位
    m_parityCombo = new QComboBox();
    m_parityCombo->addItems({"None", "Even", "Odd", "Mark", "Space"});
    m_parityCombo->setCurrentText("None");
    m_parityCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    rightForm->addRow("校验位:", m_parityCombo);

    // 流控
    m_flowControlCombo = new QComboBox();
    m_flowControlCombo->addItems({"None", "RTS/CTS", "XON/XOFF"});
    m_flowControlCombo->setCurrentText("None");
    m_flowControlCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    rightForm->addRow("流控:", m_flowControlCombo);

    // 编码
    m_encodingCombo = new QComboBox();
    m_encodingCombo->addItems({"UTF-8", "GBK", "GB2312", "Big5", "ISO-8859-1",
                               "Shift-JIS", "EUC-JP", "EUC-KR", "ASCII", "Latin-1"});
    m_encodingCombo->setCurrentText("UTF-8");
    m_encodingCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #3C3F41; border: 1px solid #3C3F41;"
        "}"
    );
    rightForm->addRow("编码:", m_encodingCombo);

    configLayout->addLayout(leftForm);
    configLayout->addLayout(rightForm);

    // ── 按钮列 ──
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(6);

    m_openBtn = new QPushButton("打开串口");
    m_openBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_openBtn->setFixedHeight(36);

    m_refreshBtn = new QPushButton("刷新端口");
    m_refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3C3F41; color: #DCDCDC;"
        "  border: 1px solid #555; padding: 6px 14px; border-radius: 4px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #4A4D50; }"
    );
    m_refreshBtn->setFixedHeight(30);

    m_sendFileBtn = new QPushButton("发送文件...");
    m_sendFileBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3C3F41; color: #DCDCDC;"
        "  border: 1px solid #555; padding: 6px 14px; border-radius: 4px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #4A4D50; }"
        "QPushButton:disabled { background-color: #2B2D30; color: #666; }"
    );
    m_sendFileBtn->setFixedHeight(30);
    m_sendFileBtn->setEnabled(false);

    btnLayout->addWidget(m_openBtn);
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addWidget(m_sendFileBtn);
    btnLayout->addStretch();

    configLayout->addLayout(btnLayout);
    mainLayout->addWidget(configGroup);

    // ── DTR / RTS 控制 ──
    auto* ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(16);

    m_dtrCheck = new QCheckBox("DTR");
    m_dtrCheck->setStyleSheet(
        "QCheckBox { color: #DCDCDC; font-size: 12px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
    m_dtrCheck->setEnabled(false);

    m_rtsCheck = new QCheckBox("RTS");
    m_rtsCheck->setStyleSheet(
        "QCheckBox { color: #DCDCDC; font-size: 12px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
    m_rtsCheck->setEnabled(false);

    // 状态指示器
    m_statusIcon = new QLabel();
    m_statusIcon->setFixedSize(12, 12);
    m_statusIcon->setStyleSheet(
        "QLabel {"
        "  background-color: #F56C6C; border-radius: 6px;"
        "  border: 1px solid #444;"
        "}"
    );

    m_statusLabel = new QLabel("未连接");
    m_statusLabel->setStyleSheet(
        "QLabel { color: #8C8C8C; font-size: 12px; }"
    );

    ctrlLayout->addWidget(m_statusIcon);
    ctrlLayout->addWidget(m_statusLabel);
    ctrlLayout->addSpacing(20);
    ctrlLayout->addWidget(m_dtrCheck);
    ctrlLayout->addWidget(m_rtsCheck);
    ctrlLayout->addStretch();

    mainLayout->addLayout(ctrlLayout);

    // ── 终端显示区域 ──
    m_terminal = new TerminalWidget();
    m_terminal->setMinimumHeight(200);
    m_terminal->setStyleSheet(
        "TerminalWidget {"
        "  background-color: #1E1F22; border: 1px solid #3C3F41;"
        "}"
    );
    mainLayout->addWidget(m_terminal, 1);

    // 初始化端口列表
    refreshPortList();
}

// ============================================================================
// Connections
// ============================================================================

void SerialWidget::setupConnections()
{
    connect(m_openBtn, &QPushButton::clicked, this, &SerialWidget::onOpenClose);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SerialWidget::onRefreshPorts);
    connect(m_sendFileBtn, &QPushButton::clicked, this, &SerialWidget::onSendFile);
    connect(m_dtrCheck, &QCheckBox::toggled, this, &SerialWidget::onDtrToggled);
    connect(m_rtsCheck, &QCheckBox::toggled, this, &SerialWidget::onRtsToggled);

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWidget::onDataReceived);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWidget::onSerialError);

    connect(m_terminal, &TerminalWidget::dataReady, this, &SerialWidget::onTerminalDataReady);

    connect(m_pollTimer, &QTimer::timeout, this, &SerialWidget::pollPorts);
}

// ============================================================================
// 端口检测与刷新
// ============================================================================

QStringList SerialWidget::detectPorts()
{
    QStringList ports;
    const auto availablePorts = QSerialPortInfo::availablePorts();
    for (const auto& info : availablePorts) {
        QString desc = info.portName();
        if (!info.description().isEmpty()) {
            desc += " (" + info.description() + ")";
        }
        if (!info.manufacturer().isEmpty()) {
            desc += " [" + info.manufacturer() + "]";
        }
        ports.append(desc);
    }
    ports.sort();
    return ports;
}

void SerialWidget::refreshPortList()
{
    QString currentText = m_portCombo->currentText();
    m_portCombo->clear();

    QStringList ports = detectPorts();
    if (ports.isEmpty()) {
        m_portCombo->addItem("(未检测到串口)");
    } else {
        m_portCombo->addItems(ports);
    }

    // 尝试恢复之前的选择
    if (!currentText.isEmpty()) {
        int idx = m_portCombo->findText(currentText, Qt::MatchStartsWith);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
        }
    }
}

void SerialWidget::pollPorts()
{
    if (m_portOpen) return; // 不干扰已打开的串口

    QStringList currentList = detectPorts();
    QStringList displayedList;
    for (int i = 0; i < m_portCombo->count(); ++i) {
        QString item = m_portCombo->itemText(i);
        if (item != "(未检测到串口)") {
            displayedList.append(item);
        }
    }

    if (currentList != displayedList) {
        refreshPortList();
        Logger::instance().debug("Serial", "端口列表已更新");
    }
}

void SerialWidget::onRefreshPorts()
{
    refreshPortList();
    Logger::instance().info("Serial", "手动刷新端口列表");
}

// ============================================================================
// 打开 / 关闭
// ============================================================================

void SerialWidget::onOpenClose()
{
    if (m_portOpen) {
        closePort();
    } else {
        openPort();
    }
}

void SerialWidget::openPort()
{
    QString portText = m_portCombo->currentText().trimmed();
    if (portText.isEmpty() || portText == "(未检测到串口)") {
        QMessageBox::warning(this, "错误", "请选择有效的串口。");
        return;
    }

    // 提取纯端口名 (去掉描述和制造商信息)
    QString portName = portText.split(" (").first().trimmed();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "错误", "无法解析端口名称。");
        return;
    }

    m_serialPort->setPortName(portName);
    applySettings();

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString errMsg = m_serialPort->errorString();
        Logger::instance().error("Serial", "打开串口失败: " + portName + " - " + errMsg);
        QMessageBox::critical(this, "打开失败",
                              "无法打开串口 " + portName + "\n" + errMsg);
        emit errorOccurred(errMsg);
        return;
    }

    m_portOpen = true;
    updatePortStatus();

    Logger::instance().info("Serial",
                            QString("串口已打开: %1 (%2, %3, %4, %5, %6)")
                                .arg(portName)
                                .arg(m_baudRateCombo->currentText())
                                .arg(m_dataBitsCombo->currentText())
                                .arg(m_stopBitsCombo->currentText())
                                .arg(m_parityCombo->currentText())
                                .arg(m_flowControlCombo->currentText()));

    m_terminal->clearTerminal();
    m_terminal->setConnected(true);
    m_terminal->setEncoding(m_encodingCombo->currentText());

    emit portOpened(portName);
}

void SerialWidget::closePort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_portOpen = false;
    updatePortStatus();

    Logger::instance().info("Serial", "串口已关闭");

    m_terminal->setConnected(false);

    emit portClosed();
}

void SerialWidget::applySettings()
{
    // 波特率
    bool ok = false;
    int baudRate = m_baudRateCombo->currentText().toInt(&ok);
    if (ok) {
        m_serialPort->setBaudRate(baudRate);
    }

    // 数据位
    int dataBits = m_dataBitsCombo->currentText().toInt();
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));

    // 停止位
    QString stopStr = m_stopBitsCombo->currentText();
    if (stopStr == "1.5") {
        m_serialPort->setStopBits(QSerialPort::OneAndHalfStop);
    } else if (stopStr == "2") {
        m_serialPort->setStopBits(QSerialPort::TwoStop);
    } else {
        m_serialPort->setStopBits(QSerialPort::OneStop);
    }

    // 校验位
    QString parityStr = m_parityCombo->currentText();
    if (parityStr == "Even") {
        m_serialPort->setParity(QSerialPort::EvenParity);
    } else if (parityStr == "Odd") {
        m_serialPort->setParity(QSerialPort::OddParity);
    } else if (parityStr == "Mark") {
        m_serialPort->setParity(QSerialPort::MarkParity);
    } else if (parityStr == "Space") {
        m_serialPort->setParity(QSerialPort::SpaceParity);
    } else {
        m_serialPort->setParity(QSerialPort::NoParity);
    }

    // 流控
    QString flowStr = m_flowControlCombo->currentText();
    if (flowStr == "RTS/CTS") {
        m_serialPort->setFlowControl(QSerialPort::HardwareControl);
    } else if (flowStr == "XON/XOFF") {
        m_serialPort->setFlowControl(QSerialPort::SoftwareControl);
    } else {
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    }
}

void SerialWidget::updatePortStatus()
{
    // 更新按钮
    m_openBtn->setText(m_portOpen ? "关闭串口" : "打开串口");
    m_openBtn->setStyleSheet(m_portOpen ?
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #F78989; }" :
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
    );

    // 更新状态指示器
    if (m_portOpen) {
        m_statusIcon->setStyleSheet(
            "QLabel {"
            "  background-color: #67C23A; border-radius: 6px;"
            "  border: 1px solid #444;"
            "}"
        );
        m_statusLabel->setText("已连接 - " + m_serialPort->portName());
        m_statusLabel->setStyleSheet("QLabel { color: #67C23A; font-size: 12px; font-weight: bold; }");
    } else {
        m_statusIcon->setStyleSheet(
            "QLabel {"
            "  background-color: #F56C6C; border-radius: 6px;"
            "  border: 1px solid #444;"
            "}"
        );
        m_statusLabel->setText("未连接");
        m_statusLabel->setStyleSheet("QLabel { color: #8C8C8C; font-size: 12px; }");
    }

    // 更新控件可用性
    m_portCombo->setEnabled(!m_portOpen);
    m_baudRateCombo->setEnabled(!m_portOpen);
    m_dataBitsCombo->setEnabled(!m_portOpen);
    m_stopBitsCombo->setEnabled(!m_portOpen);
    m_parityCombo->setEnabled(!m_portOpen);
    m_flowControlCombo->setEnabled(!m_portOpen);
    m_encodingCombo->setEnabled(!m_portOpen);
    m_refreshBtn->setEnabled(!m_portOpen);
    m_sendFileBtn->setEnabled(m_portOpen);
    m_dtrCheck->setEnabled(m_portOpen);
    m_rtsCheck->setEnabled(m_portOpen);

    // 同步 DTR/RTS 复选框状态
    if (m_portOpen) {
        m_dtrCheck->blockSignals(true);
        m_rtsCheck->blockSignals(true);
        m_dtrCheck->setChecked(m_serialPort->isDataTerminalReady());
        m_rtsCheck->setChecked(m_serialPort->isRequestToSend());
        m_dtrCheck->blockSignals(false);
        m_rtsCheck->blockSignals(false);
    } else {
        m_dtrCheck->blockSignals(true);
        m_rtsCheck->blockSignals(true);
        m_dtrCheck->setChecked(false);
        m_rtsCheck->setChecked(false);
        m_dtrCheck->blockSignals(false);
        m_rtsCheck->blockSignals(false);
    }
}

// ============================================================================
// 数据收发
// ============================================================================

void SerialWidget::onDataReceived()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        m_terminal->appendOutput(data);
    }
}

void SerialWidget::onTerminalDataReady(const QByteArray& data)
{
    sendData(data);
}

void SerialWidget::sendData(const QByteArray& data)
{
    if (!m_portOpen || !m_serialPort->isOpen()) {
        Logger::instance().warning("Serial", "串口未打开，无法发送数据");
        return;
    }

    qint64 written = m_serialPort->write(data);
    if (written == -1) {
        Logger::instance().error("Serial",
                                 "发送数据失败: " + m_serialPort->errorString());
    } else if (written < data.size()) {
        Logger::instance().warning("Serial",
                                   QString("部分数据未发送: %1/%2 字节").arg(written).arg(data.size()));
    }
}

// ============================================================================
// 错误处理
// ============================================================================

void SerialWidget::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QString errMsg = m_serialPort->errorString();
    Logger::instance().error("Serial",
                             QString("串口错误 (%1): %2").arg(error).arg(errMsg));

    // 资源错误时自动关闭
    if (error == QSerialPort::ResourceError) {
        QString portName = m_serialPort->portName();
        closePort();
        QMessageBox::warning(this, "串口错误",
                             QString("串口 %1 发生资源错误:\n%2\n\n连接已关闭。")
                                 .arg(portName, errMsg));
    }

    emit errorOccurred(errMsg);
}

// ============================================================================
// DTR / RTS 控制
// ============================================================================

void SerialWidget::onDtrToggled(bool checked)
{
    if (m_portOpen && m_serialPort->isOpen()) {
        m_serialPort->setDataTerminalReady(checked);
        Logger::instance().info("Serial",
                                QString("DTR %1").arg(checked ? "ON" : "OFF"));
    }
}

void SerialWidget::onRtsToggled(bool checked)
{
    if (m_portOpen && m_serialPort->isOpen()) {
        m_serialPort->setRequestToSend(checked);
        Logger::instance().info("Serial",
                                QString("RTS %1").arg(checked ? "ON" : "OFF"));
    }
}

// ============================================================================
// 发送文件
// ============================================================================

void SerialWidget::onSendFile()
{
    if (!m_portOpen) return;

    QString filePath = QFileDialog::getOpenFileName(
        this, "选择要发送的文件", QString(),
        "所有文件 (*.*);;文本文件 (*.txt *.log *.cfg);;二进制文件 (*.bin *.dat)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误",
                              "无法打开文件:\n" + filePath);
        Logger::instance().error("Serial", "无法打开发送文件: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        QMessageBox::information(this, "提示", "文件为空。");
        return;
    }

    qint64 fileSize = data.size();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认发送",
        QString("即将发送文件:\n%1\n大小: %2 字节\n\n确定发送原始字节到串口?")
            .arg(filePath)
            .arg(fileSize),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    sendData(data);

    Logger::instance().info("Serial",
                            QString("已发送文件: %1 (%2 字节)").arg(filePath).arg(fileSize));

    m_terminal->appendOutput(
        QString("\r\n[已发送文件: %1, %2 字节]\r\n")
            .arg(QFileInfo(filePath).fileName())
            .arg(fileSize)
            .toUtf8()
    );
}