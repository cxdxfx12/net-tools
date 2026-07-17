#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QProcess>

// ============================================================================
// ModbusWidget — Modbus 协议调试和测试工具 (基于命令行工具)
// ============================================================================
class ModbusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModbusWidget(QWidget* parent = nullptr);
    ~ModbusWidget() override;

private slots:
    void onReadCoils();
    void onReadDiscreteInputs();
    void onReadHoldingRegisters();
    void onReadInputRegisters();
    void onWriteSingleCoil();
    void onWriteSingleRegister();
    void onWriteMultipleRegisters();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClearOutput();
    void onExportResults();

private:
    void setupUI();
    void setupConnections();
    void appendOutput(const QString& text, const QString& color = "#E6EDF3");
    void executeModbusCmd(const QStringList& args, const QString& desc);
    void populateResultsTable(const QString& output, const QString& funcDesc);

    // --- 连接信息 ---
    QLineEdit*    m_hostEdit;
    QSpinBox*     m_portSpin;
    QComboBox*    m_modeCombo;  // TCP/RTU/ASCII
    QLineEdit*    m_deviceEdit; // 串口设备 (RTU/ASCII)
    QSpinBox*     m_slaveIdSpin;
    QLabel*       m_statusLabel;

    // --- 读取参数 ---
    QSpinBox*     m_readAddrSpin;
    QSpinBox*     m_readCountSpin;
    QPushButton*  m_readCoilsBtn;
    QPushButton*  m_readDiscreteBtn;
    QPushButton*  m_readHoldingBtn;
    QPushButton*  m_readInputBtn;

    // --- 写入参数 ---
    QSpinBox*     m_writeAddrSpin;
    QLineEdit*    m_writeValueEdit;
    QPushButton*  m_writeCoilBtn;
    QPushButton*  m_writeRegBtn;
    QPushButton*  m_writeMultiRegBtn;

    // --- 结果 ---
    QTableWidget*    m_resultTable;
    QPlainTextEdit*  m_outputText;
    QPushButton*     m_exportBtn;
    QPushButton*     m_clearOutputBtn;

    // --- 状态 ---
    QProcess*       m_process;
};
