#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QCheckBox>

// ============================================================================
// ConfigGenWidget — 网络设备配置生成器 (Cisco/Huawei/H3C 风格)
// ============================================================================
class ConfigGenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigGenWidget(QWidget* parent = nullptr);
    ~ConfigGenWidget() override;

private slots:
    void onGenerate();
    void onExport();
    void onClear();
    void onVendorChanged(int index);

private:
    void setupUI();
    void setupConnections();
    QString generateCiscoConfig();
    QString generateHuaweiConfig();
    QString generateH3CConfig();
    QString generateBaseConfig();

    // --- 基本参数 ---
    QComboBox*    m_vendorCombo;
    QLineEdit*    m_hostnameEdit;
    QLineEdit*    m_domainEdit;
    QLineEdit*    m_ntpServerEdit;

    // --- 接口参数 ---
    QSpinBox*     m_vlanCountSpin;
    QLineEdit*    m_vlanIpPrefixEdit;
    QLineEdit*    m_mgmtIpEdit;
    QLineEdit*    m_mgmtMaskEdit;
    QLineEdit*    m_mgmtGatewayEdit;

    // --- 安全参数 ---
    QLineEdit*    m_adminUserEdit;
    QLineEdit*    m_adminPassEdit;
    QCheckBox*    m_enableSshCheck;
    QCheckBox*    m_enableSnmpCheck;
    QLineEdit*    m_snmpCommunityEdit;
    QLineEdit*    m_snmpTrapHostEdit;

    // --- 路由参数 ---
    QCheckBox*    m_enableOspfCheck;
    QLineEdit*    m_ospfRouterIdEdit;
    QLineEdit*    m_ospfAreaEdit;
    QCheckBox*    m_enableBgpCheck;
    QLineEdit*    m_bgpAsEdit;
    QLineEdit*    m_bgpNeighborEdit;

    // --- 输出 ---
    QPlainTextEdit* m_outputText;
    QPushButton*    m_generateBtn;
    QPushButton*    m_exportBtn;
    QPushButton*    m_clearBtn;
};
