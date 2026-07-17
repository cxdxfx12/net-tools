#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

// ============================================================================
// ConfigDiffWidget — 网络设备配置差异对比工具
// ============================================================================
class ConfigDiffWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigDiffWidget(QWidget* parent = nullptr);
    ~ConfigDiffWidget() override;

private slots:
    void onLoadLeft();
    void onLoadRight();
    void onCompare();
    void onSwap();
    void onClear();
    void onExportDiff();

private:
    void setupUI();
    void setupConnections();
    void highlightDifferences();
    QString generateUnifiedDiff(const QString& left, const QString& right);

    // --- 左侧 ---
    QPlainTextEdit* m_leftEdit;
    QPushButton*    m_loadLeftBtn;
    QLabel*         m_leftLabel;

    // --- 右侧 ---
    QPlainTextEdit* m_rightEdit;
    QPushButton*    m_loadRightBtn;
    QLabel*         m_rightLabel;

    // --- 操作 ---
    QPushButton*    m_compareBtn;
    QPushButton*    m_swapBtn;
    QPushButton*    m_clearBtn;
    QPushButton*    m_exportBtn;

    // --- 差异统计 ---
    QLabel*         m_statsLabel;
};
