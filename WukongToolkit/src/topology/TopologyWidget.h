#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include <QGroupBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QList>
#include <QMap>
#include <QPointF>

// ============================================================================
// TopologyWidget — 网络拓扑中心
// ============================================================================

struct TopoNode
{
    QString id;
    QString name;
    QString ip;
    QString type; // switch, router, firewall, server, ap, isp
    QPointF pos;
    bool online;
};

struct TopoLink
{
    QString id;
    QString sourceId;
    QString targetId;
    QString sourcePort;
    QString targetPort;
    qint64 bandwidth; // Mbps
    bool up;
};

class TopologyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TopologyWidget(QWidget* parent = nullptr);
    ~TopologyWidget() override;

private:
    // ── UI Setup ──
    void setupUI();
    void setupConnections();

    // ── Actions ──
    void onAutoLayout();
    void onRefresh();
    void onSearchDevice();
    void onExportImage();
    void onZoomChanged(int value);
    void onSceneSelectionChanged();

    // ── Topology Operations ──
    void addNode(const QString& name, const QString& ip, const QString& type, const QPointF& pos);
    void addLink(const QString& source, const QString& target, const QString& srcPort, const QString& dstPort);
    void updateNodeStatus(const QString& nodeId, bool online);
    void clearTopology();
    void loadDemoTopology();

    // ── Graphics Item Creation ──
    QGraphicsEllipseItem* createNodeItem(const TopoNode& node);
    QGraphicsLineItem* createLinkItem(const TopoLink& link);

    // ── Layout Algorithms ──
    void applyForceDirectedLayout();
    void applyTreeLayout();
    void applyCircularLayout();
    void updateSceneFromData();

    // ── Helpers ──
    int indexOf(const QString& nodeId) const;

    // ── Scene ──
    QGraphicsScene* m_scene;

    // ── View ──
    QGraphicsView* m_view;

    // ── Data ──
    QList<TopoNode> m_nodes;
    QList<TopoLink> m_links;
    QMap<QString, QGraphicsEllipseItem*> m_nodeItems;

    // ── Toolbar ──
    QPushButton* m_autoLayoutBtn;
    QPushButton* m_refreshBtn;
    QLineEdit*   m_searchEdit;
    QPushButton* m_exportImageBtn;
    QSlider*     m_zoomSlider;

    // ── Right Panel ──
    QListWidget* m_deviceList;
    QGroupBox*   m_deviceDetailGroup;
    QLabel*      m_deviceDetailLabel;
    QGroupBox*   m_linkDetailGroup;
    QLabel*      m_linkDetailLabel;

    // ── Layout State ──
    int m_layoutMode; // 0=force-directed, 1=tree, 2=circular
};