#pragma once

#include <QWidget>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QList>

// ============================================================================
// 审计结果结构
// ============================================================================
struct AuditResult
{
    QString ruleName;
    QString checkResult;   // 通过/警告/失败
    QString severity;      // 低/中/高/严重
    QString detail;
    QString suggestion;
    int     score;
};

// ============================================================================
// SecurityWidget — 安全审计中心 (第53章)
// ============================================================================
class SecurityWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SecurityWidget(QWidget* parent = nullptr);
    ~SecurityWidget() override;

private slots:
    void onStartAudit();
    void onExport();
    void onDeviceChanged(int index);
    void onTemplateChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void runAuditRule(const QString& ruleName, const QString& deviceName);
    void calculateScore();
    void updateRiskList();

    // --- UI 组件 ---
    // 左栏
    QComboBox*    m_deviceCombo;
    QComboBox*    m_templateCombo;
    QListWidget*  m_ruleList;
    QPushButton*  m_startBtn;

    // 右栏
    QTableWidget* m_resultTable;
    QLabel*       m_scoreLabel;
    QTableWidget* m_riskTable;
    QPushButton*  m_exportBtn;

    // --- 审计数据 ---
    QList<AuditResult> m_results;
    int m_totalScore;
    int m_maxScore;
};