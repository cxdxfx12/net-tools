#include "terminal/TerminalTheme.h"

TerminalTheme TerminalTheme::defaultDark()
{
    TerminalTheme theme;
    theme.name = "Default Dark";
    theme.color = TerminalColor::defaultTheme();
    theme.font = QFont("Consolas, Courier New, monospace", 14);
    return theme;
}

TerminalTheme TerminalTheme::solarizedDark()
{
    TerminalTheme theme;
    theme.name = "Solarized Dark";
    theme.color = TerminalColor::solarizedDark();
    theme.font = QFont("Consolas, Courier New, monospace", 14);
    return theme;
}

TerminalTheme TerminalTheme::monokai()
{
    TerminalTheme theme;
    theme.name = "Monokai";
    theme.color = TerminalColor::monokai();
    theme.font = QFont("Consolas, Courier New, monospace", 14);
    return theme;
}

TerminalTheme TerminalTheme::greenOnBlack()
{
    TerminalTheme theme;
    theme.name = "Green on Black";
    theme.color = TerminalColor::defaultTheme();
    theme.color.setDefaultFg(QColor("#00FF00"));
    theme.color.setDefaultBg(QColor("#000000"));
    theme.font = QFont("Consolas, Courier New, monospace", 14);
    return theme;
}