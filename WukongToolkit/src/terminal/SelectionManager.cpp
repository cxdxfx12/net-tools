#include "terminal/SelectionManager.h"
#include "terminal/TerminalBuffer.h"
#include <algorithm>

SelectionManager::SelectionManager(QObject* parent)
    : QObject(parent)
{
}

void SelectionManager::startSelection(int col, int row)
{
    m_startPos = QPoint(col, row);
    m_endPos = m_startPos;
    m_hasSelection = true;
    emit selectionChanged();
}

void SelectionManager::updateSelection(int col, int row)
{
    if (!m_hasSelection) return;
    m_endPos = QPoint(col, row);
    emit selectionChanged();
}

void SelectionManager::endSelection()
{
    if (m_startPos == m_endPos) {
        clearSelection();
    }
}

void SelectionManager::clearSelection()
{
    m_hasSelection = false;
    emit selectionChanged();
}

QString SelectionManager::selectedText() const
{
    if (!m_hasSelection || !m_buffer) return {};

    int startRow = std::min(m_startPos.y(), m_endPos.y());
    int endRow = std::max(m_startPos.y(), m_endPos.y());
    int startCol = (m_startPos.y() <= m_endPos.y()) ? m_startPos.x() : m_endPos.x();
    int endCol = (m_startPos.y() <= m_endPos.y()) ? m_endPos.x() : m_startPos.x();

    // For single row
    if (startRow == endRow) {
        if (m_startPos.x() < m_endPos.x()) {
            startCol = m_startPos.x();
            endCol = m_endPos.x();
        } else {
            startCol = m_endPos.x();
            endCol = m_startPos.x();
        }
    }

    QString text;
    for (int row = startRow; row <= endRow && row < m_buffer->rows(); ++row) {
        if (row > startRow) text += '\n';
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : m_buffer->columns() - 1;

        for (int col = colStart; col <= colEnd && col < m_buffer->columns(); ++col) {
            text += m_buffer->cellAt(col, row).ch;
        }
    }

    return text;
}

bool SelectionManager::isSelected(int col, int row) const
{
    if (!m_hasSelection) return false;

    int startRow = std::min(m_startPos.y(), m_endPos.y());
    int endRow = std::max(m_startPos.y(), m_endPos.y());
    int startCol = std::min(m_startPos.x(), m_endPos.x());
    int endCol = std::max(m_startPos.x(), m_endPos.x());

    if (row < startRow || row > endRow) return false;
    if (row == startRow && col < startCol) return false;
    if (row == endRow && col > endCol) return false;
    return true;
}