#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QDateEdit>
#include <QCheckBox>
#include <QTimer>

// ============================================================================
// ReportWidget — 报表中心 (第43章)
// ============================================================================
class ReportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReportWidget(QWidget* parent = nullptr);

private slots:
    void onGenerateReport();
    void onExport();
    void onToggleAutoReport(bool enabled);
    void onTypeChanged(int index);
    void onTimeRangeChanged(int index);

private:
    void setupUI();
    void setupConnections();

    // ── 报表生成方法 ──
    QString generateInspectionReport();
    QString generateAlarmReport();
    QString generatePerformanceReport();
    QString generateAssetReport();
    QString generateConfigChangeReport();
    QString generateHealthReport();
    QString generateDailyReport();
    QString generateWeeklyReport();
    QString generateMonthlyReport();

    // ── HTML 模板与导出 ──
    QString generateHtmlTemplate(const QString& title, const QString& body);
    void exportToHtml(const QString& content);
    void exportToMarkdown(const QString& content);
    void exportToCsv(const QString& content);
    void addHistoryEntry(const QString& type, const QString& device,
                         const QString& format, const QString& filePath);

    // ── 图表绘制 ──
    void drawChart(QPainter& painter, const QRect& rect);

    // ── 配置区 ──
    QComboBox*   m_typeCombo;
    QComboBox*   m_deviceCombo;
    QComboBox*   m_timeRangeCombo;
    QDateEdit*   m_startDateEdit;
    QDateEdit*   m_endDateEdit;
    QPushButton* m_generateBtn;

    // ── 预览区 ──
    QTextEdit*   m_previewEdit;

    // ── 导出控制 ──
    QComboBox*   m_exportFormatCombo;
    QPushButton* m_exportBtn;

    // ── 定时报表 ──
    QCheckBox*   m_autoReportCheck;
    QComboBox*   m_autoTypeCombo;

    // ── 报表历史 ──
    QTableWidget* m_historyTable;

    // ── 统计图表 ──
    QWidget* m_chartWidget;

    // ── 定时器 ──
    QTimer* m_autoTimer;

    // ── 当前报表内容缓存 ──
    QString m_currentReportTitle;
    QString m_currentReportBody;
};