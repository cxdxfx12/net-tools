#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>

// ============================================================================
// MacToolkitWidget — MAC 地址工具包
// ============================================================================
class MacToolkitWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MacToolkitWidget(QWidget* parent = nullptr);
    ~MacToolkitWidget() override;

private slots:
    void onAnalyze();
    void onGenerate();
    void onFormatConvert();
    void onExport();

private:
    void setupUI();
    void setupConnections();
    void addHistoryEntry(const QString& mac, const QString& vendor, const QString& type);
    static QString lookupVendor(const QString& mac);
    static QString detectType(const QString& mac);
    static QString convertFormat(const QString& mac, const QString& format);

    // ========== 输入区 ==========
    QLineEdit*    m_macInput;
    QPushButton*  m_analyzeBtn;
    QPushButton*  m_generateBtn;

    // ========== 结果区 ==========
    QLineEdit*    m_vendorEdit;
    QLineEdit*    m_typeEdit;
    QLineEdit*    m_scopeEdit;
    QComboBox*    m_formatCombo;
    QLineEdit*    m_formatResult;

    // ========== 历史区 ==========
    QTableWidget* m_historyTable;
    QPushButton*  m_exportBtn;

    // ========== 表格列索引 ==========
    static constexpr int HIST_COL_MAC    = 0;
    static constexpr int HIST_COL_VENDOR = 1;
    static constexpr int HIST_COL_TYPE   = 2;
    static constexpr int HIST_COL_TIME   = 3;
};