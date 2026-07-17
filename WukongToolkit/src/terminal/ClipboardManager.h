#pragma once

#include <QObject>

class ClipboardManager : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardManager(QObject* parent = nullptr);

    void copy(const QString& text);
    void paste();
    QString clipboardText() const;

signals:
    void pasteReady(const QString& text);
};