#include "terminal/SessionManager.h"
#include <QHeaderView>
#include <QHBoxLayout>

SessionManager::SessionManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SessionManager::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    m_newSessionBtn = new QPushButton("+ 新建会话");
    m_newSessionBtn->setStyleSheet(
        "QPushButton { background-color: #58A6FF; color: #FFF; border-radius: 4px; "
        "padding: 4px 12px; min-height: 28px; font-size: 12px; }"
        "QPushButton:hover { background-color: #79C0FF; }"
    );
    m_closeAllBtn = new QPushButton("关闭全部");
    m_closeAllBtn->setStyleSheet(
        "QPushButton { background-color: #4A4D52; color: #E6EDF3; border-radius: 4px; "
        "padding: 4px 12px; min-height: 28px; font-size: 12px; }"
        "QPushButton:hover { background-color: #F85149; }"
    );

    toolbar->addWidget(m_newSessionBtn);
    toolbar->addWidget(m_closeAllBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // Session tree
    m_sessionTree = new QTreeWidget();
    m_sessionTree->setHeaderLabels({"会话", "状态"});
    m_sessionTree->setColumnWidth(0, 160);
    m_sessionTree->setColumnWidth(1, 60);
    m_sessionTree->setRootIsDecorated(false);
    m_sessionTree->setAlternatingRowColors(false);
    m_sessionTree->setStyleSheet(
        "QTreeWidget { background-color: #0D1117; border: 1px solid #30363D; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:selected { background-color: #30323A; }"
        "QHeaderView::section { background-color: #161B22; color: #8B949E; "
        "padding: 4px; border: none; border-bottom: 1px solid #30363D; font-size: 11px; }"
    );
    layout->addWidget(m_sessionTree);

    // Connections
    connect(m_newSessionBtn, &QPushButton::clicked, this, &SessionManager::newSessionRequested);
    connect(m_closeAllBtn, &QPushButton::clicked, this, [this]() {
        // Request close all sessions
        for (int i = 0; i < m_sessionTree->topLevelItemCount(); ++i) {
            auto* item = m_sessionTree->topLevelItem(i);
            emit sessionCloseRequested(item->data(0, Qt::UserRole).toString());
        }
    });

    connect(m_sessionTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        emit sessionSelected(item->data(0, Qt::UserRole).toString());
    });
}

void SessionManager::addSession(const QString& id, const QString& host, const QString& protocol)
{
    auto* item = new QTreeWidgetItem(m_sessionTree);
    item->setText(0, QString("%1 (%2)").arg(host, protocol));
    item->setText(1, "连接中");
    item->setData(0, Qt::UserRole, id);
    item->setForeground(1, QColor("#D29922"));
}

void SessionManager::removeSession(const QString& id)
{
    for (int i = 0; i < m_sessionTree->topLevelItemCount(); ++i) {
        auto* item = m_sessionTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == id) {
            delete m_sessionTree->takeTopLevelItem(i);
            return;
        }
    }
}

void SessionManager::updateSessionState(const QString& id, const QString& state)
{
    for (int i = 0; i < m_sessionTree->topLevelItemCount(); ++i) {
        auto* item = m_sessionTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == id) {
            item->setText(1, state);
            if (state == "已连接") {
                item->setForeground(1, QColor("#3FB950"));
            } else if (state == "已断开") {
                item->setForeground(1, QColor("#F85149"));
            } else if (state == "错误") {
                item->setForeground(1, QColor("#F85149"));
            } else {
                item->setForeground(1, QColor("#D29922"));
            }
            return;
        }
    }
}

void SessionManager::clear()
{
    m_sessionTree->clear();
}