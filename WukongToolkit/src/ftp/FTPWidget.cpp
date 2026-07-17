#include "ftp/FTPWidget.h"
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
#include <QInputDialog>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// FTPWidget 实现
// ============================================================================

FTPWidget::FTPWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_connected(false)
{
    setupUI();
    setupConnections();
}

FTPWidget::~FTPWidget()
{
    if (m_process) {
        if (m_connected) {
            m_process->write("QUIT\n");
            m_process->waitForBytesWritten(1000);
        }
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void FTPWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 连接区 ──
    auto* connGroup = new QGroupBox("FTP 连接");
    auto* connLayout = new QHBoxLayout(connGroup);
    connLayout->setSpacing(8);

    // 主机
    auto* hostLabel = new QLabel("主机:");
    
    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("ftp.example.com");
    m_hostEdit->setMinimumWidth(160);

    // 端口
    auto* portLabel = new QLabel("端口:");
    
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(21);
    m_portSpin->setFixedWidth(80);

    // 用户名
    auto* userLabel = new QLabel("用户:");
    
    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("anonymous");
    m_userEdit->setMinimumWidth(100);

    // 密码
    auto* passLabel = new QLabel("密码:");
    
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setMinimumWidth(100);

    m_connectBtn = new QPushButton("连接");
    m_connectBtn->setFixedHeight(32);

    m_disconnectBtn = new QPushButton("断开");
    m_disconnectBtn->setFixedHeight(32);
    m_disconnectBtn->setEnabled(false);

    connLayout->addWidget(hostLabel);
    connLayout->addWidget(m_hostEdit, 1);
    connLayout->addWidget(portLabel);
    connLayout->addWidget(m_portSpin);
    connLayout->addWidget(userLabel);
    connLayout->addWidget(m_userEdit, 1);
    connLayout->addWidget(passLabel);
    connLayout->addWidget(m_passEdit, 1);
    connLayout->addWidget(m_connectBtn);
    connLayout->addWidget(m_disconnectBtn);

    mainLayout->addWidget(connGroup);

    // ── 状态标签 ──
    m_statusLabel = new QLabel("未连接");
    
    mainLayout->addWidget(m_statusLabel);

    // ── 文件浏览/操作区 ──
    auto* fileGroup = new QGroupBox("文件操作");
    auto* fileLayout = new QVBoxLayout(fileGroup);
    fileLayout->setSpacing(6);

    // 远程路径 + 列表按钮
    auto* remoteRow = new QHBoxLayout();
    remoteRow->setSpacing(8);
    auto* remoteLabel = new QLabel("远程路径:");
    
    m_remotePathEdit = new QLineEdit();
    m_remotePathEdit->setPlaceholderText("/");
    m_listBtn = new QPushButton("列出目录");
    m_listBtn->setFixedHeight(30);
    m_listBtn->setEnabled(false);
    remoteRow->addWidget(remoteLabel);
    remoteRow->addWidget(m_remotePathEdit, 1);
    remoteRow->addWidget(m_listBtn);
    fileLayout->addLayout(remoteRow);

    // 远程文件树
    m_remoteTree = new QTreeWidget();
    m_remoteTree->setHeaderLabels({"文件名", "大小", "修改时间"});
    m_remoteTree->setRootIsDecorated(false);
    m_remoteTree->setColumnCount(3);
    m_remoteTree->header()->setStretchLastSection(true);
    m_remoteTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_remoteTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_remoteTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    fileLayout->addWidget(m_remoteTree, 1);

    // 本地路径
    auto* localRow = new QHBoxLayout();
    localRow->setSpacing(8);
    auto* localLabel = new QLabel("本地路径:");
    
    m_localPathEdit = new QLineEdit();
    m_localPathEdit->setText(QDir::homePath());
    m_browseBtn = new QPushButton("浏览...");
    m_browseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 14px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    localRow->addWidget(localLabel);
    localRow->addWidget(m_localPathEdit, 1);
    localRow->addWidget(m_browseBtn);
    fileLayout->addLayout(localRow);

    // 上传/下载按钮
    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(8);
    m_uploadBtn = new QPushButton("上传文件");
    m_uploadBtn->setEnabled(false);
    m_downloadBtn = new QPushButton("下载文件");
    m_downloadBtn->setEnabled(false);
    actionRow->addStretch();
    actionRow->addWidget(m_uploadBtn);
    actionRow->addWidget(m_downloadBtn);
    fileLayout->addLayout(actionRow);

    mainLayout->addWidget(fileGroup, 1);

    // ── 命令输入 ──
    auto* cmdRow = new QHBoxLayout();
    cmdRow->setSpacing(8);
    m_commandEdit = new QLineEdit();
    m_commandEdit->setPlaceholderText("输入原始 FTP 命令 (如: PASV, SYST, HELP)...");
    m_commandEdit->setEnabled(false);
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &FTPWidget::onCommandReturn);
    cmdRow->addWidget(m_commandEdit, 1);
    mainLayout->addLayout(cmdRow);

    // ── 输出区 ──
    auto* outputGroup = new QGroupBox("FTP 会话输出");
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(4);

    m_outputText = new QPlainTextEdit();
    m_outputText->setReadOnly(true);
    m_outputText->setMaximumBlockCount(5000);
    outputLayout->addWidget(m_outputText, 1);

    auto* clearRow = new QHBoxLayout();
    clearRow->addStretch();
    m_clearOutputBtn = new QPushButton("清空输出");
    m_clearOutputBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: 1px solid #30363D; padding: 4px 12px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    clearRow->addWidget(m_clearOutputBtn);
    outputLayout->addLayout(clearRow);

    mainLayout->addWidget(outputGroup, 1);
}

// ─── Signal Connections ────────────────────────────────────────────────

void FTPWidget::setupConnections()
{
    connect(m_connectBtn, &QPushButton::clicked, this, &FTPWidget::onConnect);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &FTPWidget::onDisconnect);
    connect(m_listBtn, &QPushButton::clicked, this, &FTPWidget::onListDir);
    connect(m_browseBtn, &QPushButton::clicked, this, &FTPWidget::onBrowseLocal);
    connect(m_uploadBtn, &QPushButton::clicked, this, &FTPWidget::onUpload);
    connect(m_downloadBtn, &QPushButton::clicked, this, &FTPWidget::onDownload);
    connect(m_clearOutputBtn, &QPushButton::clicked, this, &FTPWidget::onClearOutput);
}

// ─── Output Helper ─────────────────────────────────────────────────────

void FTPWidget::appendOutput(const QString& text, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_outputText->appendHtml(
        QString("<span style='color:#8C8C8C;'>[%1]</span> "
                "<span style='color:%2;'>%3</span>")
            .arg(timestamp, color, text.toHtmlEscaped()));
}

// ─── Slot: Connect ─────────────────────────────────────────────────────

void FTPWidget::onConnect()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 FTP 服务器地址。");
        return;
    }

    if (m_connected) {
        onDisconnect();
    }

    appendOutput("正在连接到 " + host + ":" + QString::number(m_portSpin->value()) + " ...", "#D29922");

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FTPWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FTPWidget::onProcessFinished);

    // 使用 curl 进行 FTP 连接测试
    QString user = m_userEdit->text().trimmed();
    if (user.isEmpty()) user = "anonymous";
    QString pass = m_passEdit->text().trimmed();
    if (pass.isEmpty()) pass = "anonymous@";

    QString url = QString("ftp://%1:%2@%3:%4/")
                      .arg(user, pass, host)
                      .arg(m_portSpin->value());

    m_process->start("curl", QStringList() << "-s" << "--list-only" << url << "--max-time" << "10");
}

// ─── Slot: Disconnect ──────────────────────────────────────────────────

void FTPWidget::onDisconnect()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_connected = false;
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
    m_listBtn->setEnabled(false);
    m_uploadBtn->setEnabled(false);
    m_downloadBtn->setEnabled(false);
    m_commandEdit->setEnabled(false);
    m_statusLabel->setText("未连接");
    
    m_remoteTree->clear();

    appendOutput("已断开连接", "#F85149");
    Logger::instance().info("FTP", "已断开 FTP 连接");
}

// ─── Slot: List Directory ──────────────────────────────────────────────

void FTPWidget::onListDir()
{
    if (!m_connected || !m_process) return;

    QString path = m_remotePathEdit->text().trimmed();
    if (path.isEmpty()) path = "/";

    QString host = m_hostEdit->text().trimmed();
    QString user = m_userEdit->text().trimmed();
    if (user.isEmpty()) user = "anonymous";
    QString pass = m_passEdit->text().trimmed();
    if (pass.isEmpty()) pass = "anonymous@";

    QString url = QString("ftp://%1:%2@%3:%4%5")
                      .arg(user, pass, host)
                      .arg(m_portSpin->value())
                      .arg(path);

    appendOutput("列出目录: " + path, "#58A6FF");

    // 重新启动 curl 进程列目录
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FTPWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FTPWidget::onProcessFinished);
    m_process->start("curl", QStringList() << "-s" << "--list-only" << url << "--max-time" << "15");
}

// ─── Slot: Browse Local ────────────────────────────────────────────────

void FTPWidget::onBrowseLocal()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择本地目录",
                                                     m_localPathEdit->text());
    if (!dir.isEmpty()) {
        m_localPathEdit->setText(dir);
    }
}

// ─── Slot: Upload ──────────────────────────────────────────────────────

void FTPWidget::onUpload()
{
    if (!m_connected) {
        QMessageBox::warning(this, "未连接", "请先连接到 FTP 服务器。");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "选择要上传的文件",
                                                     m_localPathEdit->text());
    if (filePath.isEmpty()) return;

    QString host = m_hostEdit->text().trimmed();
    QString user = m_userEdit->text().trimmed();
    if (user.isEmpty()) user = "anonymous";
    QString pass = m_passEdit->text().trimmed();
    if (pass.isEmpty()) pass = "anonymous@";

    QString remotePath = m_remotePathEdit->text().trimmed();
    if (remotePath.isEmpty()) remotePath = "/";

    QString url = QString("ftp://%1:%2@%3:%4%5/")
                      .arg(user, pass, host)
                      .arg(m_portSpin->value())
                      .arg(remotePath);

    QFileInfo fi(filePath);
    appendOutput("上传: " + fi.fileName() + " ...", "#D29922");

    QProcess* uploadProc = new QProcess(this);
    connect(uploadProc, &QProcess::readyReadStandardOutput, this, [this, uploadProc]() {
        appendOutput(QString::fromUtf8(uploadProc->readAllStandardOutput()), "#E6EDF3");
    });
    connect(uploadProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uploadProc, fileName = fi.fileName()](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            appendOutput("上传完成: " + fileName, "#3FB950");
        } else {
            QString err = QString::fromUtf8(uploadProc->readAllStandardError());
            appendOutput("上传失败: " + err, "#F85149");
        }
        uploadProc->deleteLater();
    });

    uploadProc->start("curl", QStringList() << "-s" << "-T" << filePath << url << "--max-time" << "120");
    Logger::instance().info("FTP", "上传文件: " + fi.fileName());
}

// ─── Slot: Download ────────────────────────────────────────────────────

void FTPWidget::onDownload()
{
    if (!m_connected) {
        QMessageBox::warning(this, "未连接", "请先连接到 FTP 服务器。");
        return;
    }

    QTreeWidgetItem* item = m_remoteTree->currentItem();
    QString fileName;
    if (item) {
        fileName = item->text(0);
    } else {
        fileName = QInputDialog::getText(this, "下载文件", "输入远程文件名:");
    }
    if (fileName.isEmpty()) return;

    QString savePath = QFileDialog::getSaveFileName(this, "保存文件到",
                                                      m_localPathEdit->text() + "/" + fileName);
    if (savePath.isEmpty()) return;

    QString host = m_hostEdit->text().trimmed();
    QString user = m_userEdit->text().trimmed();
    if (user.isEmpty()) user = "anonymous";
    QString pass = m_passEdit->text().trimmed();
    if (pass.isEmpty()) pass = "anonymous@";

    QString remotePath = m_remotePathEdit->text().trimmed();
    if (remotePath.isEmpty()) remotePath = "/";
    if (!remotePath.endsWith("/")) remotePath += "/";

    QString url = QString("ftp://%1:%2@%3:%4%5%6")
                      .arg(user, pass, host)
                      .arg(m_portSpin->value())
                      .arg(remotePath, fileName);

    appendOutput("下载: " + fileName + " ...", "#D29922");

    QProcess* dlProc = new QProcess(this);
    connect(dlProc, &QProcess::readyReadStandardOutput, this, [this, dlProc]() {
        appendOutput(QString::fromUtf8(dlProc->readAllStandardOutput()), "#E6EDF3");
    });
    connect(dlProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, dlProc, fileName, savePath](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            appendOutput("下载完成: " + fileName + " → " + savePath, "#3FB950");
        } else {
            QString err = QString::fromUtf8(dlProc->readAllStandardError());
            appendOutput("下载失败: " + err, "#F85149");
        }
        dlProc->deleteLater();
    });

    dlProc->start("curl", QStringList() << "-s" << "-o" << savePath << url << "--max-time" << "120");
    Logger::instance().info("FTP", "下载文件: " + fileName);
}

// ─── Slot: Process Ready Read ──────────────────────────────────────────

void FTPWidget::onProcessReadyRead()
{
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    appendOutput(output, "#E6EDF3");
}

// ─── Slot: Process Finished ────────────────────────────────────────────

void FTPWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (!m_process) return;

    QString errOutput = QString::fromUtf8(m_process->readAllStandardError());
    if (!errOutput.isEmpty()) {
        appendOutput(errOutput, "#F85149");
    }

    if (!m_connected) {
        // 初始连接尝试
        if (exitCode == 0) {
            m_connected = true;
            m_connectBtn->setEnabled(false);
            m_disconnectBtn->setEnabled(true);
            m_listBtn->setEnabled(true);
            m_uploadBtn->setEnabled(true);
            m_downloadBtn->setEnabled(true);
            m_commandEdit->setEnabled(true);
            m_statusLabel->setText("已连接 - " + m_hostEdit->text());
            
            appendOutput("连接成功!", "#3FB950");
            Logger::instance().info("FTP", "已连接到 " + m_hostEdit->text());
            // 自动列出根目录
            onListDir();
        } else {
            appendOutput("连接失败: " + errOutput, "#F85149");
            Logger::instance().error("FTP", "连接失败: " + errOutput);
            m_connectBtn->setEnabled(true);
            m_disconnectBtn->setEnabled(false);
        }
    } else {
        // 列出目录结果 - 解析文件列表
        if (exitCode == 0) {
            m_remoteTree->clear();
            QString output = QString::fromUtf8(m_process->readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (trimmed.isEmpty()) continue;
                auto* item = new QTreeWidgetItem(m_remoteTree);
                item->setText(0, trimmed);
                item->setText(1, "-");
                item->setText(2, "-");
            }
        }
    }

    // 不要在 onProcessFinished 中删除进程，它由 QProcess parent 管理
}

// ─── Slot: Command Return ──────────────────────────────────────────────

void FTPWidget::onCommandReturn()
{
    if (!m_connected) return;
    QString cmd = m_commandEdit->text().trimmed();
    if (cmd.isEmpty()) return;

    appendOutput("> " + cmd, "#58A6FF");
    m_commandEdit->clear();

    // 使用 curl 执行自定义 FTP 命令
    QString host = m_hostEdit->text().trimmed();
    QString user = m_userEdit->text().trimmed();
    if (user.isEmpty()) user = "anonymous";
    QString pass = m_passEdit->text().trimmed();
    if (pass.isEmpty()) pass = "anonymous@";

    QString url = QString("ftp://%1:%2@%3:%4/")
                      .arg(user, pass, host)
                      .arg(m_portSpin->value());

    QProcess* cmdProc = new QProcess(this);
    connect(cmdProc, &QProcess::readyReadStandardOutput, this, [this, cmdProc]() {
        appendOutput(QString::fromUtf8(cmdProc->readAllStandardOutput()), "#E6EDF3");
    });
    connect(cmdProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, cmdProc](int, QProcess::ExitStatus) {
        QString err = QString::fromUtf8(cmdProc->readAllStandardError());
        if (!err.isEmpty()) appendOutput(err, "#F85149");
        cmdProc->deleteLater();
    });

    cmdProc->start("curl", QStringList() << "-s" << "-Q" << cmd << url << "--max-time" << "15");
}

// ─── Slot: Clear Output ────────────────────────────────────────────────

void FTPWidget::onClearOutput()
{
    m_outputText->clear();
}
