#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QByteArray>
#include <QFile>

class TerminalWidget;

class SerialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SerialWidget(QWidget* parent = nullptr);
    ~SerialWidget() override;

    bool isOpen() const;

signals:
    void portOpened(const QString& portName);
    void portClosed();
    void errorOccurred(const QString& error);

private slots:
    void onOpenClose();
    void onRefreshPorts();
    void onSendFile();
    void onDataReceived();
    void onTerminalDataReady(const QByteArray& data);
    void onSerialError(QSerialPort::SerialPortError error);
    void onDtrToggled(bool checked);
    void onRtsToggled(bool checked);
    void pollPorts();

private:
    void setupUI();
    void setupConnections();
    void refreshPortList();
    void openPort();
    void closePort();
    void updatePortStatus();
    void applySettings();
    void sendData(const QByteArray& data);
    QStringList detectPorts();

    // --- UI 组件 ---
    QComboBox*   m_portCombo;
    QComboBox*   m_baudRateCombo;
    QComboBox*   m_dataBitsCombo;
    QComboBox*   m_stopBitsCombo;
    QComboBox*   m_parityCombo;
    QComboBox*   m_flowControlCombo;
    QComboBox*   m_encodingCombo;
    QPushButton* m_openBtn;
    QPushButton* m_refreshBtn;
    QPushButton* m_sendFileBtn;
    QCheckBox*   m_dtrCheck;
    QCheckBox*   m_rtsCheck;
    QLabel*      m_statusLabel;
    QLabel*      m_statusIcon;
    TerminalWidget* m_terminal;

    // --- 串口 ---
    QSerialPort* m_serialPort;
    QTimer*      m_pollTimer;
    bool         m_portOpen;
};