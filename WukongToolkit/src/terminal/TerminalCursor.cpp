#include "terminal/TerminalCursor.h"
#include <algorithm>

TerminalCursor::TerminalCursor(QObject* parent)
    : QObject(parent)
    , m_blinkTimer(new QTimer(this))
{
    m_blinkTimer->setInterval(500);
    connect(m_blinkTimer, &QTimer::timeout, this, &TerminalCursor::toggleBlink);
    m_blinkTimer->start();
}

void TerminalCursor::setPos(int col, int row)
{
    m_col = std::max(0, col);
    m_row = std::max(0, row);
}

void TerminalCursor::move(int deltaCol, int deltaRow)
{
    m_col = std::max(0, m_col + deltaCol);
    m_row = std::max(0, m_row + deltaRow);
}

void TerminalCursor::setVisible(bool visible)
{
    m_visible = visible;
}

void TerminalCursor::setBlinkEnabled(bool enabled)
{
    m_blinkEnabled = enabled;
    if (!enabled) {
        m_blinkOn = true;
    }
}

void TerminalCursor::save()
{
    m_savedCol = m_col;
    m_savedRow = m_row;
}

void TerminalCursor::restore()
{
    m_col = m_savedCol;
    m_row = m_savedRow;
}

void TerminalCursor::toggleBlink()
{
    if (m_blinkEnabled) {
        m_blinkOn = !m_blinkOn;
    }
}