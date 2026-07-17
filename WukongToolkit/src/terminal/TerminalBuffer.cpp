#include "terminal/TerminalBuffer.h"
#include <algorithm>

TerminalBuffer::TerminalBuffer(int cols, int rows)
    : m_cols(cols)
    , m_rows(rows)
{
    m_cells.resize(rows);
    for (auto& row : m_cells) {
        row.resize(cols);
    }
}

void TerminalBuffer::resize(int cols, int rows)
{
    m_cols = cols;
    m_rows = rows;
    m_cells.resize(rows);
    for (auto& row : m_cells) {
        row.resize(cols);
    }
    ensureBounds(m_cursorCol, m_cursorRow);
}

void TerminalBuffer::clear()
{
    for (auto& row : m_cells) {
        for (auto& cell : row) {
            cell.reset();
        }
    }
    m_cursorCol = 0;
    m_cursorRow = 0;
    m_scrollOffset = 0;
}

TerminalCell& TerminalBuffer::cellAt(int col, int row)
{
    ensureBounds(col, row);
    return m_cells[row][col];
}

const TerminalCell& TerminalBuffer::cellAt(int col, int row) const
{
    ensureBounds(col, row);
    return m_cells[row][col];
}

void TerminalBuffer::setCell(int col, int row, QChar ch)
{
    ensureBounds(col, row);
    m_cells[row][col].ch = ch;
}

void TerminalBuffer::setCell(int col, int row, const TerminalCell& cell)
{
    ensureBounds(col, row);
    m_cells[row][col] = cell;
}

void TerminalBuffer::insertLine(int row)
{
    if (row < 0 || row >= m_rows) return;

    // Save the bottom line to history
    addToHistory(m_cells.last());

    // Shift lines down
    for (int r = m_rows - 1; r > row; --r) {
        m_cells[r] = m_cells[r - 1];
    }
    // Clear the inserted line
    for (auto& cell : m_cells[row]) {
        cell.reset();
    }
}

void TerminalBuffer::deleteLine(int row)
{
    if (row < 0 || row >= m_rows) return;

    for (int r = row; r < m_rows - 1; ++r) {
        m_cells[r] = m_cells[r + 1];
    }
    for (auto& cell : m_cells.last()) {
        cell.reset();
    }
}

void TerminalBuffer::scrollUp(int lines)
{
    lines = std::min(lines, m_rows);
    for (int i = 0; i < lines; ++i) {
        addToHistory(m_cells.first());
        m_cells.removeFirst();
        QVector<TerminalCell> newLine(m_cols);
        m_cells.append(newLine);
    }
}

void TerminalBuffer::scrollDown(int lines)
{
    lines = std::min(lines, m_rows);
    for (int i = 0; i < lines; ++i) {
        m_cells.removeLast();
        QVector<TerminalCell> newLine(m_cols);
        m_cells.prepend(newLine);
    }
}

void TerminalBuffer::eraseLine(int row)
{
    if (row < 0 || row >= m_rows) return;
    for (auto& cell : m_cells[row]) {
        cell.reset();
    }
}

void TerminalBuffer::eraseFromCursor(int col, int row)
{
    ensureBounds(col, row);
    for (int c = col; c < m_cols; ++c) {
        m_cells[row][c].reset();
    }
    for (int r = row + 1; r < m_rows; ++r) {
        for (auto& cell : m_cells[r]) {
            cell.reset();
        }
    }
}

void TerminalBuffer::eraseToCursor(int col, int row)
{
    ensureBounds(col, row);
    for (int r = 0; r < row; ++r) {
        for (auto& cell : m_cells[r]) {
            cell.reset();
        }
    }
    for (int c = 0; c <= col; ++c) {
        m_cells[row][c].reset();
    }
}

void TerminalBuffer::eraseBelow(int col, int row)
{
    eraseFromCursor(col, row);
}

void TerminalBuffer::eraseAbove(int col, int row)
{
    eraseToCursor(col, row);
}

void TerminalBuffer::addToHistory(const QVector<TerminalCell>& line)
{
    m_history.append(line);
    if (m_history.size() > m_maxHistory) {
        m_history.removeFirst();
    }
}

const QVector<TerminalCell>& TerminalBuffer::historyLine(int index) const
{
    return m_history[index];
}

void TerminalBuffer::setScrollOffset(int offset)
{
    m_scrollOffset = std::clamp(offset, 0, maxScrollOffset());
}

int TerminalBuffer::maxScrollOffset() const
{
    return m_history.size();
}

void TerminalBuffer::setCursorPos(int col, int row)
{
    m_cursorCol = std::clamp(col, 0, m_cols - 1);
    m_cursorRow = std::clamp(row, 0, m_rows - 1);
}

void TerminalBuffer::moveCursor(int deltaCol, int deltaRow)
{
    setCursorPos(m_cursorCol + deltaCol, m_cursorRow + deltaRow);
}

void TerminalBuffer::ensureBounds(int& col, int& row) const
{
    col = std::clamp(col, 0, m_cols - 1);
    row = std::clamp(row, 0, m_rows - 1);
}