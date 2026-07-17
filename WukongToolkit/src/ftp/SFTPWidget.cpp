#include "ftp/SFTPWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>

// ═══════════════════════════════════════════════════════════════════════════
// SFTPWidget
// ═══════════════════════════════════════════════════════════════════════════

SFTPWidget::SFTPWidget(QWidget* parent)
    : QWidget(parent)
    , m_localTree(nullptr)
    , m_localModel(nullptr)
    , m_localPathEdit(nullptr)
    , m_localUpBtn(nullptr)
    , m_localHomeBtn(nullptr)
    , m_localRefreshBtn(nullptr)
    , m_remoteTree(nullptr)
    , m_remotePathEdit(nullptr)
    , m_remoteUpBtn(nullptr)
    , m_remoteHomeBtn(nullptr)
    , m_remoteRefreshBtn(nullptr)
    , m_remoteDeleteBtn(nullptr)
    , m_remoteRenameBtn(nullptr)
    , m_remoteMkdirBtn(nullptr)
    , m_uploadBtn(nullptr)
    , m_downloadBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_transferTable(nullptr)
    , m_remoteCurrentPath("/")
    , m_remoteHomePath("/")
    , m_remoteConnected(false)
{
    setupUI();
    setupConnections();
}

SFTPWidget::~SFTPWidget()
{
}

// ─── Public Interface ──────────────────────────────────────────────────

void SFTPWidget::setSessionId(const QString& sessionId)
{
    m_sessionId = sessionId;
    m_remoteConnected = !sessionId.isEmpty();
    Logger::instance().info("SFTP", QString("Session set to: %1").arg(sessionId));

    if (m_remoteConnected) {
        m_remoteCurrentPath = m_remoteHomePath;
        requestRemoteDirectory(m_remoteCurrentPath);
    }
}

QString SFTPWidget::sessionId() const
{
    return m_sessionId;
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void SFTPWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ── Dual-pane file browser ──
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet(
        "QSplitter::handle { background-color: #30363D; width: 2px; }"
    );

    auto* localPane = new QWidget();
    setupLocalPane(localPane);
    splitter->addWidget(localPane);

    auto* remotePane = new QWidget();
    setupRemotePane(remotePane);
    splitter->addWidget(remotePane);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // ── Transfer buttons ──
    setupTransferPanel(mainLayout);

    // ── Progress panel ──
    setupProgressPanel(mainLayout);

    // ── Transfer queue table ──
    auto* transferGroup = new QGroupBox("传输队列");
    auto* transferLayout = new QVBoxLayout(transferGroup);

    m_transferTable = new QTableWidget(0, 5);
    m_transferTable->setHorizontalHeaderLabels({"方向", "本地路径", "远程路径", "进度", "状态"});
    m_transferTable->horizontalHeader()->setStretchLastSection(true);
    m_transferTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_transferTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_transferTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_transferTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_transferTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transferTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_transferTable->setAlternatingRowColors(true);
    m_transferTable->setMaximumHeight(120);
    m_transferTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 2px 4px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 3px 6px; font-size: 11px; font-weight: bold;"
        "}"
        "QTableWidget::item:alternate { background-color: #161B22; }"
    );
    transferLayout->addWidget(m_transferTable);
    mainLayout->addWidget(transferGroup);
}

void SFTPWidget::setupLocalPane(QWidget* parent)
{
    auto* layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* paneLabel = new QLabel("本地文件系统");
    paneLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #58A6FF; padding: 2px 0;");
    layout->addWidget(paneLabel);

    // ── Path navigation bar ──
    auto* navLayout = new QHBoxLayout();
    navLayout->setSpacing(4);

    m_localUpBtn = new QPushButton("⬆");
    m_localUpBtn->setToolTip("上级目录");
    m_localUpBtn->setFixedSize(28, 28);
    m_localUpBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; border-radius: 3px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );

    m_localHomeBtn = new QPushButton("⌂");
    m_localHomeBtn->setToolTip("主目录");
    m_localHomeBtn->setFixedSize(28, 28);
    m_localHomeBtn->setStyleSheet(m_localUpBtn->styleSheet());

    m_localPathEdit = new QLineEdit();
    m_localPathEdit->setPlaceholderText("本地路径...");
    m_localPathEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
    );

    m_localRefreshBtn = new QPushButton("↻");
    m_localRefreshBtn->setToolTip("刷新");
    m_localRefreshBtn->setFixedSize(28, 28);
    m_localRefreshBtn->setStyleSheet(m_localUpBtn->styleSheet());

    navLayout->addWidget(m_localUpBtn);
    navLayout->addWidget(m_localHomeBtn);
    navLayout->addWidget(m_localPathEdit, 1);
    navLayout->addWidget(m_localRefreshBtn);
    layout->addLayout(navLayout);

    // ── Local tree view ──
    m_localModel = new QFileSystemModel(this);
    m_localModel->setRootPath(QDir::rootPath());
    m_localModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

    m_localTree = new QTreeView();
    m_localTree->setModel(m_localModel);
    m_localTree->setRootIndex(m_localModel->index(QDir::homePath()));
    m_localTree->setSortingEnabled(true);
    m_localTree->sortByColumn(0, Qt::AscendingOrder);
    m_localTree->setAnimated(false);
    m_localTree->setIndentation(16);
    m_localTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_localTree->hideColumn(1); // Size
    m_localTree->hideColumn(2); // Type
    m_localTree->hideColumn(3); // Date Modified
    m_localTree->setStyleSheet(
        "QTreeView {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "QTreeView::item { padding: 2px 4px; }"
        "QTreeView::item:hover { background-color: #2B2D30; }"
        "QTreeView::item:selected { background-color: #30363D; color: #58A6FF; }"
        "QTreeView::branch { background-color: #0D1117; }"
        "QTreeView::branch:has-children:!has-siblings:closed,"
        "QTreeView::branch:closed:has-children:has-siblings {"
        "  border-image: none;"
        "}"
    );

    m_localPathEdit->setText(QDir::homePath());
    layout->addWidget(m_localTree, 1);
}

void SFTPWidget::setupRemotePane(QWidget* parent)
{
    auto* layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* paneLabel = new QLabel("远程文件系统");
    paneLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #3FB950; padding: 2px 0;");
    layout->addWidget(paneLabel);

    // ── Path navigation bar ──
    auto* navLayout = new QHBoxLayout();
    navLayout->setSpacing(4);

    m_remoteUpBtn = new QPushButton("⬆");
    m_remoteUpBtn->setToolTip("上级目录");
    m_remoteUpBtn->setFixedSize(28, 28);
    m_remoteUpBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; border-radius: 3px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
    );

    m_remoteHomeBtn = new QPushButton("⌂");
    m_remoteHomeBtn->setToolTip("远程主目录");
    m_remoteHomeBtn->setFixedSize(28, 28);
    m_remoteHomeBtn->setStyleSheet(m_remoteUpBtn->styleSheet());

    m_remotePathEdit = new QLineEdit();
    m_remotePathEdit->setPlaceholderText("/");
    m_remotePathEdit->setText("/");
    m_remotePathEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
    );

    m_remoteRefreshBtn = new QPushButton("↻");
    m_remoteRefreshBtn->setToolTip("刷新");
    m_remoteRefreshBtn->setFixedSize(28, 28);
    m_remoteRefreshBtn->setStyleSheet(m_remoteUpBtn->styleSheet());

    navLayout->addWidget(m_remoteUpBtn);
    navLayout->addWidget(m_remoteHomeBtn);
    navLayout->addWidget(m_remotePathEdit, 1);
    navLayout->addWidget(m_remoteRefreshBtn);
    layout->addLayout(navLayout);

    // ── Remote tree widget ──
    m_remoteTree = new QTreeWidget();
    m_remoteTree->setColumnCount(4);
    m_remoteTree->setHeaderLabels({"名称", "大小", "类型", "修改时间"});
    m_remoteTree->header()->setStretchLastSection(true);
    m_remoteTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_remoteTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_remoteTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_remoteTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_remoteTree->setSortingEnabled(true);
    m_remoteTree->sortByColumn(0, Qt::AscendingOrder);
    m_remoteTree->setIndentation(16);
    m_remoteTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_remoteTree->setRootIsDecorated(true);
    m_remoteTree->setStyleSheet(
        "QTreeWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "QTreeWidget::item { padding: 2px 4px; }"
        "QTreeWidget::item:hover { background-color: #2B2D30; }"
        "QTreeWidget::item:selected { background-color: #30363D; color: #3FB950; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 3px 6px; font-size: 11px; font-weight: bold;"
        "}"
    );
    layout->addWidget(m_remoteTree, 1);

    // ── Remote operations buttons ──
    auto* opsLayout = new QHBoxLayout();
    opsLayout->setSpacing(4);

    auto remoteOpBtnStyle = QString(
        "QPushButton {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 10px;"
        "  border-radius: 3px; font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #30363D; }"
        "QPushButton:disabled { background-color: #0D1117; color: #484F58; }"
    );

    m_remoteDeleteBtn = new QPushButton("删除");
    m_remoteDeleteBtn->setToolTip("删除选中的远程文件/目录");
    

    m_remoteRenameBtn = new QPushButton("重命名");
    m_remoteRenameBtn->setToolTip("重命名选中的远程文件/目录");
    

    m_remoteMkdirBtn = new QPushButton("新建目录");
    m_remoteMkdirBtn->setToolTip("在远程当前路径创建目录");
    

    opsLayout->addWidget(m_remoteDeleteBtn);
    opsLayout->addWidget(m_remoteRenameBtn);
    opsLayout->addWidget(m_remoteMkdirBtn);
    opsLayout->addStretch();
    layout->addLayout(opsLayout);
}

void SFTPWidget::setupTransferPanel(QVBoxLayout* mainLayout)
{
    auto* layout = new QHBoxLayout();
    layout->setSpacing(8);

    auto transferBtnStyle = QString(
        "QPushButton {"
        "  border: none; padding: 8px 20px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:disabled { background-color: #484F58; color: #8B949E; }"
    );

    m_uploadBtn = new QPushButton("⬆ 上传");
    m_uploadBtn->setToolTip("将选中的本地文件上传到远程当前目录");
    m_uploadBtn->setStyleSheet(
        transferBtnStyle +
        "QPushButton { background-color: #58A6FF; color: white; }"
        "QPushButton:hover { background-color: #79C0FF; }"
    );

    m_downloadBtn = new QPushButton("⬇ 下载");
    m_downloadBtn->setToolTip("将选中的远程文件下载到本地当前目录");
    m_downloadBtn->setStyleSheet(
        transferBtnStyle +
        "QPushButton { background-color: #3FB950; color: white; }"
        "QPushButton:hover { background-color: #56D364; }"
    );

    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setToolTip("取消当前传输");
    m_cancelBtn->setStyleSheet(
        transferBtnStyle +
        "QPushButton { background-color: #F85149; color: white; }"
        "QPushButton:hover { background-color: #FF7B72; }"
    );
    m_cancelBtn->setEnabled(false);

    layout->addStretch();
    layout->addWidget(m_uploadBtn);
    layout->addWidget(m_downloadBtn);
    layout->addWidget(m_cancelBtn);
    layout->addStretch();

    mainLayout->addLayout(layout);
}

void SFTPWidget::setupProgressPanel(QVBoxLayout* mainLayout)
{
    auto* layout = new QHBoxLayout();
    layout->setSpacing(8);

    m_progressLabel = new QLabel("就绪");
    

    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(18);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; border-radius: 3px;"
        "  font-size: 11px; text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #58A6FF; border-radius: 2px;"
        "}"
    );

    layout->addWidget(m_progressLabel);
    layout->addWidget(m_progressBar, 1);

    mainLayout->addLayout(layout);
}

// ─── Connections ───────────────────────────────────────────────────────

void SFTPWidget::setupConnections()
{
    // Local pane
    connect(m_localTree, &QTreeView::clicked, this, &SFTPWidget::onLocalTreeClicked);
    connect(m_localTree, &QTreeView::doubleClicked, this, &SFTPWidget::onLocalTreeDoubleClicked);
    connect(m_localPathEdit, &QLineEdit::returnPressed, this, &SFTPWidget::onLocalPathChanged);
    connect(m_localUpBtn, &QPushButton::clicked, this, &SFTPWidget::onLocalGoUp);
    connect(m_localHomeBtn, &QPushButton::clicked, this, &SFTPWidget::onLocalGoHome);
    connect(m_localRefreshBtn, &QPushButton::clicked, this, &SFTPWidget::onLocalRefresh);

    // Remote pane
    connect(m_remoteTree, &QTreeWidget::itemClicked, this, &SFTPWidget::onRemoteItemClicked);
    connect(m_remoteTree, &QTreeWidget::itemDoubleClicked, this, &SFTPWidget::onRemoteItemDoubleClicked);
    connect(m_remotePathEdit, &QLineEdit::returnPressed, this, &SFTPWidget::onRemotePathChanged);
    connect(m_remoteUpBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteGoUp);
    connect(m_remoteHomeBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteGoHome);
    connect(m_remoteRefreshBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteRefresh);

    // Transfer
    connect(m_uploadBtn, &QPushButton::clicked, this, &SFTPWidget::onUploadClicked);
    connect(m_downloadBtn, &QPushButton::clicked, this, &SFTPWidget::onDownloadClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SFTPWidget::onCancelTransferClicked);

    // Remote operations
    connect(m_remoteDeleteBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteDelete);
    connect(m_remoteRenameBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteRename);
    connect(m_remoteMkdirBtn, &QPushButton::clicked, this, &SFTPWidget::onRemoteMkdir);
}

// ─── Local Pane Slots ──────────────────────────────────────────────────

void SFTPWidget::onLocalTreeClicked(const QModelIndex& index)
{
    QFileInfo info = m_localModel->fileInfo(index);
    if (info.isDir()) {
        m_localPathEdit->setText(info.absoluteFilePath());
    }
}

void SFTPWidget::onLocalTreeDoubleClicked(const QModelIndex& index)
{
    QFileInfo info = m_localModel->fileInfo(index);
    if (info.isDir()) {
        m_localTree->setRootIndex(index);
        m_localPathEdit->setText(info.absoluteFilePath());
    }
}

void SFTPWidget::onLocalPathChanged()
{
    QString path = m_localPathEdit->text().trimmed();
    QFileInfo info(path);
    if (info.exists() && info.isDir()) {
        m_localTree->setRootIndex(m_localModel->index(path));
    } else {
        m_localPathEdit->setText(QDir(m_localModel->filePath(m_localTree->rootIndex())).absolutePath());
        Logger::instance().warning("SFTP", "Invalid local path: " + path);
    }
}

void SFTPWidget::onLocalGoUp()
{
    QModelIndex current = m_localTree->rootIndex();
    QModelIndex parent = current.parent();
    if (parent.isValid()) {
        m_localTree->setRootIndex(parent);
        m_localPathEdit->setText(m_localModel->filePath(parent));
    }
}

void SFTPWidget::onLocalGoHome()
{
    QModelIndex homeIndex = m_localModel->index(QDir::homePath());
    m_localTree->setRootIndex(homeIndex);
    m_localPathEdit->setText(QDir::homePath());
}

void SFTPWidget::onLocalRefresh()
{
    // QFileSystemModel auto-refreshes, but we can force a resort
    m_localTree->sortByColumn(0, Qt::AscendingOrder);
    Logger::instance().debug("SFTP", "Local pane refreshed");
}

// ─── Remote Pane Slots ─────────────────────────────────────────────────

void SFTPWidget::onRemoteItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    m_remotePathEdit->setText(item->data(0, Qt::UserRole).toString());
}

void SFTPWidget::onRemoteItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        QString path = item->data(0, Qt::UserRole).toString();
        setRemotePath(path);
        requestRemoteDirectory(path);
    }
}

void SFTPWidget::onRemotePathChanged()
{
    QString path = m_remotePathEdit->text().trimmed();
    if (path.isEmpty()) {
        path = "/";
        m_remotePathEdit->setText(path);
    }
    setRemotePath(path);
    requestRemoteDirectory(path);
}

void SFTPWidget::onRemoteGoUp()
{
    if (m_remoteCurrentPath == "/") return;
    QDir dir(m_remoteCurrentPath);
    dir.cdUp();
    QString parentPath = dir.absolutePath();
    if (parentPath.isEmpty()) parentPath = "/";
    setRemotePath(parentPath);
    requestRemoteDirectory(parentPath);
}

void SFTPWidget::onRemoteGoHome()
{
    setRemotePath(m_remoteHomePath);
    requestRemoteDirectory(m_remoteHomePath);
}

void SFTPWidget::onRemoteRefresh()
{
    requestRemoteDirectory(m_remoteCurrentPath);
    Logger::instance().debug("SFTP", "Remote pane refreshed: " + m_remoteCurrentPath);
}

// ─── Remote Operations ─────────────────────────────────────────────────

void SFTPWidget::onRemoteDelete()
{
    QList<QTreeWidgetItem*> selected = m_remoteTree->selectedItems();
    if (selected.isEmpty()) {
        Logger::instance().warning("SFTP", "No remote file selected for deletion");
        return;
    }

    QStringList names;
    for (auto* item : selected) {
        names.append(item->text(0));
    }

    QString confirmMsg = QString("确认删除以下远程文件/目录?\n\n%1").arg(names.join("\n"));
    int ret = QMessageBox::warning(this, "确认删除", confirmMsg,
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    QVariantMap params;
    QVariantList paths;
    for (auto* item : selected) {
        paths.append(item->data(0, Qt::UserRole).toString());
    }
    params["paths"] = paths;
    params["path"] = m_remoteCurrentPath;

    emit sftpCommandRequested("delete", params);
    Logger::instance().info("SFTP", QString("Requesting delete of %1 remote item(s)").arg(paths.size()));
}

void SFTPWidget::onRemoteRename()
{
    QList<QTreeWidgetItem*> selected = m_remoteTree->selectedItems();
    if (selected.isEmpty()) {
        Logger::instance().warning("SFTP", "No remote file selected for rename");
        return;
    }

    QTreeWidgetItem* item = selected.first();
    QString oldName = item->text(0);
    bool ok = false;
    QString newName = QInputDialog::getText(this, "重命名", "新名称:",
                                            QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName) return;

    QVariantMap params;
    params["oldPath"] = item->data(0, Qt::UserRole).toString();
    params["newName"] = newName;
    params["path"] = m_remoteCurrentPath;

    emit sftpCommandRequested("rename", params);
    Logger::instance().info("SFTP", QString("Requesting rename: %1 -> %2").arg(oldName, newName));
}

void SFTPWidget::onRemoteMkdir()
{
    bool ok = false;
    QString dirName = QInputDialog::getText(this, "新建目录", "目录名称:",
                                            QLineEdit::Normal, "", &ok);
    if (!ok || dirName.isEmpty()) return;

    QVariantMap params;
    params["path"] = m_remoteCurrentPath;
    params["name"] = dirName;

    emit sftpCommandRequested("mkdir", params);
    Logger::instance().info("SFTP", QString("Requesting mkdir: %1/%2").arg(m_remoteCurrentPath, dirName));
}

// ─── Transfer Slots ────────────────────────────────────────────────────

void SFTPWidget::onUploadClicked()
{
    QModelIndexList selected = m_localTree->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        Logger::instance().warning("SFTP", "No local file selected for upload");
        return;
    }

    if (!m_remoteConnected) {
        Logger::instance().warning("SFTP", "No remote session connected");
        return;
    }

    for (const QModelIndex& index : selected) {
        QString localPath = m_localModel->filePath(index);
        QFileInfo info(localPath);
        if (info.isFile()) {
            QString remotePath = m_remoteCurrentPath + "/" + info.fileName();
            addTransferEntry(localPath, remotePath, "上传");
            emit uploadRequested(localPath, remotePath);
            Logger::instance().info("SFTP", QString("Upload requested: %1 -> %2").arg(localPath, remotePath));
        }
    }
}

void SFTPWidget::onDownloadClicked()
{
    QList<QTreeWidgetItem*> selected = m_remoteTree->selectedItems();
    if (selected.isEmpty()) {
        Logger::instance().warning("SFTP", "No remote file selected for download");
        return;
    }

    if (!m_remoteConnected) {
        Logger::instance().warning("SFTP", "No remote session connected");
        return;
    }

    QString localDir = m_localModel->filePath(m_localTree->rootIndex());

    for (auto* item : selected) {
        bool isDir = item->data(0, Qt::UserRole + 1).toBool();
        if (!isDir) {
            QString remotePath = item->data(0, Qt::UserRole).toString();
            QString localPath = localDir + "/" + item->text(0);
            addTransferEntry(localPath, remotePath, "下载");
            emit downloadRequested(remotePath, localPath);
            Logger::instance().info("SFTP", QString("Download requested: %1 -> %2").arg(remotePath, localPath));
        }
    }
}

void SFTPWidget::onCancelTransferClicked()
{
    QVariantMap params;
    emit sftpCommandRequested("cancel", params);
    Logger::instance().info("SFTP", "Transfer cancel requested");
}

// ─── Remote Directory Listing ──────────────────────────────────────────

void SFTPWidget::onRemoteDirectoryListed(QString path, QVariantList entries)
{
    if (path != m_remoteCurrentPath) {
        // Response for an old path, ignore
        return;
    }

    populateRemoteTree(entries);
    Logger::instance().debug("SFTP", QString("Remote directory listed: %1 (%2 entries)").arg(path).arg(entries.size()));
}

void SFTPWidget::requestRemoteDirectory(const QString& path)
{
    if (!m_remoteConnected) {
        m_remoteTree->clear();
        return;
    }

    QVariantMap params;
    params["path"] = path;
    emit sftpCommandRequested("list", params);
}

void SFTPWidget::populateRemoteTree(const QVariantList& entries)
{
    m_remoteTree->clear();

    for (const QVariant& entryVar : entries) {
        QVariantMap entry = entryVar.toMap();
        QString name = entry.value("name").toString();
        bool isDir = entry.value("isDir", false).toBool();
        qint64 size = entry.value("size", 0).toLongLong();
        QString type = entry.value("type").toString();
        QString modified = entry.value("modified").toString();
        QString fullPath = entry.value("path").toString();

        auto* item = new QTreeWidgetItem();
        item->setText(0, name);

        if (isDir) {
            item->setText(1, ""); // Directories don't show size
            item->setText(2, "目录");
            item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            // Format file size
            QString sizeStr;
            if (size < 1024) {
                sizeStr = QString::number(size) + " B";
            } else if (size < 1024 * 1024) {
                sizeStr = QString::number(size / 1024.0, 'f', 1) + " KB";
            } else if (size < 1024LL * 1024 * 1024) {
                sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
            } else {
                sizeStr = QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
            }
            item->setText(1, sizeStr);
            item->setText(2, type.isEmpty() ? "文件" : type);
            item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        }

        item->setText(3, modified);

        // Store full path and directory flag
        item->setData(0, Qt::UserRole, fullPath);
        item->setData(0, Qt::UserRole + 1, isDir);

        m_remoteTree->addTopLevelItem(item);
    }

    m_remotePathEdit->setText(m_remoteCurrentPath);
}

void SFTPWidget::setRemotePath(const QString& path)
{
    m_remoteCurrentPath = path;
    if (m_remoteCurrentPath.isEmpty()) {
        m_remoteCurrentPath = "/";
    }
}

// ─── Transfer Queue ────────────────────────────────────────────────────

void SFTPWidget::addTransferEntry(const QString& localPath, const QString& remotePath,
                                  const QString& direction)
{
    int row = m_transferTable->rowCount();
    m_transferTable->insertRow(row);

    auto* dirItem = new QTableWidgetItem(direction);
    if (direction == "上传") {
        dirItem->setForeground(QColor(0x58, 0xA6, 0xFF));
    } else {
        dirItem->setForeground(QColor(0x3F, 0xB9, 0x50));
    }

    auto* localItem = new QTableWidgetItem(localPath);
    auto* remoteItem = new QTableWidgetItem(remotePath);
    auto* progressItem = new QTableWidgetItem("0%");
    auto* statusItem = new QTableWidgetItem("等待中");

    m_transferTable->setItem(row, 0, dirItem);
    m_transferTable->setItem(row, 1, localItem);
    m_transferTable->setItem(row, 2, remoteItem);
    m_transferTable->setItem(row, 3, progressItem);
    m_transferTable->setItem(row, 4, statusItem);

    m_transferTable->scrollToBottom();
}

void SFTPWidget::updateTransferProgress(int row, qint64 transferred, qint64 total)
{
    if (row < 0 || row >= m_transferTable->rowCount()) return;

    int percent = (total > 0) ? static_cast<int>(transferred * 100 / total) : 0;
    auto* progressItem = m_transferTable->item(row, 3);
    if (progressItem) {
        progressItem->setText(QString("%1%").arg(percent));
    }

    auto* statusItem = m_transferTable->item(row, 4);
    if (statusItem) {
        if (percent >= 100) {
            statusItem->setText("完成");
            statusItem->setForeground(QColor(0x3F, 0xB9, 0x50));
        } else {
            statusItem->setText("传输中");
            statusItem->setForeground(QColor(0x58, 0xA6, 0xFF));
        }
    }

    // Update overall progress bar
    m_progressBar->setValue(percent);
    m_progressLabel->setText(QString("传输中: %1%").arg(percent));
}