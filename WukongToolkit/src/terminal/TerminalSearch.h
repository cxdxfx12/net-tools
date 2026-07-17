#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class TerminalBuffer;

class TerminalSearch : public QObject
{
    Q_OBJECT

public:
    explicit TerminalSearch(QObject* parent = nullptr);

    void setBuffer(TerminalBuffer* buffer) { m_buffer = buffer; }

    bool search(const QString& text, bool caseSensitive = false);
    bool searchNext();
    bool searchPrevious();
    void clearSearch();

    bool hasMatch() const { return !m_matches.isEmpty(); }
    int matchCount() const { return m_matches.size(); }
    int currentMatchIndex() const { return m_currentMatch; }
    bool isMatchAt(int col, int row) const;

signals:
    void searchResult(int matchCount, int currentIndex);
    void matchFound(int col, int row);

private:
    void findAllMatches(const QString& text, bool caseSensitive);
    QString lineText(int row) const;

    TerminalBuffer* m_buffer = nullptr;
    QString m_searchText;
    bool m_caseSensitive = false;

    struct Match {
        int row;
        int col;
    };
    QVector<Match> m_matches;
    int m_currentMatch = -1;
};