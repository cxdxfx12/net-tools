#include "ftp/SCPWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>

// ============================================================================
// 样式常量
// ============================================================================


// ============================================================================
// SCPWidget 实现
// ============================================================================

SCPWidget::SCPWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_isRunning(false)
{
    setupUI();
    setupConnections();
}

SCPWidget::~SCPWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void SCPWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 左侧: 操作区 ──
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 连接信息
    auto* connGroup = new QGroupBox("远程服务器");
    auto* connLayout = new QVBoxLayout(connGroup);
    connLayout->setSpacing(6);

    auto* hostRow = new QHBoxLayout();
    hostRow->setSpacing(8);
    auto* hostLabel = new QLabel("主机:");
    
    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("192.168.1.100");
    hostRow->addWidget(hostLabel);
    hostRow->addWidget(m_hostEdit, 1);

    auto* portLabel = new QLabel("端口:");
    
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    m_portSpin->setFixedWidth(80);
    hostRow->addWidget(portLabel);
    hostRow->addWidget(m_portSpin);
    connLayout->addLayout(hostRow);

    auto* userRow = new QHBoxLayout();
    userRow->setSpacing(8);
    auto* userLabel = new QLabel("用户:");
    
    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("root");
    userRow->addWidget(userLabel);
    userRow->addWidget(m_userEdit, 1);
    connLayout->addLayout(userRow);

    auto* remoteRow = new QHBoxLayout();
    remoteRow->setSpacing(8);
    auto* remoteLabel = new QLabel("远程路径:");
    
    m_remotePathEdit = new QLineEdit();
    m_remotePathEdit->setPlaceholderText("/home/user/ 或 /tmp/file.txt");
    remoteRow->addWidget(remoteLabel);
    remoteRow->addWidget(m_remotePathEdit, 1);
    connLayout->addLayout(remoteRow);

    leftLayout->addWidget(connGroup);

    // 本地路径
    auto* localGroup = new QGroupBox("本地路径");
    auto* localLayout = new QHBoxLayout(localGroup);
    localLayout->setSpacing(8);

    m_localPathEdit = new QLineEdit();
    m_localPathEdit->setText(QDir::homePath());
    localLayout->addWidget(m_localPathEdit, 1);

    m_browseLocalBtn = new QPushButton("浏览...");
    m_browseLocalBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 14px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );
    localLayout->addWidget(m_browseLocalBtn);
    leftLayout->addWidget(localGroup);

    // 操作按钮
    auto* actionGroup = new QGroupBox("操作");
    auto* actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->setSpacing(12);

    m_uploadBtn = new QPushButton("上传 (本地→远程)");
    m_uploadBtn->setFixedHeight(40);

    m_downloadBtn = new QPushButton("下载 (远程→本地)");
    m_downloadBtn->setFixedHeight(40);

    actionLayout->addWidget(m_uploadBtn);
    actionLayout->addWidget(m_downloadBtn);
    leftLayout->addWidget(actionGroup);

    // 进度
    auto* progressGroup = new QGroupBox("传输进度");
    auto* progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(4);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #161B22; border: 1px solid #30363D;"
        "  border-radius: 3px; text-align: center; color: #E6EDF3;"
        "  font-size: 12px; height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #58A6FF; border-radius: 2px;"
        "}"
    );
    progressLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("就绪");
    
    progressLayout->addWidget(m_statusLabel);
    leftLayout->addWidget(progressGroup);

    // 输出区
    auto* outputGroup = new QGroupBox("输出日志");
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
    leftLayout->addWidget(outputGroup, 1);

    // ── 右侧: 历史记录 ──
    auto* historyGroup = new QGroupBox("传输历史");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->setSpacing(4);

    m_historyList = new QListWidget();
    historyLayout->addWidget(m_historyList, 1);

    m_clearHistoryBtn = new QPushButton("清空历史");
    m_clearHistoryBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
    );
    historyLayout->addWidget(m_clearHistoryBtn);

    // ── Splitter ──
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(historyGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setStyleSheet("QSplitter::handle { background-color: #30363D; width: 2px; }");

    mainLayout->addWidget(splitter);
}

// ─── Signal Connections ────────────────────────────────────────────────

void SCPWidget::setupConnections()
{
    connect(m_uploadBtn, &QPushButton::clicked, this, &SCPWidget::onUpload);
    connect(m_downloadBtn, &QPushButton::clicked, this, &SCPWidget::onDownload);
    connect(m_browseLocalBtn, &QPushButton::clicked, this, &SCPWidget::onBrowseLocal);
    connect(m_clearOutputBtn, &QPushButton::clicked, this, &SCPWidget::onClearOutput);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, [this]() {
        m_historyItems.clear();
        m_historyList->clear();
    });
    connect(m_historyList, &QListWidget::itemClicked, this, &SCPWidget::onHistoryItemClicked);
}

// ─── Output Helper ─────────────────────────────────────────────────────

void SCPWidget::appendOutput(const QString& text, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_outputText->appendHtml(
        QString("<span style='color:#8C8C8C;'>[%1]</span> "
                "<span style='color:%2;'>%3</span>")
            .arg(timestamp, color, text.toHtmlEscaped()));
}

// ─── History ───────────────────────────────────────────────────────────

void SCPWidget::addToHistory(const QString& entry)
{
    m_historyItems.removeAll(entry);
    m_historyItems.prepend(entry);
    if (m_historyItems.size() > kMaxHistory) {
        m_historyItems.removeLast();
    }
    m_historyList->clear();
    for (const QString& item : m_historyItems) {
        m_historyList->addItem(item);
    }
}

void SCPWidget::onHistoryItemClicked(QListWidgetItem* item)
{
    if (item) {
        appendOutput("历史记录: " + item->text(), "#8B949E");
    }
}

// ─── Execute SCP ───────────────────────────────────────────────────────

void SCPWidget::executeScp(const QStringList& args, const QString& desc)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_isRunning = true;
    m_progressBar->setValue(0);
    m_statusLabel->setText("传输中...");
    

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &SCPWidget::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SCPWidget::onProcessFinished);

    m_process->start("scp", args);
    appendOutput(desc, "#58A6FF");
    Logger::instance().info("SCP", desc);
}

// ─── Slot: Upload ──────────────────────────────────────────────────────

void SCPWidget::onUpload()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入远程主机地址。");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "选择要上传的文件",
                                                     m_localPathEdit->text());
    if (filePath.isEmpty()) return;

    QString user = m_userEdit->text().trimmed();
    QString remotePath = m_remotePathEdit->text().trimmed();
    if (remotePath.isEmpty()) remotePath = "~/";

    QString target;
    if (user.isEmpty()) {
        target = host + ":" + remotePath;
    } else {
        target = user + "@" + host + ":" + remotePath;
    }

    int port = m_portSpin->value();
    QStringList args;
    if (port != 22) {
        args << "-P" << QString::number(port);
    }
    args << "-r" << filePath << target;

    QFileInfo fi(filePath);
    QString desc = QString("上传: %1 → %2:%3").arg(fi.fileName(), host, remotePath);
    executeScp(args, desc);
    addToHistory(desc);
}

// ─── Slot: Download ────────────────────────────────────────────────────

void SCPWidget::onDownload()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入远程主机地址。");
        return;
    }

    QString user = m_userEdit->text().trimmed();
    QString remotePath = m_remotePathEdit->text().trimmed();
    if (remotePath.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入远程文件路径。");
        return;
    }

    QString source;
    if (user.isEmpty()) {
        source = host + ":" + remotePath;
    } else {
        source = user + "@" + host + ":" + remotePath;
    }

    int port = m_portSpin->value();
    QStringList args;
    if (port != 22) {
        args << "-P" << QString::number(port);
    }
    args << "-r" << source << m_localPathEdit->text();

    QString desc = QString("下载: %1:%2 → %3").arg(host, remotePath, m_localPathEdit->text());
    executeScp(args, desc);
    addToHistory(desc);
}

// ─── Slot: Browse Local ────────────────────────────────────────────────

void SCPWidget::onBrowseLocal()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择本地目录",
                                                     m_localPathEdit->text());
    if (!dir.isEmpty()) {
        m_localPathEdit->setText(dir);
    }
}

// ─── Slot: Browse Remote Path ──────────────────────────────────────────

void SCPWidget::onBrowseRemotePath()
{
    bool ok;
    QString path = QInputDialog::getText(this, "远程路径",
                                          "输入远程目录或文件路径:",
                                          QLineEdit::Normal, "/home/", &ok);
    if (ok && !path.isEmpty()) {
        m_remotePathEdit->setText(path);
    }
}

// ─── Slot: Process Ready Read ──────────────────────────────────────────

void SCPWidget::onProcessReadyRead()
{
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!output.isEmpty()) {
        appendOutput(output.trimmed(), "#E6EDF3");
    }
}

// ─── Slot: Process Finished ────────────────────────────────────────────

void SCPWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isRunning = false;

    if (!m_process) return;

    QString errOutput = QString::fromUtf8(m_process->readAllStandardError());
    if (!errOutput.isEmpty()) {
        appendOutput(errOutput.trimmed(), "#F85149");
    }

    m_process->deleteLater();
    m_process = nullptr;

    if (exitCode == 0) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("传输完成");
        
        appendOutput("传输完成", "#3FB950");
        Logger::instance().info("SCP", "传输完成");
    } else {
        m_progressBar->setValue(0);
        m_statusLabel->setText("传输失败");
        
        appendOutput("传输失败 (退出码: " + QString::number(exitCode) + ")", "#F85149");
        Logger::instance().error("SCP", "传输失败, 退出码: " + QString::number(exitCode));
    }
}

// ─── Slot: Clear Output ────────────────────────────────────────────────

void SCPWidget::onClearOutput()
{
    m_outputText->clear();
}
