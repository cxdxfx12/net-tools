#include "topology/TopologyWidget.h"
#include "log/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QSplitter>
#include <QScrollBar>
#include <QQueue>
#include <QSet>
#include <QtMath>
#include <QRandomGenerator>

// ═══════════════════════════════════════════════════════════════════════════════
// TopologyWidget
// ═══════════════════════════════════════════════════════════════════════════════

TopologyWidget::TopologyWidget(QWidget* parent)
    : QWidget(parent)
    , m_scene(nullptr)
    , m_view(nullptr)
    , m_autoLayoutBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_searchEdit(nullptr)
    , m_exportImageBtn(nullptr)
    , m_zoomSlider(nullptr)
    , m_deviceList(nullptr)
    , m_deviceDetailGroup(nullptr)
    , m_deviceDetailLabel(nullptr)
    , m_linkDetailGroup(nullptr)
    , m_linkDetailLabel(nullptr)
    , m_layoutMode(0)
{
    setupUI();
    setupConnections();
    loadDemoTopology();
}

TopologyWidget::~TopologyWidget()
{
}

// ── UI Setup ─────────────────────────────────────────────────────────────────

void TopologyWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // ── Common style helpers ──
    auto styleButton = [](QPushButton* btn, const QString& bgColor, const QString& hoverColor) {
        btn->setStyleSheet(
            QString("QPushButton {"
                    "  background-color: %1; color: white;"
                    "  border: none; padding: 8px 16px; border-radius: 4px;"
                    "  font-size: 13px; font-weight: bold;"
                    "}"
                    "QPushButton:hover { background-color: %2; }"
                    "QPushButton:disabled { background-color: #484F58; }")
                .arg(bgColor, hoverColor)
        );
        btn->setFixedHeight(32);
    };

    auto styleSearch = [](QLineEdit* edit) {
        edit->setPlaceholderText(QStringLiteral("搜索设备名称或 IP..."));
        edit->setStyleSheet(
            "QLineEdit {"
            "  background: #161B22; color: #E6EDF3;"
            "  border: 1px solid #30363D; padding: 6px 12px;"
            "  border-radius: 4px; font-size: 13px;"
            "}"
            "QLineEdit:focus { border-color: #58A6FF; }"
        );
        edit->setFixedWidth(220);
    };

    auto styleGroup = [](QGroupBox* group, const QString& title) {
        group->setTitle(title);
        group->setStyleSheet(
            "QGroupBox {"
            "  color: #58A6FF; font-size: 13px; font-weight: bold;"
            "  border: 1px solid #30363D; border-radius: 4px; margin-top: 8px;"
            "  padding-top: 16px;"
            "}"
            "QGroupBox::title {"
            "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
            "}"
        );
    };

    auto styleLabel = [](QLabel* label, const QString& color = "#8B949E", int fontSize = 12) {
        label->setStyleSheet(
            QString("font-size: %1px; color: %2;").arg(fontSize).arg(color)
        );
    };

    // ── Top Toolbar ──
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(10);

    m_autoLayoutBtn = new QPushButton(QStringLiteral("自动布局"));
    styleButton(m_autoLayoutBtn, "#58A6FF", "#79C0FF");

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    styleButton(m_refreshBtn, "#3FB950", "#56D364");

    m_searchEdit = new QLineEdit();
    styleSearch(m_searchEdit);

    m_exportImageBtn = new QPushButton(QStringLiteral("导出图片"));
    styleButton(m_exportImageBtn, "#D29922", "#DBAB4A");

    auto* zoomLabel = new QLabel(QStringLiteral("缩放:"));
    styleLabel(zoomLabel, "#E6EDF3", 13);

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 300);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(150);
    m_zoomSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  background: #161B22; height: 4px; border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #58A6FF; width: 14px; height: 14px;"
        "  margin: -5px 0; border-radius: 7px;"
        "}"
        "QSlider::sub-page:horizontal { background: #58A6FF; border-radius: 2px; }"
    );

    toolbarLayout->addWidget(m_autoLayoutBtn);
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addSpacing(16);
    toolbarLayout->addWidget(m_searchEdit);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_exportImageBtn);
    toolbarLayout->addSpacing(16);
    toolbarLayout->addWidget(zoomLabel);
    toolbarLayout->addWidget(m_zoomSlider);

    mainLayout->addLayout(toolbarLayout);

    // ── Middle: Scene + Right Panel ──
    auto* middleSplitter = new QSplitter(Qt::Horizontal);
    middleSplitter->setStyleSheet(
        "QSplitter::handle { background-color: #30363D; width: 2px; }"
    );

    // ── Graphics View ──
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QBrush(QColor("#1A1A1A")));
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);

    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setStyleSheet(
        "QGraphicsView {"
        "  border: 1px solid #30363D;"
        "  background-color: #1A1A1A;"
        "}"
    );
    m_view->setMinimumWidth(400);

    // ── Right Panel ──
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);
    rightPanel->setFixedWidth(260);

    // Device list
    auto* deviceListGroup = new QGroupBox();
    styleGroup(deviceListGroup, QStringLiteral("设备列表"));
    auto* deviceListLayout = new QVBoxLayout(deviceListGroup);

    m_deviceList = new QListWidget();
    m_deviceList->setStyleSheet(
        "QListWidget {"
        "  background: #0D1117; color: #E6EDF3;"
        "  border: 1px solid #30363D; font-size: 12px;"
        "}"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:selected { background-color: #30363D; }"
        "QListWidget::item:hover { background-color: #21262D; }"
    );
    m_deviceList->setDragEnabled(true);
    deviceListLayout->addWidget(m_deviceList);
    rightLayout->addWidget(deviceListGroup);

    // Device detail panel
    m_deviceDetailGroup = new QGroupBox();
    styleGroup(m_deviceDetailGroup, QStringLiteral("设备详情"));
    auto* deviceDetailLayout = new QVBoxLayout(m_deviceDetailGroup);

    m_deviceDetailLabel = new QLabel(QStringLiteral("请选择一个设备节点"));
    m_deviceDetailLabel->setWordWrap(true);
    
    m_deviceDetailLabel->setMinimumHeight(100);
    deviceDetailLayout->addWidget(m_deviceDetailLabel);
    rightLayout->addWidget(m_deviceDetailGroup);

    // Link detail panel
    m_linkDetailGroup = new QGroupBox();
    styleGroup(m_linkDetailGroup, QStringLiteral("链路详情"));
    auto* linkDetailLayout = new QVBoxLayout(m_linkDetailGroup);

    m_linkDetailLabel = new QLabel(QStringLiteral("请选择一条链路"));
    m_linkDetailLabel->setWordWrap(true);
    
    m_linkDetailLabel->setMinimumHeight(80);
    linkDetailLayout->addWidget(m_linkDetailLabel);
    rightLayout->addWidget(m_linkDetailGroup);

    rightLayout->addStretch();

    middleSplitter->addWidget(m_view);
    middleSplitter->addWidget(rightPanel);
    middleSplitter->setStretchFactor(0, 3);
    middleSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(middleSplitter, 1);

    // ── Bottom Status Bar ──
    auto* statusBar = new QWidget();
    statusBar->setFixedHeight(28);
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(12, 0, 12, 0);
    statusLayout->setSpacing(20);

    auto* statusLabel = new QLabel(QStringLiteral("就绪 | 节点: 0 | 链路: 0"));
    styleLabel(statusLabel, "#8B949E", 12);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();

    mainLayout->addWidget(statusBar);
}

// ── Connections ──────────────────────────────────────────────────────────────

void TopologyWidget::setupConnections()
{
    connect(m_autoLayoutBtn, &QPushButton::clicked, this, &TopologyWidget::onAutoLayout);
    connect(m_refreshBtn, &QPushButton::clicked, this, &TopologyWidget::onRefresh);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &TopologyWidget::onSearchDevice);
    connect(m_exportImageBtn, &QPushButton::clicked, this, &TopologyWidget::onExportImage);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &TopologyWidget::onZoomChanged);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &TopologyWidget::onSceneSelectionChanged);
}

// ── Auto Layout ──────────────────────────────────────────────────────────────

void TopologyWidget::onAutoLayout()
{
    m_layoutMode = (m_layoutMode + 1) % 3;

    switch (m_layoutMode) {
    case 0: applyForceDirectedLayout(); break;
    case 1: applyTreeLayout(); break;
    case 2: applyCircularLayout(); break;
    }

    const char* names[] = { "力导向布局", "树形布局", "圆形布局" };
    Logger::instance().info(QStringLiteral("Topology"),
        QStringLiteral("应用布局: %1").arg(names[m_layoutMode]));
}

// ── Refresh ──────────────────────────────────────────────────────────────────

void TopologyWidget::onRefresh()
{
    Logger::instance().info(QStringLiteral("Topology"), QStringLiteral("刷新拓扑"));
    clearTopology();
    loadDemoTopology();
    onAutoLayout();
}

// ── Search Device ────────────────────────────────────────────────────────────

void TopologyWidget::onSearchDevice()
{
    QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) return;

    for (const auto& node : m_nodes) {
        if (node.name.contains(keyword, Qt::CaseInsensitive) ||
            node.ip.contains(keyword, Qt::CaseInsensitive)) {
            // Highlight by centering on the node
            m_view->centerOn(node.pos);
            // Select the node item
            auto it = m_nodeItems.find(node.id);
            if (it != m_nodeItems.end()) {
                m_scene->clearSelection();
                it.value()->setSelected(true);
            }

            Logger::instance().info(QStringLiteral("Topology"),
                QStringLiteral("搜索到设备: %1 (%2)").arg(node.name, node.ip));
            return;
        }
    }

    Logger::instance().info(QStringLiteral("Topology"),
        QStringLiteral("未找到设备: %1").arg(keyword));
}

// ── Export Image ─────────────────────────────────────────────────────────────

void TopologyWidget::onExportImage()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出拓扑图"),
        QStringLiteral("topology.png"),
        QStringLiteral("PNG 图片 (*.png)")
    );

    if (filePath.isEmpty()) return;

    QRectF sceneRect = m_scene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QPixmap pixmap(sceneRect.size().toSize());
    pixmap.fill(QColor("#1A1A1A"));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(), sceneRect);
    painter.end();

    if (pixmap.save(filePath, "PNG")) {
        Logger::instance().info(QStringLiteral("Topology"),
            QStringLiteral("拓扑图已导出: %1").arg(filePath));
        QMessageBox::information(this, QStringLiteral("导出成功"),
            QStringLiteral("拓扑图已保存到:\n%1").arg(filePath));
    } else {
        Logger::instance().error(QStringLiteral("Topology"),
            QStringLiteral("导出失败: %1").arg(filePath));
        QMessageBox::critical(this, QStringLiteral("导出失败"),
            QStringLiteral("无法保存图片到指定路径。"));
    }
}

// ── Zoom Changed ─────────────────────────────────────────────────────────────

void TopologyWidget::onZoomChanged(int value)
{
    qreal scale = value / 100.0;
    m_view->resetTransform();
    m_view->scale(scale, scale);
}

// ── Scene Selection Changed ──────────────────────────────────────────────────

void TopologyWidget::onSceneSelectionChanged()
{
    QList<QGraphicsItem*> selected = m_scene->selectedItems();

    if (selected.isEmpty()) {
        m_deviceDetailLabel->setText(QStringLiteral("请选择一个设备节点"));
        m_linkDetailLabel->setText(QStringLiteral("请选择一条链路"));
        return;
    }

    // Check if a node (ellipse) is selected
    for (auto* item : selected) {
        auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item);
        if (ellipse) {
            QString nodeId = ellipse->data(0).toString();
            for (const auto& node : m_nodes) {
                if (node.id == nodeId) {
                    QString statusStr = node.online ? QStringLiteral("在线") : QStringLiteral("离线");
                    QString statusColor = node.online ? "#3FB950" : "#F85149";
                    QString typeMap;
                    if (node.type == QStringLiteral("switch")) typeMap = QStringLiteral("交换机");
                    else if (node.type == QStringLiteral("router")) typeMap = QStringLiteral("路由器");
                    else if (node.type == QStringLiteral("firewall")) typeMap = QStringLiteral("防火墙");
                    else if (node.type == QStringLiteral("server")) typeMap = QStringLiteral("服务器");
                    else if (node.type == QStringLiteral("ap")) typeMap = QStringLiteral("无线AP");
                    else if (node.type == QStringLiteral("isp")) typeMap = QStringLiteral("ISP");
                    else typeMap = node.type;

                    m_deviceDetailLabel->setText(
                        QStringLiteral(
                            "<div style='line-height: 1.8;'>"
                            "<b>名称:</b> %1<br>"
                            "<b>IP:</b> %2<br>"
                            "<b>类型:</b> %3<br>"
                            "<b>状态:</b> <span style='color: %4;'>%5</span><br>"
                            "<b>接口数:</b> %6"
                            "</div>"
                        )
                        .arg(node.name, node.ip, typeMap, statusColor, statusStr)
                        .arg(3) // simulated interface count
                    );
                    return;
                }
            }
        }
    }

    // Check if a line (link) is selected
    for (auto* item : selected) {
        auto* line = dynamic_cast<QGraphicsLineItem*>(item);
        if (line) {
            QString linkId = line->data(1).toString();
            for (const auto& link : m_links) {
                if (link.id == linkId) {
                    QString srcName, dstName;
                    for (const auto& node : m_nodes) {
                        if (node.id == link.sourceId) srcName = node.name;
                        if (node.id == link.targetId) dstName = node.name;
                    }

                    QString statusStr = link.up ? QStringLiteral("Up") : QStringLiteral("Down");
                    QString statusColor = link.up ? "#3FB950" : "#F85149";
                    QString bwStr;
                    if (link.bandwidth >= 1000) {
                        bwStr = QStringLiteral("%1 Gbps").arg(link.bandwidth / 1000.0, 0, 'f', 1);
                    } else {
                        bwStr = QStringLiteral("%1 Mbps").arg(link.bandwidth);
                    }

                    m_linkDetailLabel->setText(
                        QStringLiteral(
                            "<div style='line-height: 1.8;'>"
                            "<b>源设备:</b> %1 (%2)<br>"
                            "<b>目标设备:</b> %3 (%4)<br>"
                            "<b>带宽:</b> %5<br>"
                            "<b>状态:</b> <span style='color: %6;'>%7</span>"
                            "</div>"
                        )
                        .arg(srcName, link.sourcePort, dstName, link.targetPort,
                             bwStr, statusColor, statusStr)
                    );
                    return;
                }
            }
        }
    }
}

// ── Add Node ─────────────────────────────────────────────────────────────────

void TopologyWidget::addNode(const QString& name, const QString& ip, const QString& type, const QPointF& pos)
{
    TopoNode node;
    node.id = QStringLiteral("node_%1").arg(m_nodes.size());
    node.name = name;
    node.ip = ip;
    node.type = type;
    node.pos = pos;
    node.online = true;

    m_nodes.append(node);

    auto* item = createNodeItem(node);
    m_scene->addItem(item);
    m_nodeItems[node.id] = item;

    // Add to device list
    QString typeIcon;
    if (type == QStringLiteral("switch")) typeIcon = QStringLiteral("[交换机]");
    else if (type == QStringLiteral("router")) typeIcon = QStringLiteral("[路由器]");
    else if (type == QStringLiteral("firewall")) typeIcon = QStringLiteral("[防火墙]");
    else if (type == QStringLiteral("server")) typeIcon = QStringLiteral("[服务器]");
    else if (type == QStringLiteral("ap")) typeIcon = QStringLiteral("[AP]");
    else if (type == QStringLiteral("isp")) typeIcon = QStringLiteral("[ISP]");
    else typeIcon = QStringLiteral("[设备]");

    m_deviceList->addItem(QStringLiteral("%1 %2 (%3)").arg(typeIcon, name, ip));
}

// ── Add Link ─────────────────────────────────────────────────────────────────

void TopologyWidget::addLink(const QString& source, const QString& target,
                              const QString& srcPort, const QString& dstPort)
{
    // Find source and target node indices
    int srcIdx = -1, dstIdx = -1;
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].name == source) srcIdx = i;
        if (m_nodes[i].name == target) dstIdx = i;
    }

    if (srcIdx < 0 || dstIdx < 0) return;

    TopoLink link;
    link.id = QStringLiteral("link_%1").arg(m_links.size());
    link.sourceId = m_nodes[srcIdx].id;
    link.targetId = m_nodes[dstIdx].id;
    link.sourcePort = srcPort;
    link.targetPort = dstPort;
    link.bandwidth = 1000; // 1 Gbps default
    link.up = true;

    m_links.append(link);

    auto* item = createLinkItem(link);
    m_scene->addItem(item);

    // Ensure link is drawn behind nodes
    item->setZValue(-1);
}

// ── Update Node Status ───────────────────────────────────────────────────────

void TopologyWidget::updateNodeStatus(const QString& nodeId, bool online)
{
    for (auto& node : m_nodes) {
        if (node.id == nodeId) {
            node.online = online;
            auto it = m_nodeItems.find(nodeId);
            if (it != m_nodeItems.end()) {
                m_scene->removeItem(it.value());
                delete it.value();
                auto* newItem = createNodeItem(node);
                m_scene->addItem(newItem);
                m_nodeItems[nodeId] = newItem;
            }
            break;
        }
    }
}

// ── Clear Topology ───────────────────────────────────────────────────────────

void TopologyWidget::clearTopology()
{
    m_scene->clear();
    m_nodeItems.clear();
    m_nodes.clear();
    m_links.clear();
    m_deviceList->clear();
    m_deviceDetailLabel->setText(QStringLiteral("请选择一个设备节点"));
    m_linkDetailLabel->setText(QStringLiteral("请选择一条链路"));
}

// ── Load Demo Topology ───────────────────────────────────────────────────────

void TopologyWidget::loadDemoTopology()
{
    Logger::instance().info(QStringLiteral("Topology"), QStringLiteral("加载演示拓扑"));

    // Core switch
    addNode(QStringLiteral("Core-SW"), QStringLiteral("10.0.0.1"), QStringLiteral("switch"), QPointF(0, 0));

    // Firewall
    addNode(QStringLiteral("FW"), QStringLiteral("10.0.0.254"), QStringLiteral("firewall"), QPointF(-300, -200));

    // Router
    addNode(QStringLiteral("Router"), QStringLiteral("10.0.0.253"), QStringLiteral("router"), QPointF(300, -200));

    // ISP
    addNode(QStringLiteral("ISP"), QStringLiteral("203.0.113.1"), QStringLiteral("isp"), QPointF(500, -200));

    // Access switch 1
    addNode(QStringLiteral("Access-SW1"), QStringLiteral("10.0.1.1"), QStringLiteral("switch"), QPointF(-250, 200));

    // Access switch 2
    addNode(QStringLiteral("Access-SW2"), QStringLiteral("10.0.1.2"), QStringLiteral("switch"), QPointF(250, 200));

    // Wireless AP
    addNode(QStringLiteral("AP-01"), QStringLiteral("10.0.2.1"), QStringLiteral("ap"), QPointF(-400, 350));

    // Server
    addNode(QStringLiteral("Server-01"), QStringLiteral("10.0.3.1"), QStringLiteral("server"), QPointF(0, 350));

    // Links
    addLink(QStringLiteral("Core-SW"), QStringLiteral("FW"), QStringLiteral("G0/1"), QStringLiteral("G0/0"));
    addLink(QStringLiteral("Core-SW"), QStringLiteral("Router"), QStringLiteral("G0/2"), QStringLiteral("G0/0"));
    addLink(QStringLiteral("Router"), QStringLiteral("ISP"), QStringLiteral("G0/1"), QStringLiteral("G0/0"));
    addLink(QStringLiteral("Core-SW"), QStringLiteral("Access-SW1"), QStringLiteral("G0/3"), QStringLiteral("G0/0"));
    addLink(QStringLiteral("Core-SW"), QStringLiteral("Access-SW2"), QStringLiteral("G0/4"), QStringLiteral("G0/0"));
    addLink(QStringLiteral("Access-SW1"), QStringLiteral("AP-01"), QStringLiteral("G0/1"), QStringLiteral("eth0"));
    addLink(QStringLiteral("Access-SW2"), QStringLiteral("Server-01"), QStringLiteral("G0/1"), QStringLiteral("eth0"));

    // Set some devices offline
    updateNodeStatus(QStringLiteral("node_7"), false); // AP-01 offline for demo

    // Apply default layout
    applyForceDirectedLayout();

    Logger::instance().info(QStringLiteral("Topology"),
        QStringLiteral("演示拓扑加载完成，节点: %1, 链路: %2").arg(m_nodes.size()).arg(m_links.size()));
}

// ── Create Node Item ─────────────────────────────────────────────────────────

QGraphicsEllipseItem* TopologyWidget::createNodeItem(const TopoNode& node)
{
    QColor fillColor;
    QRectF rect;

    if (node.type == QStringLiteral("switch")) {
        // Switch: rectangle 60x40
        fillColor = node.online ? QColor("#58A6FF") : QColor("#484F58");
        rect = QRectF(-30, -20, 60, 40);

        auto* item = new QGraphicsEllipseItem();
        // Use a rect-based ellipse for visual differentiation
        // Actually we create a rounded rect representation via ellipse
        item->setRect(rect);
        item->setBrush(QBrush(fillColor));
        item->setPen(QPen(QColor("#30363D"), 2));
        item->setPos(node.pos);
        item->setFlags(QGraphicsItem::ItemIsSelectable);
        item->setData(0, node.id);
        item->setData(1, QStringLiteral("node"));

        // Label
        auto* text = new QGraphicsTextItem(node.name);
        QFont font;
        font.setPointSize(9);
        font.setBold(true);
        text->setFont(font);
        text->setDefaultTextColor(QColor("#E6EDF3"));
        text->setPos(node.pos.x() - text->boundingRect().width() / 2, node.pos.y() + 22);
        text->setParentItem(item);
        text->setFlags(QGraphicsItem::ItemIgnoresTransformations);

        return item;
    }

    if (node.type == QStringLiteral("router")) {
        // Router: circle radius 25
        fillColor = node.online ? QColor("#D29922") : QColor("#484F58");
        rect = QRectF(-25, -25, 50, 50);

        auto* item = new QGraphicsEllipseItem();
        item->setRect(rect);
        item->setBrush(QBrush(fillColor));
        item->setPen(QPen(QColor("#30363D"), 2));
        item->setPos(node.pos);
        item->setFlags(QGraphicsItem::ItemIsSelectable);
        item->setData(0, node.id);
        item->setData(1, QStringLiteral("node"));

        auto* text = new QGraphicsTextItem(node.name);
        QFont font;
        font.setPointSize(9);
        font.setBold(true);
        text->setFont(font);
        text->setDefaultTextColor(QColor("#E6EDF3"));
        text->setPos(node.pos.x() - text->boundingRect().width() / 2, node.pos.y() + 28);
        text->setParentItem(item);
        text->setFlags(QGraphicsItem::ItemIgnoresTransformations);

        return item;
    }

    if (node.type == QStringLiteral("firewall")) {
        // Firewall: approximated hexagon using a diamond shape
        fillColor = node.online ? QColor("#F85149") : QColor("#484F58");
        rect = QRectF(-28, -28, 56, 56);

        auto* item = new QGraphicsEllipseItem();
        item->setRect(rect);
        item->setBrush(QBrush(fillColor));
        item->setPen(QPen(QColor("#30363D"), 2));
        item->setPos(node.pos);
        item->setFlags(QGraphicsItem::ItemIsSelectable);
        item->setData(0, node.id);
        item->setData(1, QStringLiteral("node"));

        auto* text = new QGraphicsTextItem(node.name);
        QFont font;
        font.setPointSize(9);
        font.setBold(true);
        text->setFont(font);
        text->setDefaultTextColor(QColor("#E6EDF3"));
        text->setPos(node.pos.x() - text->boundingRect().width() / 2, node.pos.y() + 30);
        text->setParentItem(item);
        text->setFlags(QGraphicsItem::ItemIgnoresTransformations);

        return item;
    }

    if (node.type == QStringLiteral("server")) {
        // Server: rectangle 50x50
        fillColor = node.online ? QColor("#3FB950") : QColor("#484F58");
        rect = QRectF(-25, -25, 50, 50);

        auto* item = new QGraphicsEllipseItem();
        item->setRect(rect);
        item->setBrush(QBrush(fillColor));
        item->setPen(QPen(QColor("#30363D"), 2));
        item->setPos(node.pos);
        item->setFlags(QGraphicsItem::ItemIsSelectable);
        item->setData(0, node.id);
        item->setData(1, QStringLiteral("node"));

        auto* text = new QGraphicsTextItem(node.name);
        QFont font;
        font.setPointSize(9);
        font.setBold(true);
        text->setFont(font);
        text->setDefaultTextColor(QColor("#E6EDF3"));
        text->setPos(node.pos.x() - text->boundingRect().width() / 2, node.pos.y() + 28);
        text->setParentItem(item);
        text->setFlags(QGraphicsItem::ItemIgnoresTransformations);

        return item;
    }

    // AP, ISP, default: circle
    if (node.type == QStringLiteral("ap")) {
        fillColor = node.online ? QColor("#9B59B6") : QColor("#484F58");
    } else if (node.type == QStringLiteral("isp")) {
        fillColor = node.online ? QColor("#1ABC9C") : QColor("#484F58");
    } else {
        fillColor = node.online ? QColor("#8B949E") : QColor("#484F58");
    }

    rect = QRectF(-22, -22, 44, 44);

    auto* item = new QGraphicsEllipseItem();
    item->setRect(rect);
    item->setBrush(QBrush(fillColor));
    item->setPen(QPen(QColor("#30363D"), 2));
    item->setPos(node.pos);
    item->setFlags(QGraphicsItem::ItemIsSelectable);
    item->setData(0, node.id);
    item->setData(1, QStringLiteral("node"));

    auto* text = new QGraphicsTextItem(node.name);
    QFont font;
    font.setPointSize(9);
    font.setBold(true);
    text->setFont(font);
    text->setDefaultTextColor(QColor("#E6EDF3"));
    text->setPos(node.pos.x() - text->boundingRect().width() / 2, node.pos.y() + 25);
    text->setParentItem(item);
    text->setFlags(QGraphicsItem::ItemIgnoresTransformations);

    return item;
}

// ── Create Link Item ─────────────────────────────────────────────────────────

QGraphicsLineItem* TopologyWidget::createLinkItem(const TopoLink& link)
{
    QPointF srcPos, dstPos;
    for (const auto& node : m_nodes) {
        if (node.id == link.sourceId) srcPos = node.pos;
        if (node.id == link.targetId) dstPos = node.pos;
    }

    auto* item = new QGraphicsLineItem();
    item->setLine(QLineF(srcPos, dstPos));

    if (link.up) {
        item->setPen(QPen(QColor("#3FB950"), 2, Qt::SolidLine));
    } else {
        QPen pen(QColor("#F85149"), 2, Qt::DashLine);
        pen.setDashPattern({4, 4});
        item->setPen(pen);
    }

    item->setFlags(QGraphicsItem::ItemIsSelectable);
    item->setData(0, link.id);
    item->setData(1, QStringLiteral("link"));

    return item;
}

// ── Force-Directed Layout ────────────────────────────────────────────────────

void TopologyWidget::applyForceDirectedLayout()
{
    if (m_nodes.isEmpty()) return;

    const int iterations = 100;
    const qreal repulsion = 5000.0;
    const qreal attraction = 0.01;
    const qreal damping = 0.9;

    QMap<QString, QPointF> velocities;
    for (const auto& node : m_nodes) {
        velocities[node.id] = QPointF(0, 0);
    }

    for (int iter = 0; iter < iterations; ++iter) {
        // Calculate forces
        QMap<QString, QPointF> forces;
        for (const auto& node : m_nodes) {
            forces[node.id] = QPointF(0, 0);
        }

        // Repulsion between all node pairs
        for (int i = 0; i < m_nodes.size(); ++i) {
            for (int j = i + 1; j < m_nodes.size(); ++j) {
                QPointF delta = m_nodes[i].pos - m_nodes[j].pos;
                qreal dist = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
                if (dist < 1.0) dist = 1.0;

                qreal force = repulsion / (dist * dist);
                QPointF dir = delta / dist;
                forces[m_nodes[i].id] += dir * force;
                forces[m_nodes[j].id] -= dir * force;
            }
        }

        // Attraction along links
        for (const auto& link : m_links) {
            QPointF delta = m_nodes[indexOf(link.sourceId)].pos - m_nodes[indexOf(link.targetId)].pos;
            qreal dist = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
            if (dist < 1.0) dist = 1.0;

            QPointF dir = delta / dist;
            QPointF force = dir * attraction * dist;
            forces[link.sourceId] -= force;
            forces[link.targetId] += force;
        }

        // Update velocities and positions
        for (auto& node : m_nodes) {
            velocities[node.id] = (velocities[node.id] + forces[node.id]) * damping;
            node.pos += velocities[node.id];
        }
    }

    // Update scene items
    for (auto& node : m_nodes) {
        auto it = m_nodeItems.find(node.id);
        if (it != m_nodeItems.end()) {
            it.value()->setPos(node.pos);
            // Update child text items
            for (auto* child : it.value()->childItems()) {
                auto* textItem = dynamic_cast<QGraphicsTextItem*>(child);
                if (textItem) {
                    textItem->setPos(node.pos.x() - textItem->boundingRect().width() / 2,
                                     node.pos.y() + 25);
                }
            }
        }
    }

    // Update link lines
    for (const auto& link : m_links) {
        auto items = m_scene->items();
        for (auto* item : items) {
            auto* line = dynamic_cast<QGraphicsLineItem*>(item);
            if (line && line->data(0).toString() == link.id) {
                QPointF srcPos, dstPos;
                for (const auto& node : m_nodes) {
                    if (node.id == link.sourceId) srcPos = node.pos;
                    if (node.id == link.targetId) dstPos = node.pos;
                }
                line->setLine(QLineF(srcPos, dstPos));
                break;
            }
        }
    }
}

// ── Tree Layout ──────────────────────────────────────────────────────────────

void TopologyWidget::applyTreeLayout()
{
    if (m_nodes.isEmpty()) return;

    // Find root: node with id "node_0" (Core-SW) or first node
    QString rootId = m_nodes.isEmpty() ? QString() : m_nodes[0].id;

    // Build adjacency list
    QMap<QString, QList<QString>> children;
    QSet<QString> hasParent;
    for (const auto& link : m_links) {
        children[link.sourceId].append(link.targetId);
        hasParent.insert(link.targetId);
    }

    // Find root (node with no parent)
    for (const auto& node : m_nodes) {
        if (!hasParent.contains(node.id)) {
            rootId = node.id;
            break;
        }
    }

    const qreal levelSpacing = 150.0;
    const qreal nodeSpacing = 120.0;

    // BFS to assign positions
    struct LayoutInfo {
        QString id;
        int level;
        int index;
    };

    QList<LayoutInfo> layout;
    QMap<int, QList<QString>> levels;
    QQueue<QPair<QString, int>> queue;
    QSet<QString> visited;

    queue.enqueue({rootId, 0});
    visited.insert(rootId);

    while (!queue.isEmpty()) {
        auto [currentId, level] = queue.dequeue();
        levels[level].append(currentId);

        for (const auto& childId : children[currentId]) {
            if (!visited.contains(childId)) {
                visited.insert(childId);
                queue.enqueue({childId, level + 1});
            }
        }
    }

    // Assign positions
    for (auto it = levels.begin(); it != levels.end(); ++it) {
        int level = it.key();
        const auto& nodeIds = it.value();
        int count = nodeIds.size();
        qreal startX = -(count - 1) * nodeSpacing / 2.0;

        for (int i = 0; i < count; ++i) {
            for (auto& node : m_nodes) {
                if (node.id == nodeIds[i]) {
                    node.pos = QPointF(startX + i * nodeSpacing, level * levelSpacing);
                    break;
                }
            }
        }
    }

    // Update scene items
    updateSceneFromData();
}

// ── Circular Layout ──────────────────────────────────────────────────────────

void TopologyWidget::applyCircularLayout()
{
    if (m_nodes.isEmpty()) return;

    int count = m_nodes.size();
    qreal radius = count * 30.0;
    if (radius < 150.0) radius = 150.0;

    for (int i = 0; i < count; ++i) {
        qreal angle = 2.0 * M_PI * i / count - M_PI / 2.0;
        m_nodes[i].pos = QPointF(radius * qCos(angle), radius * qSin(angle));
    }

    updateSceneFromData();
}

// ── Update Scene From Data ───────────────────────────────────────────────────

void TopologyWidget::updateSceneFromData()
{
    // Update node positions
    for (auto& node : m_nodes) {
        auto it = m_nodeItems.find(node.id);
        if (it != m_nodeItems.end()) {
            it.value()->setPos(node.pos);
            for (auto* child : it.value()->childItems()) {
                auto* textItem = dynamic_cast<QGraphicsTextItem*>(child);
                if (textItem) {
                    textItem->setPos(node.pos.x() - textItem->boundingRect().width() / 2,
                                     node.pos.y() + 25);
                }
            }
        }
    }

    // Update link lines
    QList<QGraphicsItem*> sceneItems = m_scene->items();
    for (const auto& link : m_links) {
        for (auto* item : sceneItems) {
            auto* line = dynamic_cast<QGraphicsLineItem*>(item);
            if (line && line->data(0).toString() == link.id) {
                QPointF srcPos, dstPos;
                for (const auto& node : m_nodes) {
                    if (node.id == link.sourceId) srcPos = node.pos;
                    if (node.id == link.targetId) dstPos = node.pos;
                }
                line->setLine(QLineF(srcPos, dstPos));
                break;
            }
        }
    }
}

// ── indexOf ──────────────────────────────────────────────────────────────────

int TopologyWidget::indexOf(const QString& nodeId) const
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == nodeId) return i;
    }
    return -1;
}