#pragma once

#include <QObject>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

class TerminalBuffer;
class TerminalCursor;
class TerminalColor;

class TerminalRenderer : public QObject
{
    Q_OBJECT

public:
    explicit TerminalRenderer(TerminalBuffer* buffer, TerminalCursor* cursor,
                              TerminalColor* color, QObject* parent = nullptr);

    void render(QPainter& painter, const QRect& rect, int scrollOffset);
    QSize cellSize() const { return m_cellSize; }
    void setFont(const QFont& font);

    int columnsForWidth(int width) const;
    int rowsForHeight(int height) const;

private:
    void drawCell(QPainter& painter, int col, int row, const QRect& cellRect, int bufferRow);
    QColor effectiveFg(const QColor& fg, bool bold) const;
    QColor effectiveBg(const QColor& bg) const;

    TerminalBuffer* m_buffer;
    TerminalCursor* m_cursor;
    TerminalColor* m_color;

    QFont m_font;
    QFontMetrics m_fontMetrics;
    QSize m_cellSize;
};