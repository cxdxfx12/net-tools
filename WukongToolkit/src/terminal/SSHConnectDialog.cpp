#include "terminal/SSHConnectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>

SSHConnectDialog::SSHConnectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("SSH 连接");
    setMinimumWidth(420);
    setModal(true);
    setupUI();
}

void SSHConnectDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Connection group
    auto* connGroup = new QGroupBox("连接信息");
    auto* connForm = new QFormLayout(connGroup);
    connForm->setSpacing(8);

    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("192.168.1.1");
    connForm->addRow("主机地址:", m_hostEdit);

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    connForm->addRow("端口:", m_portSpin);

    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("admin");
    connForm->addRow("用户名:", m_userEdit);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("输入密码");
    connForm->addRow("密码:", m_passwordEdit);

    mainLayout->addWidget(connGroup);

    // Auth group
    auto* authGroup = new QGroupBox("密钥认证 (可选)");
    auto* authForm = new QFormLayout(authGroup);
    authForm->setSpacing(8);

    m_keyEdit = new QLineEdit();
    m_keyEdit->setPlaceholderText("选择私钥文件...");
    auto* keyBrowseBtn = new QPushButton("浏览...");
    keyBrowseBtn->setFixedWidth(70);
    auto* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(m_keyEdit);
    keyLayout->addWidget(keyBrowseBtn);
    authForm->addRow("私钥:", keyLayout);

    connect(keyBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "选择私钥文件", QString(),
                                                     "密钥文件 (*.pem *.key *.ppk);;所有文件 (*)");
        if (!path.isEmpty()) {
            m_keyEdit->setText(path);
        }
    });

    m_passphraseEdit = new QLineEdit();
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_passphraseEdit->setPlaceholderText("密钥密码 (如有)");
    authForm->addRow("密钥密码:", m_passphraseEdit);

    mainLayout->addWidget(authGroup);

    // Options group
    auto* optGroup = new QGroupBox("连接选项");
    auto* optForm = new QFormLayout(optGroup);
    optForm->setSpacing(8);

    m_connectTimeoutSpin = new QSpinBox();
    m_connectTimeoutSpin->setRange(1, 60);
    m_connectTimeoutSpin->setValue(10);
    m_connectTimeoutSpin->setSuffix(" 秒");
    optForm->addRow("连接超时:", m_connectTimeoutSpin);

    m_readTimeoutSpin = new QSpinBox();
    m_readTimeoutSpin->setRange(1, 300);
    m_readTimeoutSpin->setValue(60);
    m_readTimeoutSpin->setSuffix(" 秒");
    optForm->addRow("读取超时:", m_readTimeoutSpin);

    m_keepAliveCheck = new QCheckBox("启用 KeepAlive");
    m_keepAliveCheck->setChecked(true);
    optForm->addRow("", m_keepAliveCheck);

    m_keepAliveIntervalSpin = new QSpinBox();
    m_keepAliveIntervalSpin->setRange(10, 600);
    m_keepAliveIntervalSpin->setValue(60);
    m_keepAliveIntervalSpin->setSuffix(" 秒");
    optForm->addRow("KeepAlive 间隔:", m_keepAliveIntervalSpin);

    connect(m_keepAliveCheck, &QCheckBox::toggled,
            m_keepAliveIntervalSpin, &QSpinBox::setEnabled);

    mainLayout->addWidget(optGroup);

    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("连接");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

SSHConfig SSHConnectDialog::config() const
{
    SSHConfig cfg;
    cfg.host = m_hostEdit->text().trimmed();
    cfg.port = m_portSpin->value();
    cfg.username = m_userEdit->text().trimmed();
    cfg.password = m_passwordEdit->text();
    cfg.privateKey = m_keyEdit->text().trimmed();
    cfg.passphrase = m_passphraseEdit->text();
    cfg.connectTimeout = m_connectTimeoutSpin->value() * 1000;
    cfg.readTimeout = m_readTimeoutSpin->value() * 1000;
    cfg.keepAlive = m_keepAliveCheck->isChecked();
    cfg.keepAliveInterval = m_keepAliveIntervalSpin->value();
    return cfg;
}

void SSHConnectDialog::setConfig(const SSHConfig& config)
{
    m_hostEdit->setText(config.host);
    m_portSpin->setValue(config.port);
    m_userEdit->setText(config.username);
    m_passwordEdit->setText(config.password);
    m_keyEdit->setText(config.privateKey);
    m_passphraseEdit->setText(config.passphrase);
    m_connectTimeoutSpin->setValue(config.connectTimeout / 1000);
    m_readTimeoutSpin->setValue(config.readTimeout / 1000);
    m_keepAliveCheck->setChecked(config.keepAlive);
    m_keepAliveIntervalSpin->setValue(config.keepAliveInterval);
}