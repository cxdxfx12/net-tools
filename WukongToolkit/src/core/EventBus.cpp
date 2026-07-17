#include "core/EventBus.h"
#include "log/Logger.h"

#include <QMetaMethod>

EventBus::EventBus(QObject* parent)
    : QObject(parent)
{
    Logger::instance().info(QStringLiteral("EVENTBUS"), QStringLiteral("EventBus initialized"));
}

void EventBus::publish(const QString& eventType, const QVariantMap& data)
{
    Logger::instance().debug(QStringLiteral("EVENTBUS"),
        QStringLiteral("Publishing event: %1").arg(eventType));

    emit eventPublished(eventType, data);

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