#pragma once

#include "TerminalCell.h"
#include <QVector>
#include <QPoint>
#include <QMutex>

class TerminalBuffer
{
public:
    TerminalBuffer(int cols = 80, int rows = 24);

    void resize(int cols, int rows);
    void clear();

    int columns() const { return m_cols; }
    int rows() const { return m_rows; }

    TerminalCell& cellAt(int col, int row);
    const TerminalCell& cellAt(int col, int row) const;

    void setCell(int col, int row, QChar ch);
    void setCell(int col, int row, const TerminalCell& cell);

    void insertLine(int row);
    void deleteLine(int row);
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);

    void eraseLine(int row);
    void eraseFromCursor(int col, int row);
    void eraseToCursor(int col, int row);
    void eraseBelow(int col, int row);
    void eraseAbove(int col, int row);

    // Scrollback buffer
    void addToHistory(const QVector<TerminalCell>& line);
    int historySize() const { return m_history.size(); }
    const QVector<TerminalCell>& historyLine(int index) const;
    void setMaxHistory(int max) { m_maxHistory = max; }
    int maxHistory() const { return m_maxHistory; }

    // Viewport offset for scrolling
    int scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(int offset);
    int maxScrollOffset() const;

    // Cursor
    int cursorCol() const { return m_cursorCol; }
    int cursorRow() const { return m_cursorRow; }
    void setCursorPos(int col, int row);
    void moveCursor(int deltaCol, int deltaRow);
    QPoint cursorPos() const { return QPoint(m_cursorCol, m_cursorRow); }

private:
    void ensureBounds(int& col, int& row) const;

    int m_cols;
    int m_rows;
    QVector<QVector<TerminalCell>> m_cells;
    QVector<QVector<TerminalCell>> m_history;
    int m_maxHistory = 10000;
    int m_scrollOffset = 0;

    int m_cursorCol = 0;
    int m_cursorRow = 0;
};