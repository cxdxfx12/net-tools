#pragma once

#include <QColor>
#include <QMap>
#include <QString>

class TerminalColor
{
public:
    TerminalColor();

    // Standard 16 ANSI colors
    QColor ansiColor(int index) const;
    QColor defaultFg() const { return m_defaultFg; }
    QColor defaultBg() const { return m_defaultBg; }

    // 256-color palette
    QColor color256(int index) const;

    void setDefaultFg(const QColor& color) { m_defaultFg = color; }
    void setDefaultBg(const QColor& color) { m_defaultBg = color; }

    // Theme presets
    static TerminalColor defaultTheme();
    static TerminalColor solarizedDark();
    static TerminalColor monokai();

private:
    void initStandardColors();

    QColor m_defaultFg;
    QColor m_defaultBg;
    QMap<int, QColor> m_standardColors;
};