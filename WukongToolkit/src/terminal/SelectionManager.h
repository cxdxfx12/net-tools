#pragma once

#include <QObject>
#include <QPoint>

class TerminalBuffer;

class SelectionManager : public QObject
{
    Q_OBJECT

public:
    explicit SelectionManager(QObject* parent = nullptr);

    void setBuffer(TerminalBuffer* buffer) { m_buffer = buffer; }

    void startSelection(int col, int row);
    void updateSelection(int col, int row);
    void endSelection();
    void clearSelection();

    bool hasSelection() const { return m_hasSelection; }
    QPoint startPos() const { return m_startPos; }
    QPoint endPos() const { return m_endPos; }

    QString selectedText() const;
    bool isSelected(int col, int row) const;

signals:
    void selectionChanged();

private:
    TerminalBuffer* m_buffer = nullptr;
    bool m_hasSelection = false;
    QPoint m_startPos;
    QPoint m_endPos;
};