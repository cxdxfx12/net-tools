#pragma once

#include <QColor>
#include <QChar>

struct TerminalCell
{
    QChar ch = QChar(' ');
    QColor fg = QColor("#DCDCDC");
    QColor bg = QColor("#0C0C0C");
    bool bold = false;
    bool underline = false;
    bool reverse = false;
    bool blink = false;

    void reset()
    {
        ch = QChar(' ');
        fg = QColor("#DCDCDC");
        bg = QColor("#0C0C0C");
        bold = false;
        underline = false;
        reverse = false;
        blink = false;
    }

    void setChar(QChar c) { ch = c; }
    bool isNull() const { return ch == QChar(' ') && !bold && !underline && !reverse && !blink; }
};