#include "http/HttpWidget.h"
#include "log/Logger.h"

static QString formatSize(qint64 bytes);

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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QAuthenticator>
#include <QTimer>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QMenu>

// ============================================================================
// HttpWidget 实现
// ============================================================================

HttpWidget::HttpWidget(QWidget* parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    setupUI();
    setupConnections();
}

HttpWidget::~HttpWidget()
{
    // QNetworkAccessManager is parented, auto-deleted
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void HttpWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── 左侧: 请求历史 ──
    auto* historyWidget = new QWidget();
    auto* historyLayout = new QVBoxLayout(historyWidget);
    historyLayout->setContentsMargins(0, 0, 0, 0);
    historyLayout->setSpacing(4);

    auto* historyTitle = new QLabel("请求历史");
    historyTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #E6EDF3; padding: 2px 0;");
    historyLayout->addWidget(historyTitle);

    m_historyList = new QListWidget();
    m_historyList->setStyleSheet(
        "QListWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item { padding: 5px 8px; border-bottom: 1px solid #30363D; }"
        "QListWidget::item:hover { background-color: #2B2D30; }"
        "QListWidget::item:selected { background-color: #58A6FF; color: white; }"
    );
    historyLayout->addWidget(m_historyList, 1);

    m_clearHistoryBtn = new QPushButton("清除历史");
    m_clearHistoryBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F85149; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #FF7B72; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    historyLayout->addWidget(m_clearHistoryBtn);

    historyWidget->setFixedWidth(200);
    mainLayout->addWidget(historyWidget);

    // ── 右侧: 请求/响应区 ──
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    // ── 请求配置区 ──
    auto* requestGroup = new QGroupBox("HTTP 请求");
    auto* requestLayout = new QVBoxLayout(requestGroup);
    requestLayout->setSpacing(6);

    // URL 行
    auto* urlRow = new QHBoxLayout();
    urlRow->setSpacing(6);

    m_methodCombo = new QComboBox();
    m_methodCombo->addItems({"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"});
    m_methodCombo->setFixedWidth(90);
    m_methodCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px; font-weight: bold;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    urlRow->addWidget(m_methodCombo);

    m_urlEdit = new QLineEdit();
    m_urlEdit->setPlaceholderText("输入请求 URL, 例如: https://api.example.com/users");
    m_urlEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    urlRow->addWidget(m_urlEdit, 1);

    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 8px 24px; border-radius: 4px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_sendBtn->setFixedHeight(32);
    urlRow->addWidget(m_sendBtn);

    requestLayout->addLayout(urlRow);

    // 认证 + 超时行
    auto* authRow = new QHBoxLayout();
    authRow->setSpacing(8);

    m_authEnableCheck = new QCheckBox("Basic Auth");
    m_authEnableCheck->setStyleSheet(
        "QCheckBox { color: #E6EDF3; font-size: 12px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
    authRow->addWidget(m_authEnableCheck);

    m_authUserEdit = new QLineEdit();
    m_authUserEdit->setPlaceholderText("用户名");
    m_authUserEdit->setEnabled(false);
    m_authUserEdit->setFixedWidth(120);
    m_authUserEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
    );
    authRow->addWidget(m_authUserEdit);

    m_authPassEdit = new QLineEdit();
    m_authPassEdit->setPlaceholderText("密码");
    m_authPassEdit->setEchoMode(QLineEdit::Password);
    m_authPassEdit->setEnabled(false);
    m_authPassEdit->setFixedWidth(120);
    m_authPassEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
    );
    authRow->addWidget(m_authPassEdit);

    auto* timeoutLabel = new QLabel("超时:");
    
    authRow->addWidget(timeoutLabel);

    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(1, 300);
    m_timeoutSpin->setValue(30);
    m_timeoutSpin->setSuffix(" s");
    m_timeoutSpin->setFixedWidth(80);
    m_timeoutSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
    );
    authRow->addWidget(m_timeoutSpin);

    authRow->addStretch();
    requestLayout->addLayout(authRow);

    // Headers 行
    auto* headersRow = new QHBoxLayout();
    headersRow->setSpacing(6);

    auto* headersLabel = new QLabel("请求头:");
    
    headersRow->addWidget(headersLabel);

    m_addHeaderBtn = new QPushButton("+ 添加");
    m_addHeaderBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3FB950; color: white;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #56D364; }"
    );
    m_addHeaderBtn->setFixedHeight(26);
    headersRow->addWidget(m_addHeaderBtn);

    m_removeHeaderBtn = new QPushButton("- 删除");
    m_removeHeaderBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #F85149; color: white;"
        "  border: none; padding: 4px 12px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #FF7B72; }"
    );
    m_removeHeaderBtn->setFixedHeight(26);
    headersRow->addWidget(m_removeHeaderBtn);

    headersRow->addStretch();
    requestLayout->addLayout(headersRow);

    // Headers 表格
    m_headersTable = new QTableWidget(0, 2);
    m_headersTable->setHorizontalHeaderLabels({"Name", "Value"});
    m_headersTable->horizontalHeader()->setStretchLastSection(true);
    m_headersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_headersTable->setColumnWidth(0, 180);
    m_headersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_headersTable->setMaximumHeight(120);
    m_headersTable->verticalHeader()->setVisible(false);
    m_headersTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 2px 6px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 3px 6px; font-size: 12px; font-weight: bold;"
        "}"
    );
    requestLayout->addWidget(m_headersTable);

    // Content-Type + 请求体
    auto* bodyConfigRow = new QHBoxLayout();
    bodyConfigRow->setSpacing(6);

    auto* ctLabel = new QLabel("Content-Type:");
    
    bodyConfigRow->addWidget(ctLabel);

    m_contentTypeCombo = new QComboBox();
    m_contentTypeCombo->addItems({
        "application/json",
        "application/x-www-form-urlencoded",
        "text/plain",
        "text/html",
        "application/xml",
        "multipart/form-data",
        "application/octet-stream"
    });
    m_contentTypeCombo->setCurrentText("application/json");
    m_contentTypeCombo->setStyleSheet(
        "QComboBox {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 4px 8px;"
        "  border-radius: 3px; font-size: 12px;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}"
    );
    bodyConfigRow->addWidget(m_contentTypeCombo);
    bodyConfigRow->addStretch();

    requestLayout->addLayout(bodyConfigRow);

    // 请求体编辑器
    m_requestBodyEdit = new QPlainTextEdit();
    m_requestBodyEdit->setPlaceholderText("请求体 (用于 POST/PUT/PATCH)");
    m_requestBodyEdit->setMaximumHeight(120);
    m_requestBodyEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #161B22; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px;"
        "  border-radius: 3px; font-size: 13px; font-family: 'Consolas', 'Monaco', monospace;"
        "}"
    );
    requestLayout->addWidget(m_requestBodyEdit);

    rightLayout->addWidget(requestGroup);

    // ── 响应区 ──
    auto* responseGroup = new QGroupBox("响应");
    auto* responseLayout = new QVBoxLayout(responseGroup);
    responseLayout->setSpacing(6);

    // 响应信息行
    auto* responseInfoRow = new QHBoxLayout();
    responseInfoRow->setSpacing(16);

    auto* statusTitleLabel = new QLabel("状态:");
    
    responseInfoRow->addWidget(statusTitleLabel);

    m_statusCodeLabel = new QLabel("-");
    m_statusCodeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    responseInfoRow->addWidget(m_statusCodeLabel);

    auto* timeTitleLabel = new QLabel("耗时:");
    
    responseInfoRow->addWidget(timeTitleLabel);

    m_responseTimeLabel = new QLabel("-");
    
    responseInfoRow->addWidget(m_responseTimeLabel);

    auto* sizeTitleLabel = new QLabel("大小:");
    
    responseInfoRow->addWidget(sizeTitleLabel);

    m_responseSizeLabel = new QLabel("-");
    
    responseInfoRow->addWidget(m_responseSizeLabel);

    responseInfoRow->addStretch();

    m_exportBtn = new QPushButton("导出响应");
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #58A6FF; color: white;"
        "  border: none; padding: 5px 14px; border-radius: 3px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background-color: #79C0FF; }"
        "QPushButton:disabled { background-color: #484F58; }"
    );
    m_exportBtn->setEnabled(false);
    responseInfoRow->addWidget(m_exportBtn);

    responseLayout->addLayout(responseInfoRow);

    // 响应体 / 响应头 分屏
    auto* responseSplitter = new QSplitter(Qt::Vertical);

    // 响应头表格
    m_responseHeadersTable = new QTableWidget(0, 2);
    m_responseHeadersTable->setHorizontalHeaderLabels({"Name", "Value"});
    m_responseHeadersTable->horizontalHeader()->setStretchLastSection(true);
    m_responseHeadersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_responseHeadersTable->setColumnWidth(0, 200);
    m_responseHeadersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_responseHeadersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_responseHeadersTable->verticalHeader()->setVisible(false);
    m_responseHeadersTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #0D1117; color: #E6EDF3;"
        "  gridline-color: #30363D; border: 1px solid #30363D;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item { padding: 2px 6px; }"
        "QHeaderView::section {"
        "  background-color: #161B22; color: #8B949E;"
        "  border: none; border-bottom: 2px solid #30363D;"
        "  padding: 3px 6px; font-size: 12px; font-weight: bold;"
        "}"
    );
    responseSplitter->addWidget(m_responseHeadersTable);

    // 响应体编辑器
    m_responseBodyEdit = new QPlainTextEdit();
    m_responseBodyEdit->setReadOnly(true);
    m_responseBodyEdit->setPlaceholderText("响应内容将显示在此处...");
    m_responseBodyEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; padding: 5px;"
        "  border-radius: 3px; font-size: 13px; font-family: 'Consolas', 'Monaco', monospace;"
        "}"
    );

    // 响应体右键菜单
    m_responseBodyEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_responseBodyEdit, &QPlainTextEdit::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* copyAction = menu.addAction("复制");
        QAction* selectAllAction = menu.addAction("全选");
        QAction* formatJsonAction = menu.addAction("格式化 JSON");

        QAction* chosen = menu.exec(m_responseBodyEdit->viewport()->mapToGlobal(pos));
        if (chosen == copyAction) {
            m_responseBodyEdit->copy();
        } else if (chosen == selectAllAction) {
            m_responseBodyEdit->selectAll();
        } else if (chosen == formatJsonAction) {
            formatJson(m_responseBodyEdit->toPlainText().toUtf8(), m_responseBodyEdit);
        }
    });

    responseSplitter->addWidget(m_responseBodyEdit);
    responseSplitter->setStretchFactor(0, 1);
    responseSplitter->setStretchFactor(1, 3);

    responseLayout->addWidget(responseSplitter, 1);

    rightLayout->addWidget(responseGroup, 1);

    mainLayout->addWidget(rightWidget, 1);

    // 初始状态: 请求体禁用 (GET 默认)
    m_requestBodyEdit->setEnabled(false);
}

// ─── Connections ───────────────────────────────────────────────────────

void HttpWidget::setupConnections()
{
    connect(m_sendBtn, &QPushButton::clicked, this, &HttpWidget::onSend);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &HttpWidget::onSend);
    connect(m_addHeaderBtn, &QPushButton::clicked, this, &HttpWidget::onAddHeader);
    connect(m_removeHeaderBtn, &QPushButton::clicked, this, &HttpWidget::onRemoveHeader);
    connect(m_methodCombo, &QComboBox::currentTextChanged, this, &HttpWidget::onMethodChanged);
    connect(m_historyList, &QListWidget::itemClicked, this, &HttpWidget::onHistoryItemClicked);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &HttpWidget::onClearHistory);
    connect(m_exportBtn, &QPushButton::clicked, this, &HttpWidget::onExportResponse);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &HttpWidget::onNetworkReplyFinished);

    // Basic Auth checkbox
    connect(m_authEnableCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_authUserEdit->setEnabled(checked);
        m_authPassEdit->setEnabled(checked);
    });
}

// ─── 发送请求 ─────────────────────────────────────────────────────────

void HttpWidget::onSend()
{
    sendRequest();
}

void HttpWidget::sendRequest()
{
    QString urlText = m_urlEdit->text().trimmed();
    if (urlText.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入请求 URL。");
        return;
    }

    // 自动补全 http://
    if (!urlText.startsWith("http://") && !urlText.startsWith("https://")) {
        urlText = "https://" + urlText;
        m_urlEdit->setText(urlText);
    }

    QUrl url(urlText, QUrl::TolerantMode);
    if (!url.isValid()) {
        QMessageBox::warning(this, "URL 错误", "无效的 URL: " + urlText);
        return;
    }

    QString method = m_methodCombo->currentText();

    QNetworkRequest request(url);

    // 设置超时
    int timeoutMs = m_timeoutSpin->value() * 1000;
    request.setTransferTimeout(timeoutMs);

    // 构建请求头
    for (int row = 0; row < m_headersTable->rowCount(); ++row) {
        auto* nameItem = m_headersTable->item(row, 0);
        auto* valueItem = m_headersTable->item(row, 1);
        if (nameItem && valueItem) {
            QString name = nameItem->text().trimmed();
            QString value = valueItem->text().trimmed();
            if (!name.isEmpty()) {
                request.setRawHeader(name.toUtf8(), value.toUtf8());
            }
        }
    }

    // Content-Type
    if (method == "POST" || method == "PUT" || method == "PATCH") {
        QString ct = m_contentTypeCombo->currentText();
        if (!request.hasRawHeader("Content-Type")) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, ct);
        }
    }

    // Basic Auth
    if (m_authEnableCheck->isChecked()) {
        QString user = m_authUserEdit->text().trimmed();
        QString pass = m_authPassEdit->text();
        if (!user.isEmpty()) {
            QString credentials = user + ":" + pass;
            QByteArray encoded = credentials.toUtf8().toBase64();
            request.setRawHeader("Authorization", "Basic " + encoded);
        }
    }

    // 请求体
    QByteArray bodyData;
    if (method == "POST" || method == "PUT" || method == "PATCH") {
        bodyData = m_requestBodyEdit->toPlainText().toUtf8();
    }

    // 清除旧响应
    clearResponse();

    // 发送请求
    m_timer.start();

    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        reply = m_networkManager->post(request, bodyData);
    } else if (method == "PUT") {
        reply = m_networkManager->put(request, bodyData);
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    } else if (method == "PATCH") {
        // PATCH: use sendCustomRequest
        reply = m_networkManager->sendCustomRequest(request, "PATCH", bodyData);
    } else if (method == "HEAD") {
        reply = m_networkManager->head(request);
    } else if (method == "OPTIONS") {
        reply = m_networkManager->sendCustomRequest(request, "OPTIONS");
    }

    if (reply) {
        m_sendBtn->setEnabled(false);
        m_sendBtn->setText("发送中...");
        Logger::instance().info("HTTP", QString("%1 %2").arg(method, urlText));
    }

    // 添加到历史
    addToHistory(urlText, method);
}

// ─── Headers 管理 ─────────────────────────────────────────────────────

void HttpWidget::onAddHeader()
{
    int row = m_headersTable->rowCount();
    m_headersTable->insertRow(row);

    auto* nameItem = new QTableWidgetItem("");
    auto* valueItem = new QTableWidgetItem("");

    m_headersTable->setItem(row, 0, nameItem);
    m_headersTable->setItem(row, 1, valueItem);

    m_headersTable->editItem(nameItem);
}

void HttpWidget::onRemoveHeader()
{
    int currentRow = m_headersTable->currentRow();
    if (currentRow >= 0) {
        m_headersTable->removeRow(currentRow);
    } else if (m_headersTable->rowCount() > 0) {
        m_headersTable->removeRow(m_headersTable->rowCount() - 1);
    }
}

// ─── 方法变化 ─────────────────────────────────────────────────────────

void HttpWidget::onMethodChanged(const QString& method)
{
    bool needsBody = (method == "POST" || method == "PUT" || method == "PATCH");
    m_requestBodyEdit->setEnabled(needsBody);

    // 根据方法变色
    QColor methodColor;
    if (method == "GET")        methodColor = QColor(0x3F, 0xB9, 0x50);
    else if (method == "POST")  methodColor = QColor(0xD2, 0x99, 0x22);
    else if (method == "PUT")   methodColor = QColor(0x58, 0xA6, 0xFF);
    else if (method == "DELETE")methodColor = QColor(0xF8, 0x51, 0x49);
    else if (method == "PATCH") methodColor = QColor(0x9B, 0x59, 0xB6);
    else if (method == "HEAD")  methodColor = QColor(0x90, 0x90, 0x90);
    else if (method == "OPTIONS") methodColor = QColor(0x00, 0xB4, 0xD8);
    else                        methodColor = QColor(0xDC, 0xDC, 0xDC);

    m_methodCombo->setStyleSheet(
        QString("QComboBox {"
        "  background: #161B22; color: %1;"
        "  border: 1px solid #30363D; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px; font-weight: bold;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background: #161B22; color: #E6EDF3;"
        "  selection-background-color: #58A6FF;"
        "  border: 1px solid #30363D;"
        "}").arg(methodColor.name())
    );

    if (needsBody) {
        m_requestBodyEdit->setPlaceholderText("请求体 (用于 POST/PUT/PATCH)");
    }
}

// ─── 网络响应处理 ─────────────────────────────────────────────────────

void HttpWidget::onNetworkReplyFinished(QNetworkReply* reply)
{
    m_sendBtn->setEnabled(true);
    m_sendBtn->setText("发送");

    qint64 elapsed = m_timer.elapsed();

    updateResponseDisplay(reply);

    reply->deleteLater();
}

void HttpWidget::updateResponseDisplay(QNetworkReply* reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qint64 elapsed = m_timer.elapsed();

    // 状态码
    QString codeStr;
    if (statusCode > 0) {
        codeStr = QString::number(statusCode);
        QString reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        if (!reason.isEmpty()) {
            codeStr += " " + reason;
        }
    } else {
        codeStr = "Error";
    }

    m_statusCodeLabel->setText(codeStr);
    m_statusCodeLabel->setStyleSheet(
        QString("font-size: 16px; font-weight: bold; color: %1;").arg(statusColor(statusCode))
    );

    // 耗时
    if (elapsed < 1000) {
        m_responseTimeLabel->setText(QString::number(elapsed) + " ms");
    } else {
        m_responseTimeLabel->setText(QString::number(elapsed / 1000.0, 'f', 2) + " s");
    }

    // 响应体
    QByteArray responseData = reply->readAll();
    m_responseSizeLabel->setText(formatSize(responseData.size()));

    // 响应头
    m_responseHeadersTable->setRowCount(0);
    const QList<QNetworkReply::RawHeaderPair> headers = reply->rawHeaderPairs();
    for (const auto& header : headers) {
        int row = m_responseHeadersTable->rowCount();
        m_responseHeadersTable->insertRow(row);

        auto* nameItem = new QTableWidgetItem(QString::fromUtf8(header.first));
        nameItem->setForeground(QColor(0x58, 0xA6, 0xFF));
        auto* valueItem = new QTableWidgetItem(QString::fromUtf8(header.second));

        m_responseHeadersTable->setItem(row, 0, nameItem);
        m_responseHeadersTable->setItem(row, 1, valueItem);
    }

    // 响应体
    QString contentType;
    for (const auto& header : headers) {
        if (header.first.toLower() == "content-type") {
            contentType = QString::fromUtf8(header.second).toLower();
            break;
        }
    }

    if (contentType.contains("application/json") || contentType.contains("text/json")) {
        formatJson(responseData, m_responseBodyEdit);
    } else {
        m_responseBodyEdit->setPlainText(QString::fromUtf8(responseData));
    }

    m_exportBtn->setEnabled(true);

    // 网络错误
    if (reply->error() != QNetworkReply::NoError && statusCode == 0) {
        m_statusCodeLabel->setText("Error");
        m_statusCodeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #F85149;");
        m_responseBodyEdit->setPlainText("网络错误: " + reply->errorString());
        Logger::instance().error("HTTP", "请求失败: " + reply->errorString());
    }
}

// ─── 清除响应 ─────────────────────────────────────────────────────────

void HttpWidget::clearResponse()
{
    m_statusCodeLabel->setText("-");
    m_statusCodeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    m_responseTimeLabel->setText("-");
    m_responseSizeLabel->setText("-");
    m_responseHeadersTable->setRowCount(0);
    m_responseBodyEdit->clear();
    m_exportBtn->setEnabled(false);
}

// ─── 状态码颜色 ───────────────────────────────────────────────────────

QString HttpWidget::statusColor(int code) const
{
    if (code >= 200 && code < 300) return "#3FB950";   // green: 2xx
    if (code >= 300 && code < 400) return "#D29922";   // yellow: 3xx
    if (code >= 400 && code < 500) return "#F85149";   // red: 4xx
    if (code >= 500)              return "#F85149";   // red: 5xx
    return "#E6EDF3";
}

// ─── JSON 格式化 ──────────────────────────────────────────────────────

void HttpWidget::formatJson(const QByteArray& data, QPlainTextEdit* editor)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QString formatted;
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            QJsonDocument formattedDoc(array);
            formatted = QString::fromUtf8(formattedDoc.toJson(QJsonDocument::Indented));
        } else {
            QJsonObject obj = doc.object();
            QJsonDocument formattedDoc(obj);
            formatted = QString::fromUtf8(formattedDoc.toJson(QJsonDocument::Indented));
        }
        editor->setPlainText(formatted);
    } else {
        // Not valid JSON, display as plain text
        editor->setPlainText(QString::fromUtf8(data));
    }
}

// ─── 格式化文件大小 ───────────────────────────────────────────────────

static QString formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString::number(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    } else if (bytes < 1024LL * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else {
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
}

// ─── 请求历史 ─────────────────────────────────────────────────────────

void HttpWidget::addToHistory(const QString& url, const QString& method)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString entry = QString("[%1] %2 %3").arg(timeStr, method, url);

    // 避免重复插入相同条目
    if (m_historyList->count() > 0) {
        QListWidgetItem* topItem = m_historyList->item(0);
        if (topItem && topItem->text() == entry) {
            return;
        }
    }

    m_historyList->insertItem(0, entry);

    // 限制历史数量
    while (m_historyList->count() > 100) {
        delete m_historyList->takeItem(m_historyList->count() - 1);
    }
}

void HttpWidget::onHistoryItemClicked(QListWidgetItem* item)
{
    if (!item) return;

    QString text = item->text();
    // 格式: "[HH:mm:ss] METHOD URL"
    // 去掉时间戳前缀
    int closingBracket = text.indexOf("] ");
    if (closingBracket < 0) return;

    QString rest = text.mid(closingBracket + 2);
    int firstSpace = rest.indexOf(' ');
    if (firstSpace < 0) return;

    QString method = rest.left(firstSpace);
    QString url = rest.mid(firstSpace + 1);

    m_urlEdit->setText(url);

    int idx = m_methodCombo->findText(method);
    if (idx >= 0) {
        m_methodCombo->setCurrentIndex(idx);
    }
}

void HttpWidget::onClearHistory()
{
    m_historyList->clear();
    Logger::instance().info("HTTP", "请求历史已清除");
}

// ─── 导出响应 ─────────────────────────────────────────────────────────

void HttpWidget::onExportResponse()
{
    QString content = m_responseBodyEdit->toPlainText();
    if (content.isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的响应内容。");
        return;
    }

    // 根据 Content-Type 推断扩展名
    QString ext = "txt";
    for (int row = 0; row < m_responseHeadersTable->rowCount(); ++row) {
        auto* nameItem = m_responseHeadersTable->item(row, 0);
        if (nameItem && nameItem->text().toLower() == "content-type") {
            auto* valueItem = m_responseHeadersTable->item(row, 1);
            if (valueItem) {
                QString ct = valueItem->text().toLower();
                if (ct.contains("json")) ext = "json";
                else if (ct.contains("xml")) ext = "xml";
                else if (ct.contains("html")) ext = "html";
                else if (ct.contains("css")) ext = "css";
                else if (ct.contains("javascript")) ext = "js";
                else if (ct.contains("pdf")) ext = "pdf";
                break;
            }
        }
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出响应",
        QString("response_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"), ext),
        QString("文件 (*.%1);;所有文件 (*)").arg(ext)
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    Logger::instance().info("HTTP", QString("响应已导出到: %1").arg(filePath));
    QMessageBox::information(this, "导出成功",
                             QString("响应已成功导出到:\n%1").arg(filePath));
}