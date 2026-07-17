#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QSpinBox>

// ============================================================================
// PlannerWidget — 子网规划工具 (VLSM)
// ============================================================================
class PlannerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlannerWidget(QWidget* parent = nullptr);
    ~PlannerWidget() override;

private slots:
    void onPlan();
    void onExport();
    void onAnalyzeUtilization();

private:
    void setupUI();
    void setupConnections();
    QString calculateGateway(const QString& network, const QString& mask);
    QString calculateDHCPRange(const QString& network, const QString& mask);

    // ========== 输入区域 ==========
    QLineEdit*    m_projectNameEdit;
    QLineEdit*    m_baseAddrEdit;
    QSpinBox*     m_subnetCountSpin;
    QSpinBox*     m_hostCountSpin;
    QPushButton*  m_planBtn;

    // ========== 结果区域 ==========
    QTableWidget* m_resultTable;

    // ========== 按钮区域 ==========
    QPushButton*  m_exportBtn;
    QPushButton*  m_analyzeBtn;
};