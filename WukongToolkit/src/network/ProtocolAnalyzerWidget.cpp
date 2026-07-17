#include "network/ProtocolAnalyzerWidget.h"
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
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslSocket>
#include <QSslConfiguration>

// ============================================================================
// 通用样式常量
// ============================================================================
static const char* kLineEditStyle =
    "QLineEdit {"
    "  background: #25262B; color: #DCDCDC;"
    "  border: 1px solid #3C3F41; padding: 5px 8px;"
    "  border-radius: 3px; font-size: 13px;"
    "}";

static const char* kPrimaryBtnStyle =
    "QPushButton {"
    "  background-color: #409EFF; color: white;"
    "  border: none; padding: 8px 20px; border-radius: 4px;"
    "  font-size: 13px; font-weight: bold;"
    "}"
    "QPushButton:hover { background-color: #66B1FF; }"
    "QPushButton:disabled { background-color: #5C5C5C; }";

static const char* kExportBtnStyle =
    "QPushButton {"
    "  background-color: #409EFF; color: white;"
    "  border: none; padding: 5px 14px; border-radius: 3px;"
    "  font-size: 12px;"
    "}"
    "QPushButton:hover { background-color: #66B1FF; }"
    "QPushButton:disabled { background-color: #5C5C5C; }";

static const char* kTableStyle =
    "QTableWidget {"
    "  background-color: #1E1F22; color: #DCDCDC;"
    "  gridline-color: #3C3F41; border: 1px solid #3C3F41;"
    "  font-size: 12px;"
    "}"
    "QTableWidget::item { padding: 3px 6px; }"
    "QHeaderView::section {"
    "  background-color: #25262B; color: #8C8C8C;"
    "  border: none; border-bottom: 2px solid #3C3F41;"
    "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
    "}"
    "QTableWidget::item:alternate { background-color: #25262B; }";

static const char* kPlainTextStyle =
    "QPlainTextEdit {"
    "  background-color: #1E1F22; color: #DCDCDC;"
    "  border: 1px solid #3C3F41;"
    "  font-size: 12px; font-family: 'Menlo', 'Consolas', monospace;"
    "}";

static const char* kResultLabelStyle =
    "QLabel {"
    "  background-color: #25262B; color: #DCDCDC;"
    "  border: 1px solid #3C3F41; border-radius: 3px;"
    "  padding: 6px 10px; font-size: 13px; font-family: 'Menlo', 'Consolas', monospace;"
    "  min-height: 20px;"
    "}";

static const char* kDescLabelStyle =
    "font-size: 12px; color: #8C8C8C;";

// ============================================================================
// 辅助函数
// ============================================================================
static QLabel* makeDescLabel(const QString& text)
{
    auto* lbl = new QLabel(text);
    lbl->setStyleSheet(kDescLabelStyle);
    return lbl;
}

static QLabel* makeResultLabel()
{
    auto* lbl = new QLabel("-");
    lbl->setStyleSheet(kResultLabelStyle);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl->setWordWrap(true);
    return lbl;
}

static QLineEdit* makeLineEdit(const QString& placeholder)
{
    auto* edit = new QLineEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(kLineEditStyle);
    return edit;
}

static QPushButton* makePrimaryBtn(const QString& text)
{
    auto* btn = new QPushButton(text);
    btn->setStyleSheet(kPrimaryBtnStyle);
    btn->setFixedHeight(32);
    return btn;
}

static QPushButton* makeExportBtn(const QString& text = "导出结果")
{
    auto* btn = new QPushButton(text);
    btn->setStyleSheet(kExportBtnStyle);
    return btn;
}

// ============================================================================
// ProtocolAnalyzerWidget 实现
// ============================================================================

ProtocolAnalyzerWidget::ProtocolAnalyzerWidget(QWidget* parent)
    : QWidget(parent)
    , m_httpNam(nullptr)
    , m_sslProcess(nullptr)
    , m_ntpProcess(nullptr)
    , m_whoisProcess(nullptr)
{
    setupUI();
    setupConnections();
}

ProtocolAnalyzerWidget::~ProtocolAnalyzerWidget()
{
    // 清理进程
    QProcess* procs[] = { m_sslProcess, m_ntpProcess, m_whoisProcess };
    for (QProcess* p : procs) {
        if (p) {
            p->kill();
            p->waitForFinished(3000);
            delete p;
        }
    }
}

// ─── UI Setup ──────────────────────────────────────────────────────────

void ProtocolAnalyzerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(0);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #3C3F41;"
        "  background: #1E1F22;"
        "}"
        "QTabBar::tab {"
        "  background: #25262B; color: #8C8C8C;"
        "  padding: 8px 20px; border: 1px solid #3C3F41;"
        "  border-bottom: none; font-size: 13px;"
        "  border-top-left-radius: 4px; border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1E1F22; color: #409EFF;"
        "  border-bottom: 2px solid #409EFF;"
        "}"
        "QTabBar::tab:hover {"
        "  background: #2B2D30; color: #DCDCDC;"
        "}"
    );

    auto* httpTab = new QWidget();
    auto* sslTab = new QWidget();
    auto* ntpTab = new QWidget();
    auto* whoisTab = new QWidget();
    auto* subnetTab = new QWidget();

    setupHttpTab(httpTab);
    setupSslTab(sslTab);
    setupNtpTab(ntpTab);
    setupWhoisTab(whoisTab);
    setupSubnetTab(subnetTab);

    m_tabWidget->addTab(httpTab, "HTTP Headers");
    m_tabWidget->addTab(sslTab, "SSL/TLS");
    m_tabWidget->addTab(ntpTab, "NTP");
    m_tabWidget->addTab(whoisTab, "WHOIS");
    m_tabWidget->addTab(subnetTab, "子网计算");

    mainLayout->addWidget(m_tabWidget);
}

void ProtocolAnalyzerWidget::setupConnections()
{
    // HTTP
    connect(m_httpSendBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onHttpSend);
    connect(m_httpUrlEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onHttpSend);
    connect(m_httpExportBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onHttpExport);

    // SSL
    connect(m_sslCheckBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onSslCheck);
    connect(m_sslHostEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onSslCheck);
    connect(m_sslExportBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onSslExport);

    // NTP
    connect(m_ntpQueryBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onNtpQuery);
    connect(m_ntpServerEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onNtpQuery);
    connect(m_ntpExportBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onNtpExport);

    // WHOIS
    connect(m_whoisQueryBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onWhoisQuery);
    connect(m_whoisInputEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onWhoisQuery);
    connect(m_whoisExportBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onWhoisExport);

    // Subnet
    connect(m_subnetCalcBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onSubnetCalc);
    connect(m_subnetIpEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onSubnetCalc);
    connect(m_subnetMaskEdit, &QLineEdit::returnPressed, this, &ProtocolAnalyzerWidget::onSubnetCalc);
    connect(m_subnetExportBtn, &QPushButton::clicked, this, &ProtocolAnalyzerWidget::onSubnetExport);
}

// ============================================================================
// Tab 1: HTTP Headers
// ============================================================================

void ProtocolAnalyzerWidget::setupHttpTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ── 输入行 ──
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    auto* urlLabel = new QLabel("URL:");
    urlLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(urlLabel);

    m_httpUrlEdit = makeLineEdit("https://www.example.com");
    inputRow->addWidget(m_httpUrlEdit, 1);

    m_httpSendBtn = makePrimaryBtn("发送请求");
    inputRow->addWidget(m_httpSendBtn);

    layout->addLayout(inputRow);

    // ── 状态码 ──
    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(8);
    auto* statusLabel = new QLabel("状态码:");
    statusLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    statusRow->addWidget(statusLabel);

    m_httpStatusEdit = new QLineEdit();
    m_httpStatusEdit->setReadOnly(true);
    m_httpStatusEdit->setPlaceholderText("等待请求...");
    m_httpStatusEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #25262B; color: #409EFF;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 14px; font-weight: bold;"
        "  max-width: 200px;"
        "}"
    );
    statusRow->addWidget(m_httpStatusEdit);
    statusRow->addStretch();

    m_httpExportBtn = makeExportBtn();
    m_httpExportBtn->setEnabled(false);
    statusRow->addWidget(m_httpExportBtn);

    layout->addLayout(statusRow);

    // ── 响应头表格 + 原始响应 (分栏) ──
    auto* splitter = new QSplitter(Qt::Vertical);

    // 响应头表
    m_httpHeadersTable = new QTableWidget(0, 2);
    m_httpHeadersTable->setHorizontalHeaderLabels({"Header", "Value"});
    m_httpHeadersTable->horizontalHeader()->setStretchLastSection(true);
    m_httpHeadersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_httpHeadersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_httpHeadersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_httpHeadersTable->setAlternatingRowColors(true);
    m_httpHeadersTable->verticalHeader()->setVisible(false);
    m_httpHeadersTable->setStyleSheet(kTableStyle);
    splitter->addWidget(m_httpHeadersTable);

    // 原始响应
    auto* rawWidget = new QWidget();
    auto* rawLayout = new QVBoxLayout(rawWidget);
    rawLayout->setContentsMargins(0, 0, 0, 0);
    rawLayout->setSpacing(2);

    auto* rawLabel = new QLabel("原始响应体:");
    rawLabel->setStyleSheet("font-size: 12px; color: #8C8C8C;");
    rawLayout->addWidget(rawLabel);

    m_httpRawText = new QTextEdit();
    m_httpRawText->setReadOnly(true);
    m_httpRawText->setPlaceholderText("响应体将显示在此处...");
    m_httpRawText->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1E1F22; color: #DCDCDC;"
        "  border: 1px solid #3C3F41;"
        "  font-size: 12px; font-family: 'Menlo', 'Consolas', monospace;"
        "}"
    );
    rawLayout->addWidget(m_httpRawText);

    splitter->addWidget(rawWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter, 1);
}

void ProtocolAnalyzerWidget::onHttpSend()
{
    QString urlStr = m_httpUrlEdit->text().trimmed();
    if (urlStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 URL。");
        return;
    }

    // 自动补全协议
    if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
        urlStr = "https://" + urlStr;
        m_httpUrlEdit->setText(urlStr);
    }

    QUrl url(urlStr);
    if (!url.isValid()) {
        QMessageBox::warning(this, "输入错误", "无效的 URL: " + urlStr);
        return;
    }

    if (!m_httpNam) {
        m_httpNam = new QNetworkAccessManager(this);
        connect(m_httpNam, &QNetworkAccessManager::finished,
                this, &ProtocolAnalyzerWidget::onHttpReplyFinished);
    }

    // 清空之前的结果
    m_httpHeadersTable->setRowCount(0);
    m_httpRawText->clear();
    m_httpStatusEdit->clear();
    m_httpStatusEdit->setPlaceholderText("请求中...");
    m_httpSendBtn->setEnabled(false);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "WukongToolkit/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_httpNam->get(request);

    Logger::instance().info("HTTP", "发送 GET 请求: " + urlStr);
}

void ProtocolAnalyzerWidget::onHttpReplyFinished(QNetworkReply* reply)
{
    m_httpSendBtn->setEnabled(true);
    reply->deleteLater();

    // 状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    if (statusCode > 0) {
        m_httpStatusEdit->setText(QString("%1 %2").arg(statusCode).arg(reasonPhrase));
        // 根据状态码着色
        if (statusCode >= 200 && statusCode < 300) {
            m_httpStatusEdit->setStyleSheet(
                "QLineEdit { background: #25262B; color: #67C23A;"
                "  border: 1px solid #3C3F41; padding: 5px 8px;"
                "  border-radius: 3px; font-size: 14px; font-weight: bold; max-width: 200px; }");
        } else if (statusCode >= 300 && statusCode < 400) {
            m_httpStatusEdit->setStyleSheet(
                "QLineEdit { background: #25262B; color: #E6A23C;"
                "  border: 1px solid #3C3F41; padding: 5px 8px;"
                "  border-radius: 3px; font-size: 14px; font-weight: bold; max-width: 200px; }");
        } else {
            m_httpStatusEdit->setStyleSheet(
                "QLineEdit { background: #25262B; color: #F56C6C;"
                "  border: 1px solid #3C3F41; padding: 5px 8px;"
                "  border-radius: 3px; font-size: 14px; font-weight: bold; max-width: 200px; }");
        }
    } else if (reply->error() != QNetworkReply::NoError) {
        m_httpStatusEdit->setText("错误: " + reply->errorString());
        m_httpStatusEdit->setStyleSheet(
            "QLineEdit { background: #25262B; color: #F56C6C;"
            "  border: 1px solid #3C3F41; padding: 5px 8px;"
            "  border-radius: 3px; font-size: 14px; font-weight: bold; max-width: 400px; }");
        m_httpExportBtn->setEnabled(false);
        Logger::instance().error("HTTP", "请求失败: " + reply->errorString());
        m_httpRawText->setPlainText(reply->errorString());
        return;
    }

    // 响应头
    const QList<QNetworkReply::RawHeaderPair>& headers = reply->rawHeaderPairs();
    m_httpHeadersTable->setRowCount(headers.size());
    for (int i = 0; i < headers.size(); ++i) {
        auto* nameItem = new QTableWidgetItem(QString::fromUtf8(headers[i].first));
        nameItem->setForeground(QColor(0x40, 0x9E, 0xFF));
        m_httpHeadersTable->setItem(i, 0, nameItem);

        auto* valueItem = new QTableWidgetItem(QString::fromUtf8(headers[i].second));
        m_httpHeadersTable->setItem(i, 1, valueItem);
    }

    // 响应体
    QByteArray body = reply->readAll();
    // 尝试检测是否为 JSON 并美化
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &jsonError);
    if (jsonError.error == QJsonParseError::NoError && !doc.isNull()) {
        m_httpRawText->setPlainText(doc.toJson(QJsonDocument::Indented));
    } else {
        m_httpRawText->setPlainText(QString::fromUtf8(body));
    }

    m_httpExportBtn->setEnabled(true);

    Logger::instance().info("HTTP",
                            QString("响应: %1 %2, %3 个响应头, %4 字节")
                                .arg(statusCode).arg(reasonPhrase)
                                .arg(headers.size()).arg(body.size()));
}

void ProtocolAnalyzerWidget::onHttpExport()
{
    if (m_httpHeadersTable->rowCount() == 0 && m_httpRawText->toPlainText().isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 HTTP 结果",
        QString("http_result_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "文本文件 (*.txt);;JSON 文件 (*.json)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "HTTP Response Export\n";
    out << "====================\n";
    out << "URL: " << m_httpUrlEdit->text() << "\n";
    out << "Status: " << m_httpStatusEdit->text() << "\n\n";
    out << "Response Headers:\n";
    out << "-----------------\n";
    for (int row = 0; row < m_httpHeadersTable->rowCount(); ++row) {
        auto* nameItem = m_httpHeadersTable->item(row, 0);
        auto* valueItem = m_httpHeadersTable->item(row, 1);
        out << (nameItem ? nameItem->text() : "-") << ": "
            << (valueItem ? valueItem->text() : "-") << "\n";
    }
    out << "\nResponse Body:\n";
    out << "--------------\n";
    out << m_httpRawText->toPlainText() << "\n";

    file.close();
    Logger::instance().info("HTTP", "结果已导出到: " + filePath);
    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}

// ============================================================================
// Tab 2: SSL/TLS 证书信息
// ============================================================================

void ProtocolAnalyzerWidget::setupSslTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ── 输入行 ──
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    auto* hostLabel = new QLabel("主机:");
    hostLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(hostLabel);

    m_sslHostEdit = makeLineEdit("example.com");
    inputRow->addWidget(m_sslHostEdit, 1);

    auto* portLabel = new QLabel("端口:");
    portLabel->setStyleSheet("font-size: 13px; color: #DCDCDC;");
    inputRow->addWidget(portLabel);

    m_sslPortSpin = new QSpinBox();
    m_sslPortSpin->setRange(1, 65535);
    m_sslPortSpin->setValue(443);
    m_sslPortSpin->setStyleSheet(
        "QSpinBox {"
        "  background: #25262B; color: #DCDCDC;"
        "  border: 1px solid #3C3F41; padding: 5px 8px;"
        "  border-radius: 3px; font-size: 13px;"
        "}"
    );
    inputRow->addWidget(m_sslPortSpin);

    m_sslCheckBtn = makePrimaryBtn("检查证书");
    inputRow->addWidget(m_sslCheckBtn);

    layout->addLayout(inputRow);

    // ── 导出按钮 ──
    auto* exportRow = new QHBoxLayout();
    exportRow->addStretch();
    m_sslExportBtn = makeExportBtn();
    m_sslExportBtn->setEnabled(false);
    exportRow->addWidget(m_sslExportBtn);
    layout->addLayout(exportRow);

    // ── 证书信息 Form ──
    auto* infoGroup = new QGroupBox("证书信息");
    infoGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px;"
        "  margin-top: 10px; padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 12px; padding: 0 6px;"
        "}"
    );
    auto* infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(8);
    infoForm->setContentsMargins(12, 16, 12, 12);

    m_sslSubjectLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("Subject:"), m_sslSubjectLabel);

    m_sslIssuerLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("Issuer:"), m_sslIssuerLabel);

    m_sslValidFromLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("有效期从:"), m_sslValidFromLabel);

    m_sslValidToLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("有效期至:"), m_sslValidToLabel);

    m_sslSANLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("SAN:"), m_sslSANLabel);

    m_sslSerialLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("序列号:"), m_sslSerialLabel);

    m_sslFingerprintLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("指纹:"), m_sslFingerprintLabel);

    layout->addWidget(infoGroup);
    layout->addStretch();
}

void ProtocolAnalyzerWidget::onSslCheck()
{
    QString host = m_sslHostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入主机名。");
        return;
    }

    int port = m_sslPortSpin->value();

    // 清空之前的结果
    m_sslSubjectLabel->setText("-");
    m_sslIssuerLabel->setText("-");
    m_sslValidFromLabel->setText("-");
    m_sslValidToLabel->setText("-");
    m_sslSANLabel->setText("-");
    m_sslSerialLabel->setText("-");
    m_sslFingerprintLabel->setText("-");

    m_sslCheckBtn->setEnabled(false);
    m_sslCheckBtn->setText("检查中...");
    m_sslExportBtn->setEnabled(false);

    // 使用 QProcess 调用 openssl
    if (m_sslProcess) {
        m_sslProcess->kill();
        m_sslProcess->waitForFinished(2000);
        delete m_sslProcess;
        m_sslProcess = nullptr;
    }

    m_sslProcess = new QProcess(this);
    connect(m_sslProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProtocolAnalyzerWidget::onSslProcessFinished);

    // 使用 openssl s_client 获取证书，然后通过管道传给 x509 解析
    // echo | openssl s_client -connect host:port -servername host 2>&1
    QStringList args;
    args << "s_client" << "-connect" << QString("%1:%2").arg(host).arg(port)
         << "-servername" << host << "-showcerts";

    m_sslProcess->start("openssl", args);

    // 写入空行以关闭连接
    if (m_sslProcess->waitForStarted(3000)) {
        m_sslProcess->write("\n");
        m_sslProcess->closeWriteChannel();
    }

    Logger::instance().info("SSL", QString("检查 SSL 证书: %1:%2").arg(host).arg(port));
}

void ProtocolAnalyzerWidget::onSslProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_sslCheckBtn->setEnabled(true);
    m_sslCheckBtn->setText("检查证书");

    if (!m_sslProcess) return;

    QString output = QString::fromUtf8(m_sslProcess->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_sslProcess->readAllStandardError());

    m_sslProcess->deleteLater();
    m_sslProcess = nullptr;

    if (output.isEmpty() && errOutput.isEmpty()) {
        m_sslSubjectLabel->setText("无法连接到服务器");
        Logger::instance().error("SSL", "openssl s_client 未返回数据");
        return;
    }

    // 合并 stdout 和 stderr（openssl 把证书信息输出到两者）
    QString combined = output + "\n" + errOutput;

    // 提取证书部分（BEGIN CERTIFICATE ... END CERTIFICATE）
    static QRegularExpression certRe(
        R"(-----BEGIN CERTIFICATE-----\s*([\s\S]*?)\s*-----END CERTIFICATE-----)");
    QRegularExpressionMatch certMatch = certRe.match(combined);

    if (!certMatch.hasMatch()) {
        m_sslSubjectLabel->setText("未能获取证书");
        // 显示原始错误信息
        QString err = errOutput.trimmed();
        if (!err.isEmpty()) {
            m_sslSubjectLabel->setText("错误: " + err.left(200));
        }
        Logger::instance().error("SSL", "未能从输出中提取证书");
        return;
    }

    // 重建 PEM 格式证书
    QString pem = "-----BEGIN CERTIFICATE-----\n" + certMatch.captured(1).trimmed() + "\n-----END CERTIFICATE-----";

    // 使用 second openssl 进程解析证书
    auto* x509Proc = new QProcess(this);
    connect(x509Proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pem](int x509ExitCode, QProcess::ExitStatus x509ExitStatus) {
        Q_UNUSED(x509ExitCode)
        Q_UNUSED(x509ExitStatus)

        auto* proc = qobject_cast<QProcess*>(sender());
        if (!proc) return;

        QString text = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        m_sslExportBtn->setEnabled(true);

        // 解析 x509 输出
        static QRegularExpression subjRe(R"(subject\s*=\s*(.+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression issuerRe(R"(issuer\s*=\s*(.+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression notBeforeRe(R"(notBefore\s*=\s*(.+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression notAfterRe(R"(notAfter\s*=\s*(.+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression sanRe(R"(DNS:([^\s,]+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression serialRe(R"(serial\s*=\s*([0-9A-Fa-f:]+))", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression fingerprintRe(R"(SHA256 Fingerprint\s*=\s*([0-9A-Fa-f:]+))", QRegularExpression::CaseInsensitiveOption);

        auto matchOrDash = [&](const QRegularExpression& re, const QString& text) -> QString {
            QRegularExpressionMatch m = re.match(text);
            return m.hasMatch() ? m.captured(1).trimmed() : "-";
        };

        m_sslSubjectLabel->setText(matchOrDash(subjRe, text));
        m_sslIssuerLabel->setText(matchOrDash(issuerRe, text));
        m_sslValidFromLabel->setText(matchOrDash(notBeforeRe, text));
        m_sslValidToLabel->setText(matchOrDash(notAfterRe, text));
        m_sslSerialLabel->setText(matchOrDash(serialRe, text));
        m_sslFingerprintLabel->setText(matchOrDash(fingerprintRe, text));

        // 提取所有 SAN DNS 条目
        QRegularExpressionMatchIterator sanIt = sanRe.globalMatch(text);
        QStringList sanList;
        while (sanIt.hasNext()) {
            sanList << sanIt.next().captured(1);
        }
        m_sslSANLabel->setText(sanList.isEmpty() ? "-" : sanList.join(", "));

        // 检查证书有效期
        QString notAfter = m_sslValidToLabel->text();
        if (notAfter != "-") {
            QDateTime expiry = QDateTime::fromString(notAfter, "MMM dd HH:mm:ss yyyy GMT");
            if (!expiry.isValid()) {
                expiry = QDateTime::fromString(notAfter, Qt::ISODate);
            }
            if (expiry.isValid()) {
                qint64 daysLeft = QDateTime::currentDateTimeUtc().daysTo(expiry);
                if (daysLeft < 0) {
                    m_sslValidToLabel->setStyleSheet(kResultLabelStyle + QString(" color: #F56C6C;"));
                    m_sslValidToLabel->setText(notAfter + QString("  [已过期 %1 天]").arg(-daysLeft));
                } else if (daysLeft < 30) {
                    m_sslValidToLabel->setStyleSheet(kResultLabelStyle + QString(" color: #E6A23C;"));
                    m_sslValidToLabel->setText(notAfter + QString("  [剩余 %1 天]").arg(daysLeft));
                } else {
                    m_sslValidToLabel->setStyleSheet(kResultLabelStyle + QString(" color: #67C23A;"));
                    m_sslValidToLabel->setText(notAfter + QString("  [剩余 %1 天]").arg(daysLeft));
                }
            }
        }

        Logger::instance().info("SSL", "证书解析完成");
    });

    // 写入 PEM 到 stdin
    x509Proc->start("openssl", QStringList() << "x509" << "-noout"
                                               << "-subject" << "-issuer"
                                               << "-dates" << "-ext" << "subjectAltName"
                                               << "-serial" << "-fingerprint"
                                               << "-sha256");
    if (x509Proc->waitForStarted(3000)) {
        x509Proc->write(pem.toUtf8());
        x509Proc->closeWriteChannel();
    }
}

void ProtocolAnalyzerWidget::onSslExport()
{
    QString host = m_sslHostEdit->text().trimmed();
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 SSL 证书信息",
        QString("ssl_cert_%1_%2.txt").arg(host).arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "文本文件 (*.txt)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "SSL Certificate Information\n";
    out << "===========================\n";
    out << "Host: " << host << ":" << m_sslPortSpin->value() << "\n\n";
    out << "Subject:     " << m_sslSubjectLabel->text() << "\n";
    out << "Issuer:      " << m_sslIssuerLabel->text() << "\n";
    out << "Valid From:  " << m_sslValidFromLabel->text() << "\n";
    out << "Valid To:    " << m_sslValidToLabel->text() << "\n";
    out << "SAN:         " << m_sslSANLabel->text() << "\n";
    out << "Serial:      " << m_sslSerialLabel->text() << "\n";
    out << "Fingerprint: " << m_sslFingerprintLabel->text() << "\n";

    file.close();
    Logger::instance().info("SSL", "证书信息已导出到: " + filePath);
    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}

// ============================================================================
// Tab 3: NTP 查询
// ============================================================================

void ProtocolAnalyzerWidget::setupNtpTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ── 输入行 ──
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    auto* serverLabel = new QLabel("NTP 服务器:");
    serverLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(serverLabel);

    m_ntpServerEdit = makeLineEdit("pool.ntp.org");
    inputRow->addWidget(m_ntpServerEdit, 1);

    m_ntpQueryBtn = makePrimaryBtn("查询");
    inputRow->addWidget(m_ntpQueryBtn);

    layout->addLayout(inputRow);

    // ── 导出按钮 ──
    auto* exportRow = new QHBoxLayout();
    exportRow->addStretch();
    m_ntpExportBtn = makeExportBtn();
    m_ntpExportBtn->setEnabled(false);
    exportRow->addWidget(m_ntpExportBtn);
    layout->addLayout(exportRow);

    // ── 结果 Form ──
    auto* infoGroup = new QGroupBox("NTP 查询结果");
    infoGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px;"
        "  margin-top: 10px; padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 12px; padding: 0 6px;"
        "}"
    );
    auto* infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(8);
    infoForm->setContentsMargins(12, 16, 12, 12);

    m_ntpTimeLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("服务器时间:"), m_ntpTimeLabel);

    m_ntpStratumLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("Stratum:"), m_ntpStratumLabel);

    m_ntpDelayLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("延迟 (Delay):"), m_ntpDelayLabel);

    m_ntpOffsetLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("偏移 (Offset):"), m_ntpOffsetLabel);

    layout->addWidget(infoGroup);
    layout->addStretch();
}

void ProtocolAnalyzerWidget::onNtpQuery()
{
    QString server = m_ntpServerEdit->text().trimmed();
    if (server.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 NTP 服务器地址。");
        return;
    }

    m_ntpTimeLabel->setText("查询中...");
    m_ntpStratumLabel->setText("-");
    m_ntpDelayLabel->setText("-");
    m_ntpOffsetLabel->setText("-");

    m_ntpQueryBtn->setEnabled(false);
    m_ntpQueryBtn->setText("查询中...");
    m_ntpExportBtn->setEnabled(false);

    if (m_ntpProcess) {
        m_ntpProcess->kill();
        m_ntpProcess->waitForFinished(2000);
        delete m_ntpProcess;
        m_ntpProcess = nullptr;
    }

    m_ntpProcess = new QProcess(this);
    connect(m_ntpProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProtocolAnalyzerWidget::onNtpProcessFinished);

    // 尝试使用 sntp，也尝试 ntpdate -q
    // sntp -K /dev/null server  (macOS sntp)
    m_ntpProcess->start("sntp", QStringList() << "-K" << "/dev/null" << server);

    Logger::instance().info("NTP", "查询 NTP 服务器: " + server);
}

void ProtocolAnalyzerWidget::onNtpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_ntpQueryBtn->setEnabled(true);
    m_ntpQueryBtn->setText("查询");

    if (!m_ntpProcess) return;

    QString output = QString::fromUtf8(m_ntpProcess->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_ntpProcess->readAllStandardError());

    m_ntpProcess->deleteLater();
    m_ntpProcess = nullptr;

    QString combined = output + "\n" + errOutput;

    if (exitCode != 0 && output.trimmed().isEmpty()) {
        // sntp 失败，尝试 ntpdate -q
        auto* ntpdateProc = new QProcess(this);
        connect(ntpdateProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int ec, QProcess::ExitStatus es) {
            Q_UNUSED(es)
            auto* proc = qobject_cast<QProcess*>(sender());
            if (!proc) return;

            QString out = QString::fromUtf8(proc->readAllStandardOutput());
            QString err = QString::fromUtf8(proc->readAllStandardError());
            proc->deleteLater();

            QString combined2 = out + "\n" + err;
            if (ec != 0) {
                m_ntpTimeLabel->setText("查询失败");
                m_ntpStratumLabel->setText(err.trimmed().isEmpty() ? "无法连接" : err.trimmed().left(200));
                Logger::instance().error("NTP", "ntpdate 查询失败: " + err.trimmed());
                return;
            }
            // 解析 ntpdate 输出
            parseNtpOutput(combined2);
        });

        QString server = m_ntpServerEdit->text().trimmed();
        ntpdateProc->start("ntpdate", QStringList() << "-q" << server);
        return;
    }

    parseNtpOutput(combined);
}

void ProtocolAnalyzerWidget::parseNtpOutput(const QString& output)
{
    // sntp 输出格式: "2024-01-15 10:30:45.123456 (+0800) +0.00123 +/- 0.02345 pool.ntp.org 1.2.3.4 s1"
    // ntpdate 输出格式: "server 1.2.3.4, stratum 1, offset 0.001234, delay 0.023456"
    static QRegularExpression sntpRe(
        R"(^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+)\s+\([^)]+\)\s+([+-]\d+\.\d+)\s+\+\/-\s+(\d+\.\d+)\s+\S+\s+\S+\s+s(\d+))");
    static QRegularExpression ntpdateRe(
        R"(stratum\s+(\d+).*offset\s+([+-]?\d+\.\d+).*delay\s+(\d+\.\d+))");
    static QRegularExpression ntpdateTimeRe(
        R"(adjust time server \S+ offset ([+-]?\d+\.\d+) sec)");

    QRegularExpressionMatch sntpMatch = sntpRe.match(output);
    if (sntpMatch.hasMatch()) {
        m_ntpTimeLabel->setText(sntpMatch.captured(1));
        m_ntpStratumLabel->setText(sntpMatch.captured(4));
        m_ntpOffsetLabel->setText(sntpMatch.captured(2) + " 秒");
        m_ntpDelayLabel->setText(sntpMatch.captured(3) + " 秒");
        m_ntpExportBtn->setEnabled(true);
        Logger::instance().info("NTP", "sntp 查询成功");
        return;
    }

    QRegularExpressionMatch ntpdateMatch = ntpdateRe.match(output);
    if (ntpdateMatch.hasMatch()) {
        m_ntpStratumLabel->setText(ntpdateMatch.captured(1));
        m_ntpOffsetLabel->setText(ntpdateMatch.captured(2) + " 秒");
        m_ntpDelayLabel->setText(ntpdateMatch.captured(3) + " 秒");
        m_ntpTimeLabel->setText("见延迟/偏移");
        m_ntpExportBtn->setEnabled(true);
        Logger::instance().info("NTP", "ntpdate 查询成功");
        return;
    }

    // 显示原始输出
    m_ntpTimeLabel->setText("无法解析响应");
    m_ntpStratumLabel->setText(output.trimmed().left(500));
    Logger::instance().error("NTP", "无法解析 NTP 响应: " + output.trimmed().left(200));
}

void ProtocolAnalyzerWidget::onNtpExport()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 NTP 结果",
        QString("ntp_result_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "文本文件 (*.txt)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "NTP Query Result\n";
    out << "================\n";
    out << "Server: " << m_ntpServerEdit->text() << "\n\n";
    out << "Time:    " << m_ntpTimeLabel->text() << "\n";
    out << "Stratum: " << m_ntpStratumLabel->text() << "\n";
    out << "Delay:   " << m_ntpDelayLabel->text() << "\n";
    out << "Offset:  " << m_ntpOffsetLabel->text() << "\n";

    file.close();
    Logger::instance().info("NTP", "NTP 结果已导出到: " + filePath);
    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}

// ============================================================================
// Tab 4: WHOIS 查询
// ============================================================================

void ProtocolAnalyzerWidget::setupWhoisTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ── 输入行 ──
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    auto* inputLabel = new QLabel("域名/IP:");
    inputLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(inputLabel);

    m_whoisInputEdit = makeLineEdit("example.com");
    inputRow->addWidget(m_whoisInputEdit, 1);

    m_whoisQueryBtn = makePrimaryBtn("查询");
    inputRow->addWidget(m_whoisQueryBtn);

    layout->addLayout(inputRow);

    // ── 导出按钮 ──
    auto* exportRow = new QHBoxLayout();
    exportRow->addStretch();
    m_whoisExportBtn = makeExportBtn();
    m_whoisExportBtn->setEnabled(false);
    exportRow->addWidget(m_whoisExportBtn);
    layout->addLayout(exportRow);

    // ── 结果区 ──
    m_whoisResultText = new QPlainTextEdit();
    m_whoisResultText->setReadOnly(true);
    m_whoisResultText->setPlaceholderText("WHOIS 查询结果将显示在此处...");
    m_whoisResultText->setStyleSheet(kPlainTextStyle);
    layout->addWidget(m_whoisResultText, 1);
}

void ProtocolAnalyzerWidget::onWhoisQuery()
{
    QString target = m_whoisInputEdit->text().trimmed();
    if (target.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入域名或 IP 地址。");
        return;
    }

    m_whoisResultText->clear();
    m_whoisResultText->setPlaceholderText("查询中...");
    m_whoisQueryBtn->setEnabled(false);
    m_whoisQueryBtn->setText("查询中...");
    m_whoisExportBtn->setEnabled(false);

    if (m_whoisProcess) {
        m_whoisProcess->kill();
        m_whoisProcess->waitForFinished(2000);
        delete m_whoisProcess;
        m_whoisProcess = nullptr;
    }

    m_whoisProcess = new QProcess(this);
    connect(m_whoisProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProtocolAnalyzerWidget::onWhoisProcessFinished);

    m_whoisProcess->start("whois", QStringList() << target);

    Logger::instance().info("WHOIS", "查询 WHOIS: " + target);
}

void ProtocolAnalyzerWidget::onWhoisProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    m_whoisQueryBtn->setEnabled(true);
    m_whoisQueryBtn->setText("查询");

    if (!m_whoisProcess) return;

    QString output = QString::fromUtf8(m_whoisProcess->readAllStandardOutput());
    QString errOutput = QString::fromUtf8(m_whoisProcess->readAllStandardError());

    m_whoisProcess->deleteLater();
    m_whoisProcess = nullptr;

    if (output.isEmpty() && !errOutput.isEmpty()) {
        m_whoisResultText->setPlainText("错误: " + errOutput.trimmed());
        Logger::instance().error("WHOIS", "查询失败: " + errOutput.trimmed());
        return;
    }

    if (output.trimmed().isEmpty()) {
        m_whoisResultText->setPlainText("未返回结果。");
        return;
    }

    m_whoisResultText->setPlainText(output);
    m_whoisExportBtn->setEnabled(true);

    Logger::instance().info("WHOIS",
                            QString("WHOIS 查询完成，%1 字符").arg(output.size()));
}

void ProtocolAnalyzerWidget::onWhoisExport()
{
    if (m_whoisResultText->toPlainText().isEmpty()) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出 WHOIS 结果",
        QString("whois_result_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "文本文件 (*.txt)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "WHOIS Query Result\n";
    out << "==================\n";
    out << "Target: " << m_whoisInputEdit->text() << "\n";
    out << "Time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    out << m_whoisResultText->toPlainText() << "\n";

    file.close();
    Logger::instance().info("WHOIS", "WHOIS 结果已导出到: " + filePath);
    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}

// ============================================================================
// Tab 5: 子网计算
// ============================================================================

void ProtocolAnalyzerWidget::setupSubnetTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // ── 输入行 ──
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    auto* ipLabel = new QLabel("IP 地址:");
    ipLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(ipLabel);

    m_subnetIpEdit = makeLineEdit("192.168.1.0");
    inputRow->addWidget(m_subnetIpEdit, 1);

    auto* maskLabel = new QLabel("掩码:");
    maskLabel->setStyleSheet("font-size: 13px; color: #DCDCDC; font-weight: bold;");
    inputRow->addWidget(maskLabel);

    m_subnetMaskEdit = makeLineEdit("24 或 255.255.255.0");
    inputRow->addWidget(m_subnetMaskEdit, 1);

    m_subnetCalcBtn = makePrimaryBtn("计算");
    inputRow->addWidget(m_subnetCalcBtn);

    layout->addLayout(inputRow);

    // ── 导出按钮 ──
    auto* exportRow = new QHBoxLayout();
    exportRow->addStretch();
    m_subnetExportBtn = makeExportBtn();
    m_subnetExportBtn->setEnabled(false);
    exportRow->addWidget(m_subnetExportBtn);
    layout->addLayout(exportRow);

    // ── 结果 Form ──
    auto* infoGroup = new QGroupBox("子网计算结果");
    infoGroup->setStyleSheet(
        "QGroupBox {"
        "  color: #409EFF; font-size: 13px; font-weight: bold;"
        "  border: 1px solid #3C3F41; border-radius: 4px;"
        "  margin-top: 10px; padding-top: 16px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 12px; padding: 0 6px;"
        "}"
    );
    auto* infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(8);
    infoForm->setContentsMargins(12, 16, 12, 12);

    m_subnetNetworkLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("网络地址:"), m_subnetNetworkLabel);

    m_subnetBroadcastLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("广播地址:"), m_subnetBroadcastLabel);

    m_subnetFirstHostLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("第一个主机:"), m_subnetFirstHostLabel);

    m_subnetLastHostLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("最后一个主机:"), m_subnetLastHostLabel);

    m_subnetTotalHostsLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("可用主机数:"), m_subnetTotalHostsLabel);

    m_subnetWildcardLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("通配符掩码:"), m_subnetWildcardLabel);

    m_subnetBinaryLabel = makeResultLabel();
    infoForm->addRow(makeDescLabel("二进制掩码:"), m_subnetBinaryLabel);

    layout->addWidget(infoGroup);
    layout->addStretch();
}

// ─── 子网计算辅助函数 ───────────────────────────────────────────────────

quint32 ProtocolAnalyzerWidget::ipToUint32(const QString& ip)
{
    QHostAddress addr(ip);
    if (addr.isNull()) return 0;
    return addr.toIPv4Address();
}

QString ProtocolAnalyzerWidget::uint32ToIp(quint32 val)
{
    return QString("%1.%2.%3.%4")
        .arg((val >> 24) & 0xFF)
        .arg((val >> 16) & 0xFF)
        .arg((val >> 8) & 0xFF)
        .arg(val & 0xFF);
}

int ProtocolAnalyzerWidget::cidrToMaskInt(const QString& cidr)
{
    bool ok = false;
    int bits = cidr.toInt(&ok);
    if (ok && bits >= 0 && bits <= 32) {
        return bits;
    }
    return -1;
}

QString ProtocolAnalyzerWidget::maskIntToDotted(int bits)
{
    if (bits < 0 || bits > 32) return "无效";
    if (bits == 0) return "0.0.0.0";
    quint32 mask = 0xFFFFFFFF << (32 - bits);
    return uint32ToIp(mask);
}

void ProtocolAnalyzerWidget::onSubnetCalc()
{
    QString ipStr = m_subnetIpEdit->text().trimmed();
    QString maskStr = m_subnetMaskEdit->text().trimmed();

    if (ipStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 IP 地址。");
        return;
    }
    if (maskStr.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入掩码（CIDR 如 24 或点分十进制如 255.255.255.0）。");
        return;
    }

    // 解析掩码
    int cidr = -1;
    quint32 maskUint = 0;

    // 尝试 CIDR 格式
    cidr = cidrToMaskInt(maskStr);
    if (cidr >= 0) {
        maskUint = (cidr == 0) ? 0 : (0xFFFFFFFF << (32 - cidr));
    } else {
        // 尝试点分十进制
        maskUint = ipToUint32(maskStr);
        if (maskUint == 0 && maskStr != "0.0.0.0") {
            QMessageBox::warning(this, "输入错误", "无效的掩码格式，请使用 CIDR（如 24）或点分十进制（如 255.255.255.0）。");
            return;
        }
        // 检查是否为合法掩码（连续的1后跟连续的0）
        quint32 inverted = ~maskUint;
        if (inverted != 0 && (inverted & (inverted + 1)) != 0) {
            QMessageBox::warning(this, "输入错误", "无效的子网掩码。子网掩码必须是连续的 1 后跟连续的 0。");
            return;
        }
        // 计算 CIDR
        cidr = 0;
        quint32 m = maskUint;
        while (m & 0x80000000) {
            cidr++;
            m <<= 1;
        }
    }

    // 解析 IP
    quint32 ipUint = ipToUint32(ipStr);
    if (ipUint == 0 && ipStr != "0.0.0.0") {
        QMessageBox::warning(this, "输入错误", "无效的 IP 地址。");
        return;
    }

    // 计算
    quint32 networkUint = ipUint & maskUint;
    quint32 broadcastUint = networkUint | (~maskUint);
    quint32 firstHost = (cidr >= 31) ? networkUint : networkUint + 1;
    quint32 lastHost = (cidr >= 31) ? broadcastUint : broadcastUint - 1;
    quint32 wildcardUint = ~maskUint;

    qint64 totalHosts = (cidr >= 31) ? 0 : (static_cast<qint64>(1) << (32 - cidr)) - 2;
    if (cidr == 32) totalHosts = 0;
    if (cidr == 31) totalHosts = 2; // RFC 3021 point-to-point

    // 显示结果
    m_subnetNetworkLabel->setText(QString("%1/%2").arg(uint32ToIp(networkUint)).arg(cidr));
    m_subnetBroadcastLabel->setText(uint32ToIp(broadcastUint));
    m_subnetFirstHostLabel->setText((cidr >= 31) ? "N/A (点对点)" : uint32ToIp(firstHost));
    m_subnetLastHostLabel->setText((cidr >= 31) ? "N/A (点对点)" : uint32ToIp(lastHost));

    if (cidr == 32) {
        m_subnetTotalHostsLabel->setText("1 (主机路由)");
    } else if (cidr == 31) {
        m_subnetTotalHostsLabel->setText("2 (RFC 3021 点对点)");
    } else {
        m_subnetTotalHostsLabel->setText(QString("%1").arg(totalHosts));
    }

    m_subnetWildcardLabel->setText(uint32ToIp(wildcardUint));

    // 二进制掩码
    QString binaryMask;
    for (int i = 31; i >= 0; --i) {
        binaryMask += (maskUint >> i) & 1 ? '1' : '0';
        if (i % 8 == 0 && i > 0) binaryMask += '.';
    }
    m_subnetBinaryLabel->setText(binaryMask);

    m_subnetExportBtn->setEnabled(true);

    Logger::instance().info("Subnet", QString("子网计算: %1/%2").arg(ipStr).arg(cidr));
}

void ProtocolAnalyzerWidget::onSubnetExport()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出子网计算结果",
        QString("subnet_calc_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "文本文件 (*.txt)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    file.write("\xEF\xBB\xBF");
    QTextStream out(&file);

    out << "Subnet Calculation Result\n";
    out << "=========================\n";
    out << "Input: " << m_subnetIpEdit->text() << " / " << m_subnetMaskEdit->text() << "\n\n";
    out << "Network:     " << m_subnetNetworkLabel->text() << "\n";
    out << "Broadcast:   " << m_subnetBroadcastLabel->text() << "\n";
    out << "First Host:  " << m_subnetFirstHostLabel->text() << "\n";
    out << "Last Host:   " << m_subnetLastHostLabel->text() << "\n";
    out << "Total Hosts: " << m_subnetTotalHostsLabel->text() << "\n";
    out << "Wildcard:    " << m_subnetWildcardLabel->text() << "\n";
    out << "Binary Mask: " << m_subnetBinaryLabel->text() << "\n";

    file.close();
    Logger::instance().info("Subnet", "子网计算结果已导出到: " + filePath);
    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}