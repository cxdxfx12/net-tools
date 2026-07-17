#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QProcess>
#include <QByteArray>
#include <QVector>
#include <QStringList>
#include <QMap>

// ============================================================================
// PacketInfo — 单个数据包信息
// ============================================================================
struct PacketInfo
{
    int     number = 0;
    QString timestamp;
    QString source;
    QString destination;
    QString protocol;
    int     length = 0;
    QString info;
    QByteArray hexData;   // 原始 hex 字节，用于 hex dump
    QString rawLine;      // tcpdump 原始输出行，用于导出
};

// ============================================================================
// PacketCaptureWidget — 数据包捕获工具 (基于 tcpdump)
// ============================================================================
class PacketCaptureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PacketCaptureWidget(QWidget* parent = nullptr);
    ~PacketCaptureWidget() override;

private slots:
    void onStartCapture();
    void onStopCapture();
    void onClearCapture();
    void onExportPcap();
    void onInterfaceRefresh();
    void onPacketSelected(int row, int col);
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setupUI();
    void setupConnections();
    void detectInterfaces();
    void addPacketRow(const PacketInfo& pkt);
    void updateProtocolStats();
    void showHexDump(const PacketInfo& pkt);
    bool checkSudoWarning();
    QString buildTcpdumpArgs();
    void parsePacketLine(const QString& line);
    QString parseProtocol(const QString& line);
    void parseSourceDest(const QString& line, QString& src, QString& dst);
    int parseLength(const QString& line);
    QString parseInfo(const QString& line);

    // --- UI 组件 ---
    // 配置区
    QComboBox*   m_interfaceCombo;
    QPushButton* m_refreshInterfaceBtn;
    QLineEdit*   m_filterEdit;
    QSpinBox*    m_packetCountSpin;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_exportBtn;

    // 状态区
    QLabel*      m_statusLabel;
    QLabel*      m_packetCountLabel;

    // 数据包列表
    QTableWidget* m_packetTable;

    // 数据包详情 (hex dump + ASCII)
    QPlainTextEdit* m_detailView;

    // 协议层次统计
    QTreeWidget* m_protocolTree;

    // --- 捕获状态 ---
    QProcess*       m_process;
    QVector<PacketInfo> m_packets;
    QStringList     m_rawOutput;       // 原始 tcpdump 输出
    int             m_packetCount = 0;
    bool            m_capturing = false;
    int             m_maxPackets = 0;
    QString         m_tempPcapPath;    // 临时 pcap 文件路径
    QString         m_currentFilter;

    // 协议统计
    QMap<QString, int> m_protocolCounts;
    struct PacketBuffer {
        QStringList hexLines;
        QString summaryLine;
    };
    PacketBuffer m_currentBuf;
    bool m_inHexDump = false;
};