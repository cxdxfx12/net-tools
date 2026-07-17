#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QProcess>

// ============================================================================
// MacLookupWidget — MAC 地址厂商查询工具 (基于 IEEE OUI 数据库)
// ============================================================================
class MacLookupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MacLookupWidget(QWidget* parent = nullptr);
    ~MacLookupWidget() override;

private slots:
    void onLookup();
    void onBatchLookup();
    void onExport();
    void onClear();
    void onHistoryItemDoubleClicked(int row, int col);

private:
    void setupUI();
    void setupConnections();
    void addResult(const QString& mac, const QString& vendor, const QString& oui);
    void addToHistory(const QString& mac, const QString& vendor);
    static QString lookupOUI(const QString& macPrefix);

    // --- 输入 ---
    QLineEdit*    m_macInput;
    QPushButton*  m_lookupBtn;
    QPushButton*  m_batchLookupBtn;

    // --- 结果 ---
    QLineEdit*    m_ouiEdit;
    QLineEdit*    m_vendorEdit;
    QLineEdit*    m_typeEdit;

    // --- 历史 ---
    QTableWidget* m_historyTable;
    QPushButton*  m_exportBtn;
    QPushButton*  m_clearBtn;

    static constexpr int HIST_COL_MAC    = 0;
    static constexpr int HIST_COL_VENDOR = 1;
    static constexpr int HIST_COL_OUI    = 2;
    static constexpr int HIST_COL_TIME   = 3;
};
