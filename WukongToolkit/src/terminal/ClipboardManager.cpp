#include "terminal/ClipboardManager.h"
#include <QApplication>
#include <QClipboard>

ClipboardManager::ClipboardManager(QObject* parent)
    : QObject(parent)
{
}

void ClipboardManager::copy(const QString& text)
{
    QApplication::clipboard()->setText(text);
}

void ClipboardManager::paste()
{
    QString text = QApplication::clipboard()->text();
    if (!text.isEmpty()) {
        emit pasteReady(text);
    }
}

QString ClipboardManager::clipboardText() const
{
    return QApplication::clipboard()->text();
}