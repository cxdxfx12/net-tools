#pragma once

#include <QString>
#include <QFont>
#include "TerminalColor.h"

struct TerminalTheme
{
    QString name;
    TerminalColor color;
    QFont font;
    double opacity = 1.0;

    static TerminalTheme defaultDark();
    static TerminalTheme solarizedDark();
    static TerminalTheme monokai();
    static TerminalTheme greenOnBlack();
};