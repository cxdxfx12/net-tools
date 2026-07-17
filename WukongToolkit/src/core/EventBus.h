#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QPair>
#include <QMetaMethod>

class EventBus : public QObject
{
    Q_OBJECT

public:
    explicit EventBus(QObject* parent = nullptr);

    template<typename T>
    void subscribe(const QString& eventType, QObject* receiver, void (T::*method)(const QVariantMap&));

    void publish(const QString& eventType, const QVariantMap& data);
    void unsubscribe(const QString& eventType, QObject* receiver);

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    QMap<QString, QList<QPair<QObject*, QString>>> m_subscribers;
};

template<typename T>
void EventBus::subscribe(const QString& eventType, QObject* receiver, void (T::*method)(const QVariantMap&))
{
    if (!receiver) {
        return;
    }

    const char* signal = QMetaMethod::fromSignal(method).methodSignature().constData();
    QString methodName = QString::fromLatin1(signal);
    // Remove parameter types to get the method name only
    int parenIdx = methodName.indexOf(QLatin1Char('('));
    if (parenIdx != -1) {
        methodName = methodName.left(parenIdx);
    }

    m_subscribers[eventType].append(qMakePair(receiver, methodName));
}

