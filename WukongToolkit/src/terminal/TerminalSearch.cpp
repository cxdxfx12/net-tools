#include "terminal/TerminalSearch.h"
#include "terminal/TerminalBuffer.h"

TerminalSearch::TerminalSearch(QObject* parent)
    : QObject(parent)
{
}

bool TerminalSearch::search(const QString& text, bool caseSensitive)
{
    if (text.isEmpty()) {
        clearSearch();
        return false;
    }

    m_searchText = text;
    m_caseSensitive = caseSensitive;
    findAllMatches(text, caseSensitive);

    if (!m_matches.isEmpty()) {
        m_currentMatch = 0;
        auto& match = m_matches[0];
        emit matchFound(match.col, match.row);
        emit searchResult(m_matches.size(), 1);
        return true;
    }

    emit searchResult(0, 0);
    return false;
}

bool TerminalSearch::searchNext()
{
    if (m_matches.isEmpty()) return false;

    m_currentMatch = (m_currentMatch + 1) % m_matches.size();
    auto& match = m_matches[m_currentMatch];
    emit matchFound(match.col, match.row);
    emit searchResult(m_matches.size(), m_currentMatch + 1);
    return true;
}

bool TerminalSearch::searchPrevious()
{
    if (m_matches.isEmpty()) return false;

    m_currentMatch = (m_currentMatch - 1 + m_matches.size()) % m_matches.size();
    auto& match = m_matches[m_currentMatch];
    emit matchFound(match.col, match.row);
    emit searchResult(m_matches.size(), m_currentMatch + 1);
    return true;
}

void TerminalSearch::clearSearch()
{
    m_searchText.clear();
    m_matches.clear();
    m_currentMatch = -1;
    emit searchResult(0, 0);
}

bool TerminalSearch::isMatchAt(int col, int row) const
{
    if (m_currentMatch < 0 || m_currentMatch >= m_matches.size()) return false;

    auto& match = m_matches[m_currentMatch];
    if (row != match.row) return false;
    return col >= match.col && col < match.col + m_searchText.length();
}

void TerminalSearch::findAllMatches(const QString& text, bool caseSensitive)
{
    m_matches.clear();
    if (!m_buffer) return;

    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int row = 0; row < m_buffer->rows(); ++row) {
        QString line = lineText(row);
        int col = 0;
        while ((col = line.indexOf(text, col, cs)) != -1) {
            m_matches.append({row, col});
            col += text.length();
        }
    }
}

QString TerminalSearch::lineText(int row) const
{
    if (!m_buffer || row < 0 || row >= m_buffer->rows()) return {};

    QString text;
    for (int col = 0; col < m_buffer->columns(); ++col) {
        text += m_buffer->cellAt(col, row).ch;
    }
    return text;
}