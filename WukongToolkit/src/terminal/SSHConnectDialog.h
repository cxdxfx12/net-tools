#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include "SSHConfig.h"

class SSHConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SSHConnectDialog(QWidget* parent = nullptr);

    SSHConfig config() const;
    void setConfig(const SSHConfig& config);

private:
    void setupUI();

    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_userEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_keyEdit;
    QLineEdit* m_passphraseEdit;
    QSpinBox* m_connectTimeoutSpin;
    QSpinBox* m_readTimeoutSpin;
    QCheckBox* m_keepAliveCheck;
    QSpinBox* m_keepAliveIntervalSpin;
};