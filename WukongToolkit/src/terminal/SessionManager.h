#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>

class SessionManager : public QWidget
{
    Q_OBJECT

public:
    explicit SessionManager(QWidget* parent = nullptr);

    void addSession(const QString& id, const QString& host, const QString& protocol);
    void removeSession(const QString& id);
    void updateSessionState(const QString& id, const QString& state);
    void clear();

signals:
    void sessionSelected(const QString& sessionId);
    void sessionCloseRequested(const QString& sessionId);
    void newSessionRequested();

private:
    void setupUI();

    QTreeWidget* m_sessionTree;
    QPushButton* m_newSessionBtn;
    QPushButton* m_closeAllBtn;
};