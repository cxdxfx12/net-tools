#include "terminal/TerminalRenderer.h"
#include "terminal/TerminalBuffer.h"
#include "terminal/TerminalCursor.h"
#include "terminal/TerminalColor.h"

TerminalRenderer::TerminalRenderer(TerminalBuffer* buffer, TerminalCursor* cursor,
                                   TerminalColor* color, QObject* parent)
    : QObject(parent)
    , m_buffer(buffer)
    , m_cursor(cursor)
    , m_color(color)
    , m_fontMetrics(m_font)
{
    setFont(QFont("Consolas, Courier New, monospace", 14));
}

void TerminalRenderer::setFont(const QFont& font)
{
    m_font = font;
    m_fontMetrics = QFontMetrics(m_font);
    m_cellSize = QSize(m_fontMetrics.horizontalAdvance('W'),
                       m_fontMetrics.height());
}

int TerminalRenderer::columnsForWidth(int width) const
{
    return std::max(1, width / m_cellSize.width());
}

int TerminalRenderer::rowsForHeight(int height) const
{
    return std::max(1, height / m_cellSize.height());
}

void TerminalRenderer::render(QPainter& painter, const QRect& rect, int scrollOffset)
{
    painter.save();
    painter.setFont(m_font);

    int visibleRows = rect.height() / m_cellSize.height();
    int visibleCols = rect.width() / m_cellSize.width();

    // Draw background
    painter.fillRect(rect, m_color->defaultBg());

    for (int row = 0; row < visibleRows; ++row) {
        for (int col = 0; col < visibleCols; ++col) {
            // Calculate buffer row considering scroll offset
            int bufferRow = row + scrollOffset;
            int historyRows = m_buffer->historySize();

            QRect cellRect(rect.x() + col * m_cellSize.width(),
                          rect.y() + row * m_cellSize.height(),
                          m_cellSize.width(), m_cellSize.height());

            if (bufferRow < historyRows) {
                // Draw from history
                const auto& line = m_buffer->historyLine(bufferRow);
                if (col < line.size()) {
                    drawCell(painter, col, row, cellRect, bufferRow);
                }
            } else {
                bufferRow -= historyRows;
                if (bufferRow < m_buffer->rows()) {
                    drawCell(painter, col, row, cellRect, bufferRow);
                }
            }
        }
    }

    // Draw cursor
    if (m_cursor->visible() && m_cursor->blinkOn() && scrollOffset == m_buffer->maxScrollOffset()) {
        int cursorRow = m_cursor->row();
        int cursorCol = m_cursor->col();

        if (cursorRow < visibleRows && cursorCol < visibleCols) {
            QRect cursorRect(rect.x() + cursorCol * m_cellSize.width(),
                            rect.y() + cursorRow * m_cellSize.height(),
                            m_cellSize.width(), m_cellSize.height());

            painter.setPen(Qt::NoPen);
            painter.setBrush(m_color->defaultFg());
            painter.drawRect(cursorRect);
        }
    }

    painter.restore();
}

void TerminalRenderer::drawCell(QPainter& painter, int col, int row,
                                 const QRect& cellRect, int bufferRow)
{
    const auto& cell = m_buffer->cellAt(col, bufferRow);

    // Background
    QColor bg = effectiveBg(cell.bg);
    painter.fillRect(cellRect, bg);

    // Character
    if (cell.ch != QChar(' ') && cell.ch != QChar('\0')) {
        QColor fg = effectiveFg(cell.fg, cell.bold);
        painter.setPen(fg);

        if (m_font.bold() || cell.bold) {
            QFont boldFont = m_font;
            boldFont.setBold(true);
            painter.setFont(boldFont);
        }

        QString text(cell.ch);
        painter.drawText(cellRect, Qt::AlignCenter, text);

        if (cell.bold && !m_font.bold()) {
            painter.setFont(m_font);
        }
    }

    // Underline
    if (cell.underline) {
        painter.setPen(cell.fg);
        painter.drawLine(cellRect.bottomLeft(), cellRect.bottomRight());
    }
}

QColor TerminalRenderer::effectiveFg(const QColor& fg, bool bold) const
{
    if (bold) {
        return fg.lighter(130);
    }
    return fg;
}

QColor TerminalRenderer::effectiveBg(const QColor& bg) const
{
    return bg;
}