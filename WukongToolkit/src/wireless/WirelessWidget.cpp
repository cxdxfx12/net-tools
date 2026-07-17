#include "wireless/WirelessWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QTabWidget>
#include <QDateTime>
#include <QRandomGenerator>
#include <QFrame>
#include <QSplitter>

// ═══════════════════════════════════════════════════════════════════════════════
// ChannelChartWidget — 信道利用率柱状图
// ═══════════════════════════════════════════════════════════════════════════════

class ChannelChartWidget : public QWidget
{
public:
    explicit ChannelChartWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(200);
        setStyleSheet("background-color: #1E1F22; border: 1px solid #3C3F41;");
    }

    void setChannelData(const QList<int>& ch24G, const QList<int>& ch5G)
    {
        m_ch24G = ch24G;
        m_ch5G = ch5G;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        if (m_ch24G.isEmpty() && m_ch5G.isEmpty()) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();
        int marginLeft = 50;
        int marginRight = 20;
        int marginTop = 20;
        int marginBottom = 40;

        // Background
        painter.fillRect(rect(), QColor("#1E1F22"));

        int chartW = w - marginLeft - marginRight;
        int chartH = h - marginTop - marginBottom;

        // Determine max value for scaling
        int maxVal = 10;
        for (int v : m_ch24G) maxVal = std::max(maxVal, v);
        for (int v : m_ch5G) maxVal = std::max(maxVal, v);
        maxVal = std::max(maxVal, 100);

        // Grid lines
        painter.setPen(QPen(QColor("#3C3F41"), 1, Qt::DotLine));
        int yStep = chartH / 4;
        for (int i = 0; i <= 4; ++i) {
            int y = marginTop + i * yStep;
            painter.drawLine(marginLeft, y, w - marginRight, y);
        }

        // Y-axis labels
        painter.setPen(QColor("#8C8C8C"));
        painter.setFont(QFont("Arial", 9));
        for (int i = 0; i <= 4; ++i) {
            int y = marginTop + i * yStep;
            int val = maxVal * (4 - i) / 4;
            painter.drawText(2, y + 4, QString("%1%").arg(val));
        }

        // ── 2.4G channels ──
        int total24GBars = m_ch24G.size();
        int total5GBars = m_ch5G.size();
        int totalBars = total24GBars + total5GBars;
        int spacing = 6;
        int barW = (chartW - (totalBars + 1) * spacing) / totalBars;
        if (barW < 4) barW = 4;

        int x = marginLeft + spacing;

        // 2.4G section label
        if (total24GBars > 0) {
            painter.setPen(QColor("#E6A23C"));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            int labelX = x;
            painter.drawText(labelX, marginTop - 4, "2.4 GHz");
        }

        for (int i = 0; i < total24GBars; ++i) {
            int barH = (int)((qreal)m_ch24G[i] / maxVal * chartH);
            int y = marginTop + chartH - barH;

            QColor barColor = m_ch24G[i] > 70 ? QColor("#F56C6C") :
                              m_ch24G[i] > 40 ? QColor("#E6A23C") : QColor("#67C23A");
            painter.fillRect(x, y, barW, barH, barColor);

            // Channel label
            painter.setPen(QColor("#8C8C8C"));
            painter.setFont(QFont("Arial", 8));
            QString label = QString("CH%1").arg(i + 1);
            painter.drawText(x, h - marginBottom + 14, barW, 16, Qt::AlignCenter, label);

            x += barW + spacing;
        }

        // Separator
        x += 8;

        // 5G section label
        if (total5GBars > 0) {
            painter.setPen(QColor("#409EFF"));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            painter.drawText(x, marginTop - 4, "5 GHz");
        }

        for (int i = 0; i < total5GBars; ++i) {
            int barH = (int)((qreal)m_ch5G[i] / maxVal * chartH);
            int y = marginTop + chartH - barH;

            QColor barColor = m_ch5G[i] > 70 ? QColor("#F56C6C") :
                              m_ch5G[i] > 40 ? QColor("#E6A23C") : QColor("#409EFF");
            painter.fillRect(x, y, barW, barH, barColor);

            painter.setPen(QColor("#8C8C8C"));
            painter.setFont(QFont("Arial", 8));
            QString label = QString("CH%1").arg(36 + i * 4);
            painter.drawText(x, h - marginBottom + 14, barW, 16, Qt::AlignCenter, label);

            x += barW + spacing;
        }

        // X-axis title
        painter.setPen(QColor("#8C8C8C"));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(marginLeft, h - 4, chartW, 16, Qt::AlignCenter, "信道");
    }

private:
    QList<int> m_ch24G;
    QList<int> m_ch5G;
};

// ═══════════════════════════════════════════════════════════════════════════════
// WirelessWidget
// ═══════════════════════════════════════════════════════════════════════════════

WirelessWidget::WirelessWidget(QWidget* parent)
    : QWidget(parent)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    generateMockAPData();
    generateMockSSIDData();
    generateMockClientData();
    onRefresh();
}

WirelessWidget::~WirelessWidget()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

void WirelessWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common style helpers ──
    auto styleCombo = [](QComboBox* combo, int minWidth = 200) {
        combo->setMinimumWidth(minWidth);
        combo->setStyleSheet(
            "QComboBox {"
            "  background: #25262B; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; padding: 4px 8px;"
            "  border-radius: 3px; font-size: 13px;"
            "}"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView {"
            "  background: #25262B; color: #DCDCDC;"
            "  selection-background-color: #3C3F41;"
            "}"
        );
    };

    auto styleTable = [](QTableWidget* table) {
        table->setStyleSheet(
            "QTableWidget {"
            "  background-color: #1E1F22; color: #DCDCDC;"
            "  border: 1px solid #3C3F41; font-size: 12px;"
            "  gridline-color: #2C2D30;"
            "}"
            "QTableWidget::item { padding: 3px 6px; }"
            "QTableWidget::item:selected { background-color: #3C3F41; }"
            "QHeaderView::section {"
            "  background-color: #25262B; color: #8C8C8C;"
            "  border: none; border-bottom: 2px solid #3C3F41;"
            "  padding: 4px 8px; font-size: 12px; font-weight: bold;"
            "}"
        );
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
    };

    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString("QPushButton {"
                    "  background-color: %1; color: white;"
                    "  border: none; padding: 8px 20px; border-radius: 4px;"
                    "  font-size: 13px; font-weight: bold;"
                    "}"
                    "QPushButton:hover { background-color: %2; }"
                    "QPushButton:disabled { background-color: #5C5C5C; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(34);
    };

    auto styleCard = [](QFrame* card) {
        card->setStyleSheet(
            "QFrame {"
            "  background-color: #25262B;"
            "  border: 1px solid #3C3F41;"
            "  border-radius: 6px;"
            "}"
        );
    };

    auto styleGroupBox = [](QGroupBox* group, const QString& title) {
        group->setStyleSheet(
            "QGroupBox {"
            "  color: #409EFF; font-size: 13px; font-weight: bold;"
            "  border: 1px solid #3C3F41; border-radius: 4px; margin-top: 8px;"
            "  padding-top: 16px;"
            "}"
            "QGroupBox::title {"
            "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
            "}"
        );
        group->setTitle(title);
    };

    auto styleLabel = [](QLabel* label, const QString& color = "#8C8C8C", int fontSize = 12) {
        label->setStyleSheet(
            QString("font-size: %1px; color: %2;").arg(fontSize).arg(color)
        );
    };

    // ── Top: AC selection and controls ──
    auto* topGroup = new QGroupBox();
    styleGroupBox(topGroup, "无线管理中心");
    auto* topLayout = new QHBoxLayout(topGroup);
    topLayout->setSpacing(12);

    auto* acLabel = new QLabel("AC 控制器:");
    styleLabel(acLabel, "#8C8C8C", 13);
    m_acCombo = new QComboBox();
    styleCombo(m_acCombo, 200);

    auto* refreshBtn = new QPushButton("立即刷新");
    styleButton(refreshBtn, "#67C23A", "#85CE61");

    m_exportBtn = new QPushButton("导出 CSV");
    styleButton(m_exportBtn, "#E6A23C", "#EBB563");

    topLayout->addWidget(acLabel);
    topLayout->addWidget(m_acCombo);
    topLayout->addWidget(refreshBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(topGroup);

    // ── AC Status Overview ──
    auto* statusGroup = new QGroupBox();
    styleGroupBox(statusGroup, "AC 状态概览");
    auto* statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(16);

    auto makeStatusCard = [&](const QString& title, const QString& icon, QLabel*& valueLabel) -> QWidget* {
        auto* card = new QFrame();
        styleCard(card);
        auto* cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(10);

        auto* iconLabel = new QLabel(icon);
        iconLabel->setStyleSheet("font-size: 22px; border: none;");
        auto* textLayout = new QVBoxLayout();
        auto* titleLabel = new QLabel(title);
        styleLabel(titleLabel, "#8C8C8C", 11);
        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet("font-size: 16px; color: #409EFF; font-weight: bold; border: none;");
        textLayout->addWidget(titleLabel);
        textLayout->addWidget(valueLabel);

        cardLayout->addWidget(iconLabel);
        cardLayout->addLayout(textLayout);
        cardLayout->addStretch();
        return card;
    };

    statusLayout->addWidget(makeStatusCard("AC 名称", "🖥", m_acNameLabel));
    statusLayout->addWidget(makeStatusCard("AC IP", "🌐", m_acIpLabel));
    statusLayout->addWidget(makeStatusCard("在线 AP", "📡", m_onlineAPLabel));
    statusLayout->addWidget(makeStatusCard("客户端数", "👥", m_clientCountLabel));
    statusLayout->addWidget(makeStatusCard("运行时间", "⏱", m_uptimeLabel));

    mainLayout->addWidget(statusGroup);

    // ── Middle: AP List + SSID Management (Tab Widget) ──
    auto* middleSplitter = new QSplitter(Qt::Horizontal);
    middleSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #3C3F41; width: 2px; }"
    );

    // Left: AP List
    auto* apGroup = new QGroupBox();
    styleGroupBox(apGroup, "AP 列表");
    auto* apLayout = new QVBoxLayout(apGroup);
    apLayout->setContentsMargins(4, 8, 4, 4);

    m_apTable = new QTableWidget(0, 8);
    m_apTable->setHorizontalHeaderLabels({
        "AP 名称", "IP 地址", "MAC 地址", "型号",
        "2.4G 信道", "5G 信道", "状态", "客户端数"
    });
    m_apTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_apTable->setColumnWidth(1, 110);
    m_apTable->setColumnWidth(2, 130);
    m_apTable->setColumnWidth(3, 100);
    m_apTable->setColumnWidth(4, 80);
    m_apTable->setColumnWidth(5, 80);
    m_apTable->setColumnWidth(6, 70);
    m_apTable->setColumnWidth(7, 80);
    styleTable(m_apTable);

    apLayout->addWidget(m_apTable);

    // Right: Tab widget for SSID + Client + Channel
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #3C3F41; background-color: #1E1F22;"
        "}"
        "QTabBar::tab {"
        "  background-color: #25262B; color: #8C8C8C;"
        "  padding: 6px 16px; border: 1px solid #3C3F41;"
        "  border-bottom: none; margin-right: 2px; font-size: 12px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #1E1F22; color: #409EFF;"
        "  border-bottom: 2px solid #409EFF;"
        "}"
    );

    // SSID tab
    auto* ssidTab = new QWidget();
    auto* ssidLayout = new QVBoxLayout(ssidTab);
    ssidLayout->setContentsMargins(4, 4, 4, 4);

    m_ssidTable = new QTableWidget(0, 5);
    m_ssidTable->setHorizontalHeaderLabels({
        "SSID 名称", "安全类型", "频段", "VLAN", "状态"
    });
    m_ssidTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ssidTable->setColumnWidth(1, 120);
    m_ssidTable->setColumnWidth(2, 80);
    m_ssidTable->setColumnWidth(3, 60);
    m_ssidTable->setColumnWidth(4, 80);
    styleTable(m_ssidTable);

    ssidLayout->addWidget(m_ssidTable);
    tabWidget->addTab(ssidTab, "SSID 管理");

    // Client tab
    auto* clientTab = new QWidget();
    auto* clientLayout = new QVBoxLayout(clientTab);
    clientLayout->setContentsMargins(4, 4, 4, 4);

    m_clientTable = new QTableWidget(0, 6);
    m_clientTable->setHorizontalHeaderLabels({
        "客户端 MAC", "IP 地址", "关联 AP", "SSID",
        "信号强度 (dBm)", "连接时长"
    });
    m_clientTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_clientTable->setColumnWidth(1, 110);
    m_clientTable->setColumnWidth(2, 140);
    m_clientTable->setColumnWidth(3, 120);
    m_clientTable->setColumnWidth(4, 110);
    m_clientTable->setColumnWidth(5, 120);
    styleTable(m_clientTable);

    clientLayout->addWidget(m_clientTable);
    tabWidget->addTab(clientTab, "无线客户端");

    // Channel chart tab
    auto* channelTab = new QWidget();
    auto* channelLayout = new QVBoxLayout(channelTab);
    channelLayout->setContentsMargins(4, 4, 4, 4);

    m_channelChart = new ChannelChartWidget();
    channelLayout->addWidget(m_channelChart);

    tabWidget->addTab(channelTab, "信道利用率");

    rightLayout->addWidget(tabWidget);
    rightWidget->setLayout(rightLayout);

    middleSplitter->addWidget(apGroup);
    middleSplitter->addWidget(rightWidget);
    middleSplitter->setStretchFactor(0, 3);
    middleSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(middleSplitter, 1);

    // ── Connect refresh button ──
    connect(refreshBtn, &QPushButton::clicked, this, &WirelessWidget::onRefresh);

    setStyleSheet("background-color: #1E1F22;");
}

// ─── Connections ─────────────────────────────────────────────────────────────

void WirelessWidget::setupConnections()
{
    connect(m_acCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WirelessWidget::onACChanged);
    connect(m_exportBtn, &QPushButton::clicked, this, &WirelessWidget::onExport);
    connect(m_refreshTimer, &QTimer::timeout, this, &WirelessWidget::onRefresh);
    m_refreshTimer->start(30000); // 30 seconds auto-refresh
}

// ─── Refresh ─────────────────────────────────────────────────────────────────

void WirelessWidget::onRefresh()
{
    // Re-randomize some metrics for realism
    auto* rng = QRandomGenerator::global();

    // Update AC status
    m_acNameLabel->setText("WX-AC-5500");
    m_acIpLabel->setText("10.1.0.1");

    int onlineCount = 0;
    int totalClients = 0;
    for (auto& ap : m_apList) {
        if (ap.status == "Online") {
            onlineCount++;
            ap.clientCount = rng->bounded(0, 30);
            totalClients += ap.clientCount;
        }
    }
    m_onlineAPLabel->setText(QString::number(onlineCount));
    m_clientCountLabel->setText(QString::number(totalClients));

    int days = rng->bounded(10, 180);
    int hours = rng->bounded(0, 24);
    m_uptimeLabel->setText(QString("%1天 %2时").arg(days).arg(hours));

    // Populate AP table
    m_apTable->setRowCount(m_apList.size());
    for (int i = 0; i < m_apList.size(); ++i) {
        const auto& ap = m_apList[i];
        m_apTable->setItem(i, 0, new QTableWidgetItem(ap.name));
        m_apTable->setItem(i, 1, new QTableWidgetItem(ap.ip));
        m_apTable->setItem(i, 2, new QTableWidgetItem(ap.mac));
        m_apTable->setItem(i, 3, new QTableWidgetItem(ap.model));
        m_apTable->setItem(i, 4, new QTableWidgetItem(QString::number(ap.channel24G)));
        m_apTable->setItem(i, 5, new QTableWidgetItem(QString::number(ap.channel5G)));

        auto* statusItem = new QTableWidgetItem(ap.status);
        if (ap.status == "Online") {
            statusItem->setForeground(QColor("#67C23A"));
        } else {
            statusItem->setForeground(QColor("#F56C6C"));
        }
        m_apTable->setItem(i, 6, statusItem);

        m_apTable->setItem(i, 7, new QTableWidgetItem(QString::number(ap.clientCount)));
    }

    // Populate SSID table
    m_ssidTable->setRowCount(m_ssidList.size());
    for (int i = 0; i < m_ssidList.size(); ++i) {
        const auto& ssid = m_ssidList[i];
        m_ssidTable->setItem(i, 0, new QTableWidgetItem(ssid.name));
        m_ssidTable->setItem(i, 1, new QTableWidgetItem(ssid.securityType));
        m_ssidTable->setItem(i, 2, new QTableWidgetItem(ssid.band));
        m_ssidTable->setItem(i, 3, new QTableWidgetItem(QString::number(ssid.vlan)));

        auto* statusItem = new QTableWidgetItem(ssid.status);
        if (ssid.status == "启用") {
            statusItem->setForeground(QColor("#67C23A"));
        } else {
            statusItem->setForeground(QColor("#F56C6C"));
        }
        m_ssidTable->setItem(i, 4, statusItem);
    }

    // Populate Client table
    m_clientTable->setRowCount(m_clientList.size());
    for (int i = 0; i < m_clientList.size(); ++i) {
        const auto& client = m_clientList[i];
        m_clientTable->setItem(i, 0, new QTableWidgetItem(client.mac));
        m_clientTable->setItem(i, 1, new QTableWidgetItem(client.ip));
        m_clientTable->setItem(i, 2, new QTableWidgetItem(client.apName));
        m_clientTable->setItem(i, 3, new QTableWidgetItem(client.ssid));

        auto* signalItem = new QTableWidgetItem(QString::number(client.signalStrength));
        if (client.signalStrength > -50) {
            signalItem->setForeground(QColor("#67C23A"));
        } else if (client.signalStrength > -70) {
            signalItem->setForeground(QColor("#E6A23C"));
        } else {
            signalItem->setForeground(QColor("#F56C6C"));
        }
        m_clientTable->setItem(i, 4, signalItem);

        int mins = client.connectedDuration / 60;
        int h = mins / 60;
        int m = mins % 60;
        QString durationStr;
        if (h > 0) {
            durationStr = QString("%1时 %2分").arg(h).arg(m);
        } else {
            durationStr = QString("%1分").arg(m);
        }
        m_clientTable->setItem(i, 5, new QTableWidgetItem(durationStr));
    }

    // Update channel chart
    paintChannelChart();

    Logger::instance().info("Wireless Center",
        QString("刷新完成: %1 在线AP, %2 客户端").arg(onlineCount).arg(totalClients));
}

// ─── Export CSV ──────────────────────────────────────────────────────────────

void WirelessWidget::onExport()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出无线管理数据",
        QString("wireless_center_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        Logger::instance().error("Wireless Center", "Failed to open CSV file: " + filePath);
        return;
    }

    QTextStream stream(&file);
    stream << "\xEF\xBB\xBF"; // BOM for Excel UTF-8

    // AP table
    stream << "=== AP 列表 ===\n";
    stream << "AP名称,IP地址,MAC地址,型号,2.4G信道,5G信道,状态,客户端数\n";
    for (int row = 0; row < m_apTable->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < m_apTable->columnCount(); ++col) {
            auto* item = m_apTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            cells << cell;
        }
        stream << cells.join(",") << "\n";
    }

    // SSID table
    stream << "\n=== SSID 管理 ===\n";
    stream << "SSID名称,安全类型,频段,VLAN,状态\n";
    for (int row = 0; row < m_ssidTable->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < m_ssidTable->columnCount(); ++col) {
            auto* item = m_ssidTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            cells << cell;
        }
        stream << cells.join(",") << "\n";
    }

    // Client table
    stream << "\n=== 无线客户端 ===\n";
    stream << "客户端MAC,IP地址,关联AP,SSID,信号强度(dBm),连接时长\n";
    for (int row = 0; row < m_clientTable->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < m_clientTable->columnCount(); ++col) {
            auto* item = m_clientTable->item(row, col);
            QString cell = item ? item->text() : "";
            if (cell.contains(',') || cell.contains('"')) {
                cell.replace("\"", "\"\"");
                cell = "\"" + cell + "\"";
            }
            cells << cell;
        }
        stream << cells.join(",") << "\n";
    }

    file.close();

    int totalRows = m_apTable->rowCount() + m_ssidTable->rowCount() + m_clientTable->rowCount();
    Logger::instance().info("Wireless Center",
        QString("导出 %1 条记录到: %2").arg(totalRows).arg(filePath));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条记录到:\n%2").arg(totalRows).arg(filePath));
}

// ─── AC Changed ──────────────────────────────────────────────────────────────

void WirelessWidget::onACChanged(int index)
{
    Logger::instance().info("Wireless Center",
        QString("切换 AC 控制器: index=%1").arg(index));
    onRefresh();
}

// ─── Channel Chart ───────────────────────────────────────────────────────────

void WirelessWidget::paintChannelChart()
{
    auto* rng = QRandomGenerator::global();

    // 2.4G channels: 1-13
    QList<int> ch24G;
    for (int i = 0; i < 13; ++i) {
        ch24G.append(rng->bounded(10, 90));
    }

    // 5G channels: 36,40,44,48,52,56,60,64,149,153,157,161,165
    QList<int> ch5G;
    for (int i = 0; i < 13; ++i) {
        ch5G.append(rng->bounded(5, 80));
    }

    auto* chart = static_cast<ChannelChartWidget*>(m_channelChart);
    chart->setChannelData(ch24G, ch5G);
}

// ─── Mock AP Data ────────────────────────────────────────────────────────────

void WirelessWidget::generateMockAPData()
{
    m_apList.clear();

    m_apList.append({
        "AP-1F-Lobby", "10.1.10.11", "00:1A:2B:3C:4D:01",
        "WX-AP510", 1, 36, "Online", 12
    });
    m_apList.append({
        "AP-1F-Office", "10.1.10.12", "00:1A:2B:3C:4D:02",
        "WX-AP510", 6, 40, "Online", 8
    });
    m_apList.append({
        "AP-2F-East", "10.1.10.21", "00:1A:2B:3C:4D:03",
        "WX-AP520", 11, 44, "Online", 15
    });
    m_apList.append({
        "AP-2F-West", "10.1.10.22", "00:1A:2B:3C:4D:04",
        "WX-AP520", 1, 52, "Online", 6
    });
    m_apList.append({
        "AP-3F-Conf", "10.1.10.31", "00:1A:2B:3C:4D:05",
        "WX-AP530", 6, 149, "Online", 22
    });
    m_apList.append({
        "AP-3F-Dev", "10.1.10.32", "00:1A:2B:3C:4D:06",
        "WX-AP510", 11, 153, "Online", 3
    });
    m_apList.append({
        "AP-4F-Exec", "10.1.10.41", "00:1A:2B:3C:4D:07",
        "WX-AP530", 1, 157, "Offline", 0
    });
    m_apList.append({
        "AP-Outdoor", "10.1.10.99", "00:1A:2B:3C:4D:08",
        "WX-AP550", 6, 161, "Offline", 0
    });

    // Populate AC combo
    m_acCombo->clear();
    m_acCombo->addItem("WX-AC-5500 (10.1.0.1)", 0);
    m_acCombo->addItem("WX-AC-3300 (10.1.0.2)", 1);
    m_acCombo->setCurrentIndex(0);

    Logger::instance().debug("Wireless Center",
        QString("生成 %1 个模拟 AP 数据").arg(m_apList.size()));
}

// ─── Mock SSID Data ──────────────────────────────────────────────────────────

void WirelessWidget::generateMockSSIDData()
{
    m_ssidList.clear();

    m_ssidList.append({
        "Corp-Secure", "WPA3-Enterprise", "双频", 100, "启用"
    });
    m_ssidList.append({
        "Corp-Staff", "WPA2-PSK", "双频", 200, "启用"
    });
    m_ssidList.append({
        "Guest-WiFi", "Open + Portal", "2.4G", 300, "启用"
    });

    Logger::instance().debug("Wireless Center",
        QString("生成 %1 个模拟 SSID 数据").arg(m_ssidList.size()));
}

// ─── Mock Client Data ────────────────────────────────────────────────────────

void WirelessWidget::generateMockClientData()
{
    m_clientList.clear();
    auto* rng = QRandomGenerator::global();

    struct ClientTemplate {
        QString macPrefix;
        QString ipPrefix;
        QString ssid;
    };

    QList<ClientTemplate> templates = {
        {"A4:CF:12", "10.1.100.", "Corp-Secure"},
        {"B8:27:EB", "10.1.100.", "Corp-Secure"},
        {"DC:A6:32", "10.1.100.", "Corp-Staff"},
        {"E0:55:3D", "10.1.100.", "Corp-Staff"},
        {"F4:0F:24", "10.1.200.", "Guest-WiFi"},
    };

    QStringList apNames = {"AP-1F-Lobby", "AP-1F-Office", "AP-2F-East",
                           "AP-2F-West", "AP-3F-Conf", "AP-3F-Dev"};

    for (int i = 0; i < 20; ++i) {
        const auto& tmpl = templates[i % templates.size()];
        WirelessClient client;
        client.mac = QString("%1:%2:%3:%4")
            .arg(tmpl.macPrefix)
            .arg(rng->bounded(0, 256), 2, 16, QChar('0'))
            .arg(rng->bounded(0, 256), 2, 16, QChar('0'))
            .arg(rng->bounded(0, 256), 2, 16, QChar('0')).toUpper();
        client.ip = QString("%1%2").arg(tmpl.ipPrefix).arg(rng->bounded(10, 250));
        client.apName = apNames[rng->generate() % apNames.size()];
        client.ssid = tmpl.ssid;
        client.signalStrength = rng->bounded(-75, -30);
        client.connectedDuration = rng->bounded(60, 86400); // 1 min to 24 hours

        m_clientList.append(client);
    }

    Logger::instance().debug("Wireless Center",
        QString("生成 %1 个模拟客户端数据").arg(m_clientList.size()));
}