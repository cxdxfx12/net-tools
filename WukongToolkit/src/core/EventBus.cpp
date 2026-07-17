#include "core/EventBus.h"
#include "log/Logger.h"

#include <QMetaMethod>
#include <QMutexLocker>

EventBus::EventBus(QObject* parent)
    : QObject(parent)
{
    Logger::instance().info(QStringLiteral("EVENTBUS"), QStringLiteral("EventBus initialized"));
}

void EventBus::subscribe(const QString& eventType, QObject* receiver, const QString& methodName)
{
    if (!receiver || methodName.isEmpty()) {
        Logger::instance().warning(QStringLiteral("EVENTBUS"),
            QStringLiteral("Invalid subscribe: receiver=%1, method=%2")
                .arg(receiver ? QStringLiteral("valid") : QStringLiteral("null"), methodName));
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_subscribers[eventType].append(qMakePair(receiver, methodName));

    Logger::instance().debug(QStringLiteral("EVENTBUS"),
        QStringLiteral("Subscribed: event=%1, receiver=%2, method=%3")
            .arg(eventType, receiver->objectName(), methodName));
}

void EventBus::publish(const QString& eventType, const QVariantMap& data)
{
    Logger::instance().debug(QStringLiteral("EVENTBUS"),
        QStringLiteral("Publishing event: %1").arg(eventType));

    emit eventPublished(eventType, data);

    QMutexLocker locker(&m_mutex);

    if (!m_subscribers.contains(eventType)) {
        return;
    }

    const auto& subscribers = m_subscribers[eventType];
    for (const auto& sub : subscribers) {
        QObject* receiver = sub.first;
        const QString& methodName = sub.second;

        if (!receiver) {
            continue;
        }

        QMetaObject::invokeMethod(receiver, methodName.toLatin1().constData(),
            Qt::AutoConnection,
            Q_ARG(QVariantMap, data));
    }
}

void EventBus::unsubscribe(const QString& eventType, QObject* receiver)
{
    QMutexLocker locker(&m_mutex);

    if (!m_subscribers.contains(eventType)) {
        return;
    }

    auto& subscribers = m_subscribers[eventType];
    for (int i = subscribers.size() - 1; i >= 0; --i) {
        if (subscribers[i].first == receiver) {
            subscribers.removeAt(i);
        }
    }

    Logger::instance().debug(QStringLiteral("EVENTBUS"),
        QStringLiteral("Unsubscribed from event: %1").arg(eventType));

    if (subscribers.isEmpty()) {
        m_subscribers.remove(eventType);
    }
}