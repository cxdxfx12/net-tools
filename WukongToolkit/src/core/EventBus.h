#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QPair>
#include <QMutex>

class EventBus : public QObject
{
    Q_OBJECT

public:
    explicit EventBus(QObject* parent = nullptr);

    // 使用 Q_INVOKABLE 方法名订阅（类型安全）
    // 订阅者需要提供一个标记为 Q_INVOKABLE 或 slot 的方法：
    //   Q_INVOKABLE void onSomeEvent(const QVariantMap& data);
    void subscribe(const QString& eventType, QObject* receiver, const QString& methodName);

    void publish(const QString& eventType, const QVariantMap& data);
    void unsubscribe(const QString& eventType, QObject* receiver);

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    QMutex m_mutex;
    QMap<QString, QList<QPair<QObject*, QString>>> m_subscribers;
};

