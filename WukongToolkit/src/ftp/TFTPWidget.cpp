#include "ftp/TFTPWidget.h"
#include "database/DatabaseManager.h"
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
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkDatagram>
#include <QDir>
#include <QFileInfo>

// ═══════════════════════════════════════════════════════════════════════════
// TFTPServer
// ═══════════════════════════════════════════════════════════════════════════

// TFTP opcodes
static constexpr quint16 TFTP_RRQ   = 1;
static constexpr quint16 TFTP_WRQ   = 2;
static constexpr quint16 TFTP_DATA  = 3;
static constexpr quint16 TFTP_ACK   = 4;
static constexpr quint16 TFTP_ERROR = 5;

static constexpr int TFTP_BLOCK_SIZE = 512;
static constexpr int TFTP_MAX_RETRIES = 5;

TFTPServer::TFTPServer(QObject* parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_running(false)
    , m_port(0)
{
}

TFTPServer::~TFTPServer()
{
    stop();
}

bool TFTPServer::start(quint16 port, const QString& rootDir)
{
    if (m_running) {
        stop();
    }

    QDir dir(rootDir);
    if (!dir.exists()) {
        emit serverLog(QString("Root directory does not exist: %1").arg(rootDir));
        return false;
    }

    m_rootDir = dir.absolutePath();
    m_port = port;

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &TFTPServer::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &TFTPServer::onErrorOccurred);

    if (!m_socket->bind(QHostAddress::Any, port)) {
        emit serverLog(QString("Failed to bind to port %1: %2").arg(port).arg(m_socket->errorString()));
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    m_running = true;
    emit serverLog(QString("TFTP server started on port %1, root directory: %2").arg(port).arg(m_rootDir));
    return true;
}

void TFTPServer::stop()
{
    if (!m_running) return;

    m_running = false;

    // Close all pending transfers
    for (auto& transfer : m_transfers) {
        if (transfer.file && transfer.file->isOpen()) {
            transfer.file->close();
        }
        delete transfer.file;
    }
    m_transfers.clear();

    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    emit serverLog("TFTP server stopped");
}

bool TFTPServer::isRunning() const
{
    return m_running;
}

quint16 TFTPServer::port() const
{
    return m_port;
}

QString TFTPServer::rootDir() const
{
    return m_rootDir;
}

QString TFTPServer::clientKey(const QHostAddress& addr, quint16 port) const
{
    return addr.toString() + ":" + QString::number(port);
}

void TFTPServer::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress sender = datagram.senderAddress();
        quint16 senderPort = datagram.senderPort();

        if (data.size() < 4) {
            emit serverLog(QString("Invalid packet from %1:%2 (too small)").arg(sender.toString()).arg(senderPort));
            continue;
        }

        quint16 opcode = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);

        switch (opcode) {
        case TFTP_RRQ:
        case TFTP_WRQ: {
            // Parse filename and mode from the packet
            int pos = 2;
            int filenameEnd = data.indexOf('\0', pos);
            if (filenameEnd < 0) {
                emit serverLog(QString("Malformed request from %1:%2").arg(sender.toString()).arg(senderPort));
                break;
            }
            QString filename = QString::fromUtf8(data.mid(pos, filenameEnd - pos));
            pos = filenameEnd + 1;
            int modeEnd = data.indexOf('\0', pos);
            QString mode = (modeEnd > pos) ? QString::fromLatin1(data.mid(pos, modeEnd - pos)).toLower() : "octet";

            emit clientConnected(sender.toString());

            if (opcode == TFTP_RRQ) {
                handleRRQ(sender, senderPort, filename, mode);
            } else {
                handleWRQ(sender, senderPort, filename, mode);
            }
            break;
        }
        case TFTP_ACK: {
            quint16 blockNum = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
            QString key = clientKey(sender, senderPort);
            auto it = m_transfers.find(key);
            if (it != m_transfers.end() && it->isWrite == false) {
                // Client acknowledged our DATA block during RRQ
                if (blockNum == it->lastBlock) {
                    QByteArray nextBlock = readBlock(*it->file, blockNum + 1);
                    if (nextBlock.isEmpty() && it->file->atEnd()) {
                        // Transfer complete
                        sendData(sender, senderPort, blockNum + 1, QByteArray());
                        it->lastBlock = blockNum + 1;
                        qint64 total = it->file->size();
                        it->file->close();
                        emit transferCompleted(sender.toString(), it->filename, total);
                        delete it->file;
                        m_transfers.erase(it);
                    } else if (!nextBlock.isEmpty()) {
                        sendData(sender, senderPort, blockNum + 1, nextBlock);
                        it->lastBlock = blockNum + 1;
                        emit transferProgress(sender.toString(), blockNum + 1, (blockNum + 1) * TFTP_BLOCK_SIZE);
                    }
                }
            } else if (it != m_transfers.end() && it->isWrite == true) {
                // Client acknowledged our ACK during WRQ — this shouldn't happen in normal TFTP
                // but we handle it gracefully
            }
            break;
        }
        case TFTP_DATA: {
            // DATA packet from client during WRQ
            quint16 blockNum = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
            QByteArray fileData = data.mid(4);
            QString key = clientKey(sender, senderPort);
            auto it = m_transfers.find(key);
            if (it != m_transfers.end() && it->isWrite == true) {
                if (blockNum == it->lastBlock + 1) {
                    it->file->write(fileData);
                    it->lastBlock = blockNum;
                    sendAck(sender, senderPort, blockNum);
                    emit transferProgress(sender.toString(), blockNum, blockNum * TFTP_BLOCK_SIZE);

                    if (fileData.size() < TFTP_BLOCK_SIZE) {
                        // Last block received
                        qint64 total = it->file->size();
                        it->file->close();
                        emit transferCompleted(sender.toString(), it->filename, total);
                        delete it->file;
                        m_transfers.erase(it);
                    }
                }
            }
            break;
        }
        case TFTP_ERROR: {
            QString key = clientKey(sender, senderPort);
            auto it = m_transfers.find(key);
            if (it != m_transfers.end()) {
                QString errMsg = data.size() > 4 ? QString::fromUtf8(data.mid(4, data.indexOf('\0', 4) - 4)) : "Unknown error";
                emit transferError(sender.toString(), errMsg);
                if (it->file) {
                    if (it->file->isOpen()) it->file->close();
                    delete it->file;
                }
                m_transfers.erase(it);
            }
            break;
        }
        default:
            emit serverLog(QString("Unknown opcode %1 from %2:%3").arg(opcode).arg(sender.toString()).arg(senderPort));
            sendError(sender, senderPort, 4, "Illegal TFTP operation");
            break;
        }
    }
}

void TFTPServer::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit serverLog(QString("Socket error: %1").arg(m_socket ? m_socket->errorString() : "unknown"));
}

void TFTPServer::handleRRQ(const QHostAddress& sender, quint16 senderPort, const QString& filename, const QString& mode)
{
    Q_UNUSED(mode)

    // Sanitize filename to prevent path traversal
    QString safeFilename = filename;
    safeFilename.remove("../");
    safeFilename.remove("..\\");
    if (safeFilename.startsWith('/') || safeFilename.startsWith('\\')) {
        safeFilename = safeFilename.mid(1);
    }

    QString filePath = m_rootDir + "/" + safeFilename;
    QFileInfo fileInfo(filePath);

    // Ensure the resolved path is within root directory
    if (!fileInfo.absoluteFilePath().startsWith(m_rootDir)) {
        emit serverLog(QString("Path traversal blocked: %1 from %2").arg(filename).arg(sender.toString()));
        sendError(sender, senderPort, 2, "Access violation");
        return;
    }

    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit serverLog(QString("File not found: %1 (requested by %2)").arg(filename).arg(sender.toString()));
        sendError(sender, senderPort, 1, "File not found");
        return;
    }

    auto* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit serverLog(QString("Cannot open file: %1").arg(filePath));
        sendError(sender, senderPort, 2, "Access violation");
        delete file;
        return;
    }

    ClientTransfer transfer;
    transfer.address = sender;
    transfer.port = senderPort;
    transfer.lastBlock = 0;
    transfer.filename = filename;
    transfer.file = file;
    transfer.isWrite = false;

    QString key = clientKey(sender, senderPort);
    m_transfers[key] = transfer;

    emit transferStarted(sender.toString(), filename, "RRQ (download)");

    // Send first data block
    QByteArray firstBlock = readBlock(*file, 1);
    sendData(sender, senderPort, 1, firstBlock);
    transfer.lastBlock = 1;
    m_transfers[key] = transfer;

    emit transferProgress(sender.toString(), 1, firstBlock.size());
}

void TFTPServer::handleWRQ(const QHostAddress& sender, quint16 senderPort, const QString& filename, const QString& mode)
{
    Q_UNUSED(mode)

    // Sanitize filename to prevent path traversal
    QString safeFilename = filename;
    safeFilename.remove("../");
    safeFilename.remove("..\\");
    if (safeFilename.startsWith('/') || safeFilename.startsWith('\\')) {
        safeFilename = safeFilename.mid(1);
    }

    QString filePath = m_rootDir + "/" + safeFilename;
    QFileInfo fileInfo(filePath);

    if (!fileInfo.absoluteFilePath().startsWith(m_rootDir)) {
        emit serverLog(QString("Path traversal blocked: %1 from %2").arg(filename).arg(sender.toString()));
        sendError(sender, senderPort, 2, "Access violation");
        return;
    }

    if (fileInfo.exists()) {
        emit serverLog(QString("File already exists: %1 (requested by %2)").arg(filename).arg(sender.toString()));
        sendError(sender, senderPort, 6, "File already exists");
        return;
    }

    auto* file = new QFile(filePath);
    if (!file->open(QIODevice::WriteOnly)) {
        emit serverLog(QString("Cannot create file: %1").arg(filePath));
        sendError(sender, senderPort, 2, "Access violation");
        delete file;
        return;
    }

    ClientTransfer transfer;
    transfer.address = sender;
    transfer.port = senderPort;
    transfer.lastBlock = 0;
    transfer.filename = filename;
    transfer.file = file;
    transfer.isWrite = true;

    QString key = clientKey(sender, senderPort);
    m_transfers[key] = transfer;

    emit transferStarted(sender.toString(), filename, "WRQ (upload)");

    // Acknowledge WRQ with block 0
    sendAck(sender, senderPort, 0);
}

void TFTPServer::sendData(const QHostAddress& addr, quint16 port, quint16 blockNum, const QByteArray& data)
{
    QByteArray packet;
    packet.resize(4 + data.size());
    packet[0] = static_cast<char>((TFTP_DATA >> 8) & 0xFF);
    packet[1] = static_cast<char>(TFTP_DATA & 0xFF);
    packet[2] = static_cast<char>((blockNum >> 8) & 0xFF);
    packet[3] = static_cast<char>(blockNum & 0xFF);
    if (!data.isEmpty()) {
        memcpy(packet.data() + 4, data.constData(), data.size());
    }
    m_socket->writeDatagram(packet, addr, port);
}

void TFTPServer::sendAck(const QHostAddress& addr, quint16 port, quint16 blockNum)
{
    QByteArray packet;
    packet.resize(4);
    packet[0] = static_cast<char>((TFTP_ACK >> 8) & 0xFF);
    packet[1] = static_cast<char>(TFTP_ACK & 0xFF);
    packet[2] = static_cast<char>((blockNum >> 8) & 0xFF);
    packet[3] = static_cast<char>(blockNum & 0xFF);
    m_socket->writeDatagram(packet, addr, port);
}

void TFTPServer::sendError(const QHostAddress& addr, quint16 port, quint16 errorCode, const QString& errorMsg)
{
    QByteArray msg = errorMsg.toUtf8();
    QByteArray packet;
    packet.resize(4 + msg.size() + 1);
    packet[0] = static_cast<char>((TFTP_ERROR >> 8) & 0xFF);
    packet[1] = static_cast<char>(TFTP_ERROR & 0xFF);
    packet[2] = static_cast<char>((errorCode >> 8) & 0xFF);
    packet[3] = static_cast<char>(errorCode & 0xFF);
    memcpy(packet.data() + 4, msg.constData(), msg.size());
    packet[4 + msg.size()] = '\0';
    m_socket->writeDatagram(packet, addr, port);
}

QByteArray TFTPServer::readBlock(QFile& file, quint16 blockNum)
{
    Q_UNUSED(blockNum)
    return file.read(TFTP_BLOCK_SIZE);
}

// ═══════════════════════════════════════════════════════════════════════════
// TFTPWidget
// ═══════════════════════════════════════════════════════════════════════════

TFTPWidget::TFTPWidget(QWidget* parent)
    : QWidget(parent)
    , m_tftpServer(nullptr)
    , m_clientProcess(nullptr)
    , m_clientCurrentPort(69)
    , m_clientIsUpload(true)
{
    setupUI();
    setupConnections();
}

TFTPWidget::~TFTPWidget()
{
    if (m_tftpServer) {
        m_tftpServer->stop();
        delete m_tftpServer;
        m_tftpServer = nullptr;
    }
    if (m_clientProcess && m_clientProcess->state() != QProcess::NotRunning) {
        m_clientProcess->kill();
        m_clientProcess->waitForFinished(3000);
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void TFTPWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Mode selector ──
    auto* modeLayout = new QHBoxLayout();
    auto* modeLabel = new QLabel("模式:");
    modeLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    m_modeCombo = new QComboBox();
    m_modeCombo->addItem("TFTP 服务器", 0);
    m_modeCombo->addItem("TFTP 客户端", 1);
    m_modeCombo->setStyleSheet(
        "QComboBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px; min-width: 160px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #25262B; color: #DCDCDC;"
        "  selection-background-color: #409EFF; border: 1px solid #3C3F41;"
        "}"
    );
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_modeCombo);
    modeLayout->addStretch();
    mainLayout->addLayout(modeLayout);

    // ── Stacked pages ──
    m_pageStack = new QStackedWidget();
    m_pageStack->addWidget(createServerPage());
    m_pageStack->addWidget(createClientPage());
    m_pageStack->setCurrentIndex(0);
    mainLayout->addWidget(m_pageStack, 1);

    // ── Transfer history ──
    auto* historyGroup = new QGroupBox("传输历史");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setContentsMargins(4, 4, 4, 4);

    m_historyTable = new QTableWidget(0, 7);
    m_historyTable->setHorizontalHeaderLabels({"时间", "方向", "服务器", "端口", "本地文件", "远程文件", "状态"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setMaximumHeight(150);
    m_historyTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  gridline-color: #3C3F41; border: 1px solid #3C3F41;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 2px 4px; }"
        "QHeaderView::section {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  border: none; border-bottom: 2px solid #3C3F41;"
        "  padding: 3px 6px; font-size: 11px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #25262B; }"
    );
    historyLayout->addWidget(m_historyTable);

    auto* historyBtnLayout = new QHBoxLayout();
    historyBtnLayout->addStretch();

    m_exportBtn = new QPushButton("导出 CSV");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
    );
    historyBtnLayout->addWidget(m_exportBtn);

    m_clearBtn = new QPushButton("清空历史");
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #5C5C5C; color: white;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #707070; }"
    );
    historyBtnLayout->addWidget(m_clearBtn);

    historyLayout->addLayout(historyBtnLayout);
    mainLayout->addWidget(historyGroup);
}

QWidget* TFTPWidget::createServerPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // ── Server configuration ──
    auto* configGroup = new QGroupBox("服务器配置");
    auto* configLayout = new QFormLayout(configGroup);
    configLayout->setSpacing(8);

    m_serverPortSpin = new QSpinBox();
    m_serverPortSpin->setRange(1, 65535);
    m_serverPortSpin->setValue(69);
    m_serverPortSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    configLayout->addRow("监听端口:", m_serverPortSpin);

    auto* rootDirLayout = new QHBoxLayout();
    m_serverRootDirEdit = new QLineEdit();
    m_serverRootDirEdit->setPlaceholderText("选择 TFTP 根目录...");
    m_serverRootDirEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    rootDirLayout->addWidget(m_serverRootDirEdit);

    m_serverBrowseBtn = new QPushButton("浏览");
    m_serverBrowseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3C3F41; color: #DCDCDC;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #4C4F51; }"
    );
    rootDirLayout->addWidget(m_serverBrowseBtn);
    configLayout->addRow("根目录:", rootDirLayout);

    layout->addWidget(configGroup);

    // ── Server controls ──
    auto* controlLayout = new QHBoxLayout();

    m_serverStartBtn = new QPushButton("启动服务器");
    m_serverStartBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    controlLayout->addWidget(m_serverStartBtn);

    m_serverStopBtn = new QPushButton("停止服务器");
    m_serverStopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F56C6C; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #F78989; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    m_serverStopBtn->setEnabled(false);
    controlLayout->addWidget(m_serverStopBtn);

    m_serverStatusLabel = new QLabel("状态: 已停止");
    m_serverStatusLabel->setStyleSheet("font-size: 13px; color: #8C8C8C; padding: 0 12px;");
    controlLayout->addWidget(m_serverStatusLabel);

    controlLayout->addStretch();
    layout->addLayout(controlLayout);

    // ── Server log ──
    auto* logGroup = new QGroupBox("服务器日志");
    auto* logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(4, 4, 4, 4);

    m_serverLogEdit = new QTextEdit();
    m_serverLogEdit->setReadOnly(true);
    m_serverLogEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41;"
        "  font-family: 'Consolas', 'Menlo', monospace;"
        "  font-size: 12px;"
        "}"
    );
    m_serverLogEdit->setPlaceholderText("服务器日志将显示在这里...");
    logLayout->addWidget(m_serverLogEdit);
    layout->addWidget(logGroup, 1);

    return page;
}

QWidget* TFTPWidget::createClientPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // ── Client configuration ──
    auto* configGroup = new QGroupBox("客户端配置");
    auto* configLayout = new QFormLayout(configGroup);
    configLayout->setSpacing(8);

    m_clientServerAddrEdit = new QLineEdit();
    m_clientServerAddrEdit->setPlaceholderText("输入 TFTP 服务器地址, 如 192.168.1.1");
    m_clientServerAddrEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    configLayout->addRow("服务器地址:", m_clientServerAddrEdit);

    m_clientPortSpin = new QSpinBox();
    m_clientPortSpin->setRange(1, 65535);
    m_clientPortSpin->setValue(69);
    m_clientPortSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    configLayout->addRow("端口:", m_clientPortSpin);

    auto* localFileLayout = new QHBoxLayout();
    m_clientLocalFileEdit = new QLineEdit();
    m_clientLocalFileEdit->setPlaceholderText("选择本地文件路径...");
    m_clientLocalFileEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    localFileLayout->addWidget(m_clientLocalFileEdit);

    m_clientBrowseBtn = new QPushButton("浏览");
    m_clientBrowseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3C3F41; color: #DCDCDC;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #4C4F51; }"
    );
    localFileLayout->addWidget(m_clientBrowseBtn);
    configLayout->addRow("本地文件:", localFileLayout);

    m_clientRemoteFileEdit = new QLineEdit();
    m_clientRemoteFileEdit->setPlaceholderText("输入远程文件名, 如 config.bin");
    m_clientRemoteFileEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    configLayout->addRow("远程文件:", m_clientRemoteFileEdit);

    layout->addWidget(configGroup);

    // ── Client controls ──
    auto* controlLayout = new QHBoxLayout();

    m_clientUploadBtn = new QPushButton("上传 (PUT)");
    m_clientUploadBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #409EFF; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #66B1FF; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    controlLayout->addWidget(m_clientUploadBtn);

    m_clientDownloadBtn = new QPushButton("下载 (GET)");
    m_clientDownloadBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #67C23A; color: white;"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #85CE61; }"
        "QPushButton:disabled { background-color: #5C5C5C; }"
    );
    controlLayout->addWidget(m_clientDownloadBtn);

    m_clientStatusLabel = new QLabel("就绪");
    m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #8C8C8C; padding: 0 12px;");
    controlLayout->addWidget(m_clientStatusLabel);

    controlLayout->addStretch();
    layout->addLayout(controlLayout);

    // ── Progress bar ──
    m_clientProgressBar = new QProgressBar();
    m_clientProgressBar->setRange(0, 100);
    m_clientProgressBar->setValue(0);
    m_clientProgressBar->setTextVisible(true);
    m_clientProgressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #3C3F41; border-radius: 3px;"
        "  background-color: #25262B; text-align: center;"
        "  color: #DCDCDC; font-size: 12px; height: 22px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #409EFF; border-radius: 2px;"
        "}"
    );
    layout->addWidget(m_clientProgressBar);

    layout->addStretch(1);

    return page;
}

// ─── Connections ───────────────────────────────────────────────────────

void TFTPWidget::setupConnections()
{
    // Mode switching
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TFTPWidget::onModeChanged);

    // Server
    connect(m_serverBrowseBtn, &QPushButton::clicked, this, &TFTPWidget::onServerBrowseRootDir);
    connect(m_serverStartBtn, &QPushButton::clicked, this, &TFTPWidget::onServerStart);
    connect(m_serverStopBtn, &QPushButton::clicked, this, &TFTPWidget::onServerStop);

    // Client
    connect(m_clientBrowseBtn, &QPushButton::clicked, this, &TFTPWidget::onClientBrowseLocalFile);
    connect(m_clientUploadBtn, &QPushButton::clicked, this, &TFTPWidget::onClientUpload);
    connect(m_clientDownloadBtn, &QPushButton::clicked, this, &TFTPWidget::onClientDownload);

    // History
    connect(m_exportBtn, &QPushButton::clicked, this, &TFTPWidget::onExportHistory);
    connect(m_clearBtn, &QPushButton::clicked, this, &TFTPWidget::onClearHistory);
}

// ─── Mode Switching ────────────────────────────────────────────────────

void TFTPWidget::onModeChanged(int index)
{
    m_pageStack->setCurrentIndex(index);
}

// ═══════════════════════════════════════════════════════════════════════════
// Server Operations
// ═══════════════════════════════════════════════════════════════════════════

void TFTPWidget::onServerBrowseRootDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择 TFTP 根目录");
    if (!dir.isEmpty()) {
        m_serverRootDirEdit->setText(dir);
    }
}

void TFTPWidget::onServerStart()
{
    if (m_serverRootDirEdit->text().trimmed().isEmpty()) {
        Logger::instance().warning("TFTP", "请先选择根目录");
        m_serverLogEdit->append("[警告] 请先选择 TFTP 根目录");
        return;
    }

    if (m_tftpServer) {
        delete m_tftpServer;
        m_tftpServer = nullptr;
    }

    m_tftpServer = new TFTPServer(this);

    connect(m_tftpServer, &TFTPServer::serverLog, this, &TFTPWidget::onServerLog);
    connect(m_tftpServer, &TFTPServer::transferStarted, this, &TFTPWidget::onServerTransferStarted);
    connect(m_tftpServer, &TFTPServer::transferCompleted, this, &TFTPWidget::onServerTransferCompleted);

    quint16 port = static_cast<quint16>(m_serverPortSpin->value());
    QString rootDir = m_serverRootDirEdit->text().trimmed();

    if (m_tftpServer->start(port, rootDir)) {
        m_serverStartBtn->setEnabled(false);
        m_serverStopBtn->setEnabled(true);
        m_serverPortSpin->setEnabled(false);
        m_serverRootDirEdit->setEnabled(false);
        m_serverBrowseBtn->setEnabled(false);
        m_serverStatusLabel->setText(QString("状态: 运行中 (端口 %1)").arg(port));
        m_serverStatusLabel->setStyleSheet("font-size: 13px; color: #67C23A; padding: 0 12px; font-weight: bold;");
        Logger::instance().info("TFTP", QString("Server started on port %1, root: %2").arg(port).arg(rootDir));
    } else {
        m_serverLogEdit->append("[错误] 启动 TFTP 服务器失败");
        Logger::instance().error("TFTP", "Failed to start server");
        delete m_tftpServer;
        m_tftpServer = nullptr;
    }
}

void TFTPWidget::onServerStop()
{
    if (m_tftpServer) {
        m_tftpServer->stop();
        delete m_tftpServer;
        m_tftpServer = nullptr;
    }

    m_serverStartBtn->setEnabled(true);
    m_serverStopBtn->setEnabled(false);
    m_serverPortSpin->setEnabled(true);
    m_serverRootDirEdit->setEnabled(true);
    m_serverBrowseBtn->setEnabled(true);
    m_serverStatusLabel->setText("状态: 已停止");
    m_serverStatusLabel->setStyleSheet("font-size: 13px; color: #8C8C8C; padding: 0 12px;");
    Logger::instance().info("TFTP", "Server stopped");
}

void TFTPWidget::onServerLog(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_serverLogEdit->append(QString("[%1] %2").arg(timestamp, message));
}

void TFTPWidget::onServerTransferStarted(const QString& clientAddr, const QString& filename, const QString& mode)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_serverLogEdit->append(QString("[%1] %2 - %3: %4").arg(timestamp, clientAddr, mode, filename));
    Logger::instance().info("TFTP", QString("Transfer started: %1 %2 from %3").arg(mode, filename, clientAddr));
}

void TFTPWidget::onServerTransferCompleted(const QString& clientAddr, const QString& filename, qint64 totalBytes)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString sizeStr;
    if (totalBytes >= 1024 * 1024) {
        sizeStr = QString::number(totalBytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (totalBytes >= 1024) {
        sizeStr = QString::number(totalBytes / 1024.0, 'f', 2) + " KB";
    } else {
        sizeStr = QString::number(totalBytes) + " B";
    }
    m_serverLogEdit->append(QString("[%1] ✓ 传输完成: %2 → %3 (%4)")
                              .arg(timestamp, clientAddr, filename, sizeStr));

    // Add to history
    TransferRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.direction = "下载";
    record.serverAddr = clientAddr;
    record.port = m_serverPortSpin->value();
    record.localFile = m_serverRootDirEdit->text() + "/" + filename;
    record.remoteFile = filename;
    record.fileSize = totalBytes;
    record.success = true;
    addHistoryRecord(record);
    saveToDatabase(record);

    Logger::instance().info("TFTP", QString("Transfer completed: %1 (%2 bytes)").arg(filename).arg(totalBytes));
}

// ═══════════════════════════════════════════════════════════════════════════
// Client Operations
// ═══════════════════════════════════════════════════════════════════════════

void TFTPWidget::onClientBrowseLocalFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (!filePath.isEmpty()) {
        m_clientLocalFileEdit->setText(filePath);
    }
}

void TFTPWidget::onClientUpload()
{
    QString serverAddr = m_clientServerAddrEdit->text().trimmed();
    QString localFile = m_clientLocalFileEdit->text().trimmed();
    QString remoteFile = m_clientRemoteFileEdit->text().trimmed();

    if (serverAddr.isEmpty()) {
        Logger::instance().warning("TFTP", "请填写服务器地址");
        m_clientStatusLabel->setText("请填写服务器地址");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }
    if (localFile.isEmpty()) {
        Logger::instance().warning("TFTP", "请选择本地文件");
        m_clientStatusLabel->setText("请选择本地文件");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }
    if (remoteFile.isEmpty()) {
        Logger::instance().warning("TFTP", "请填写远程文件名");
        m_clientStatusLabel->setText("请填写远程文件名");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }

    QFileInfo fileInfo(localFile);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        Logger::instance().warning("TFTP", "本地文件不存在: " + localFile);
        m_clientStatusLabel->setText("本地文件不存在");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }

    if (m_clientProcess && m_clientProcess->state() != QProcess::NotRunning) {
        m_clientProcess->kill();
        m_clientProcess->waitForFinished(3000);
    }

    m_clientCurrentLocalFile = localFile;
    m_clientCurrentRemoteFile = remoteFile;
    m_clientCurrentServerAddr = serverAddr;
    m_clientCurrentPort = static_cast<quint16>(m_clientPortSpin->value());
    m_clientIsUpload = true;

    m_clientUploadBtn->setEnabled(false);
    m_clientDownloadBtn->setEnabled(false);
    m_clientProgressBar->setValue(0);
    m_clientStatusLabel->setText("上传中...");
    m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #409EFF; padding: 0 12px;");

    if (!m_clientProcess) {
        m_clientProcess = new QProcess(this);
        connect(m_clientProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &TFTPWidget::onClientProcessFinished);
        connect(m_clientProcess, &QProcess::errorOccurred, this, &TFTPWidget::onClientProcessError);
    }

    // Use system tftp client: tftp <host> <port> -c put <local> <remote>
    QStringList args;
    args << serverAddr;
    args << QString::number(m_clientCurrentPort);
    args << "-c" << "put" << localFile << remoteFile;

    m_clientProcess->start("tftp", args);
    Logger::instance().info("TFTP", QString("Upload started: %1 → %2@%3:%4")
                                       .arg(localFile, remoteFile, serverAddr)
                                       .arg(m_clientCurrentPort));
}

void TFTPWidget::onClientDownload()
{
    QString serverAddr = m_clientServerAddrEdit->text().trimmed();
    QString localFile = m_clientLocalFileEdit->text().trimmed();
    QString remoteFile = m_clientRemoteFileEdit->text().trimmed();

    if (serverAddr.isEmpty()) {
        Logger::instance().warning("TFTP", "请填写服务器地址");
        m_clientStatusLabel->setText("请填写服务器地址");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }
    if (localFile.isEmpty()) {
        Logger::instance().warning("TFTP", "请选择本地保存路径");
        m_clientStatusLabel->setText("请选择本地保存路径");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }
    if (remoteFile.isEmpty()) {
        Logger::instance().warning("TFTP", "请填写远程文件名");
        m_clientStatusLabel->setText("请填写远程文件名");
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        return;
    }

    if (m_clientProcess && m_clientProcess->state() != QProcess::NotRunning) {
        m_clientProcess->kill();
        m_clientProcess->waitForFinished(3000);
    }

    m_clientCurrentLocalFile = localFile;
    m_clientCurrentRemoteFile = remoteFile;
    m_clientCurrentServerAddr = serverAddr;
    m_clientCurrentPort = static_cast<quint16>(m_clientPortSpin->value());
    m_clientIsUpload = false;

    m_clientUploadBtn->setEnabled(false);
    m_clientDownloadBtn->setEnabled(false);
    m_clientProgressBar->setValue(0);
    m_clientStatusLabel->setText("下载中...");
    m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #409EFF; padding: 0 12px;");

    if (!m_clientProcess) {
        m_clientProcess = new QProcess(this);
        connect(m_clientProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &TFTPWidget::onClientProcessFinished);
        connect(m_clientProcess, &QProcess::errorOccurred, this, &TFTPWidget::onClientProcessError);
    }

    // Use system tftp client: tftp <host> <port> -c get <remote> <local>
    QStringList args;
    args << serverAddr;
    args << QString::number(m_clientCurrentPort);
    args << "-c" << "get" << remoteFile << localFile;

    m_clientProcess->start("tftp", args);
    Logger::instance().info("TFTP", QString("Download started: %1@%2:%3 → %4")
                                       .arg(remoteFile, serverAddr)
                                       .arg(m_clientCurrentPort)
                                       .arg(localFile));
}

void TFTPWidget::onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_clientUploadBtn->setEnabled(true);
    m_clientDownloadBtn->setEnabled(true);

    QString direction = m_clientIsUpload ? "上传" : "下载";

    TransferRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.direction = direction;
    record.serverAddr = m_clientCurrentServerAddr;
    record.port = m_clientCurrentPort;
    record.localFile = m_clientCurrentLocalFile;
    record.remoteFile = m_clientCurrentRemoteFile;

    if (exitCode == 0) {
        m_clientProgressBar->setValue(100);
        m_clientStatusLabel->setText(QString("%1 完成").arg(direction));
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #67C23A; padding: 0 12px;");

        // Read file size
        QFileInfo fileInfo(m_clientCurrentLocalFile);
        record.fileSize = fileInfo.exists() ? fileInfo.size() : 0;
        record.success = true;

        Logger::instance().info("TFTP", QString("%1 completed: %2").arg(direction, m_clientCurrentRemoteFile));
    } else {
        m_clientProgressBar->setValue(0);
        QString errOutput = m_clientProcess ? QString::fromUtf8(m_clientProcess->readAllStandardError()) : "";
        m_clientStatusLabel->setText(QString("%1 失败").arg(direction));
        m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");
        record.fileSize = 0;
        record.success = false;
        record.errorMsg = errOutput.trimmed();
        if (record.errorMsg.isEmpty()) {
            record.errorMsg = QString("进程退出码: %1").arg(exitCode);
        }

        Logger::instance().error("TFTP", QString("%1 failed: %2 (exit code %3)")
                                           .arg(direction, m_clientCurrentRemoteFile)
                                           .arg(exitCode));
    }

    addHistoryRecord(record);
    saveToDatabase(record);
}

void TFTPWidget::onClientProcessError(QProcess::ProcessError error)
{
    m_clientUploadBtn->setEnabled(true);
    m_clientDownloadBtn->setEnabled(true);
    m_clientProgressBar->setValue(0);

    QString errorStr;
    switch (error) {
    case QProcess::FailedToStart:
        errorStr = "tftp 命令未找到，请确认系统中已安装 TFTP 客户端";
        break;
    case QProcess::Timedout:
        errorStr = "传输超时";
        break;
    default:
        errorStr = QString("传输错误: %1").arg(m_clientProcess ? m_clientProcess->errorString() : "unknown");
        break;
    }

    m_clientStatusLabel->setText(errorStr);
    m_clientStatusLabel->setStyleSheet("font-size: 13px; color: #F56C6C; padding: 0 12px;");

    Logger::instance().error("TFTP", errorStr);
}

// ═══════════════════════════════════════════════════════════════════════════
// Transfer History
// ═══════════════════════════════════════════════════════════════════════════

void TFTPWidget::addHistoryRecord(const TransferRecord& record)
{
    int row = m_historyTable->rowCount();
    m_historyTable->insertRow(row);

    auto* timeItem = new QTableWidgetItem(record.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    auto* dirItem = new QTableWidgetItem(record.direction);
    auto* addrItem = new QTableWidgetItem(record.serverAddr);
    auto* portItem = new QTableWidgetItem(QString::number(record.port));
    auto* localItem = new QTableWidgetItem(record.localFile);
    auto* remoteItem = new QTableWidgetItem(record.remoteFile);
    auto* statusItem = new QTableWidgetItem(record.success ? "成功" : "失败");

    if (record.success) {
        statusItem->setForeground(QColor(0x67, 0xC2, 0x3A));
    } else {
        statusItem->setForeground(QColor(0xF5, 0x6C, 0x6C));
    }

    if (record.direction == "上传") {
        dirItem->setForeground(QColor(0x40, 0x9E, 0xFF));
    } else {
        dirItem->setForeground(QColor(0xE6, 0xA2, 0x3C));
    }

    m_historyTable->setItem(row, 0, timeItem);
    m_historyTable->setItem(row, 1, dirItem);
    m_historyTable->setItem(row, 2, addrItem);
    m_historyTable->setItem(row, 3, portItem);
    m_historyTable->setItem(row, 4, localItem);
    m_historyTable->setItem(row, 5, remoteItem);
    m_historyTable->setItem(row, 6, statusItem);

    m_historyTable->scrollToBottom();
}

void TFTPWidget::saveToDatabase(const TransferRecord& record)
{
    auto& db = DatabaseManager::instance();
    if (!db.isOpen()) {
        Logger::instance().warning("TFTP", "Database not open, skipping history save");
        return;
    }

    QSqlQuery query(db.database());
    query.prepare(
        "INSERT INTO tftp_history (time, direction, server, port, local_file, remote_file, size, success, error) "
        "VALUES (:time, :direction, :server, :port, :local_file, :remote_file, :size, :success, :error)"
    );
    query.bindValue(":time", record.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":direction", record.direction);
    query.bindValue(":server", record.serverAddr);
    query.bindValue(":port", record.port);
    query.bindValue(":local_file", record.localFile);
    query.bindValue(":remote_file", record.remoteFile);
    query.bindValue(":size", record.fileSize);
    query.bindValue(":success", record.success ? 1 : 0);
    query.bindValue(":error", record.errorMsg);

    if (!query.exec()) {
        Logger::instance().error("TFTP", "Failed to save history: " + query.lastError().text());
    }
}

void TFTPWidget::onExportHistory()
{
    if (m_historyTable->rowCount() == 0) {
        Logger::instance().warning("TFTP", "没有可导出的历史记录");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 TFTP 传输历史",
        QString("tftp_history_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().error("TFTP", "Failed to open file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF";
    stream << "时间,方向,服务器,端口,本地文件,远程文件,状态\n";

    for (int row = 0; row < m_historyTable->rowCount(); ++row) {
        stream << m_historyTable->item(row, 0)->text() << ","
               << m_historyTable->item(row, 1)->text() << ","
               << m_historyTable->item(row, 2)->text() << ","
               << m_historyTable->item(row, 3)->text() << ","
               << m_historyTable->item(row, 4)->text() << ","
               << m_historyTable->item(row, 5)->text() << ","
               << m_historyTable->item(row, 6)->text() << "\n";
    }

    file.close();
    Logger::instance().info("TFTP", "History exported to: " + filePath);
}

void TFTPWidget::onClearHistory()
{
    m_historyTable->setRowCount(0);
    Logger::instance().info("TFTP", "History cleared");
}