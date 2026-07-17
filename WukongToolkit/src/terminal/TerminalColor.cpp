#include "terminal/TerminalColor.h"

TerminalColor::TerminalColor()
    : m_defaultFg("#DCDCDC")
    , m_defaultBg("#0C0C0C")
{
    initStandardColors();
}

void TerminalColor::initStandardColors()
{
    // Standard ANSI colors
    m_standardColors[0]  = QColor("#000000"); // Black
    m_standardColors[1]  = QColor("#CD0000"); // Red
    m_standardColors[2]  = QColor("#00CD00"); // Green
    m_standardColors[3]  = QColor("#CDCD00"); // Yellow
    m_standardColors[4]  = QColor("#0000EE"); // Blue
    m_standardColors[5]  = QColor("#CD00CD"); // Magenta
    m_standardColors[6]  = QColor("#00CDCD"); // Cyan
    m_standardColors[7]  = QColor("#E5E5E5"); // White
    m_standardColors[8]  = QColor("#7F7F7F"); // Bright Black
    m_standardColors[9]  = QColor("#FF0000"); // Bright Red
    m_standardColors[10] = QColor("#00FF00"); // Bright Green
    m_standardColors[11] = QColor("#FFFF00"); // Bright Yellow
    m_standardColors[12] = QColor("#5C5CFF"); // Bright Blue
    m_standardColors[13] = QColor("#FF00FF"); // Bright Magenta
    m_standardColors[14] = QColor("#00FFFF"); // Bright Cyan
    m_standardColors[15] = QColor("#FFFFFF"); // Bright White
}

QColor TerminalColor::ansiColor(int index) const
{
    return m_standardColors.value(index, m_defaultFg);
}

QColor TerminalColor::color256(int index) const
{
    // 6x6x6 color cube (16-231) + grayscale (232-255)
    if (index < 16) {
        return ansiColor(index);
    }
    if (index >= 16 && index <= 231) {
        int idx = index - 16;
        int r = (idx / 36) * 51;
        int g = ((idx / 6) % 6) * 51;
        int b = (idx % 6) * 51;
        return QColor(r, g, b);
    }
    if (index >= 232 && index <= 255) {
        int gray = (index - 232) * 10 + 8;
        return QColor(gray, gray, gray);
    }
    return m_defaultFg;
}

TerminalColor TerminalColor::defaultTheme()
{
    return TerminalColor();
}

TerminalColor TerminalColor::solarizedDark()
{
    TerminalColor theme;
    theme.m_defaultBg = QColor("#002B36");
    theme.m_defaultFg = QColor("#839496");
    theme.m_standardColors[0]  = QColor("#073642");
    theme.m_standardColors[1]  = QColor("#DC322F");
    theme.m_standardColors[2]  = QColor("#859900");
    theme.m_standardColors[3]  = QColor("#B58900");
    theme.m_standardColors[4]  = QColor("#268BD2");
    theme.m_standardColors[5]  = QColor("#D33682");
    theme.m_standardColors[6]  = QColor("#2AA198");
    theme.m_standardColors[7]  = QColor("#EEE8D5");
    theme.m_standardColors[8]  = QColor("#002B36");
    theme.m_standardColors[9]  = QColor("#CB4B16");
    theme.m_standardColors[10] = QColor("#586E75");
    theme.m_standardColors[11] = QColor("#657B83");
    theme.m_standardColors[12] = QColor("#839496");
    theme.m_standardColors[13] = QColor("#6C71C4");
    theme.m_standardColors[14] = QColor("#93A1A1");
    theme.m_standardColors[15] = QColor("#FDF6E3");
    return theme;
}

TerminalColor TerminalColor::monokai()
{
    TerminalColor theme;
    theme.m_defaultBg = QColor("#272822");
    theme.m_defaultFg = QColor("#F8F8F2");
    theme.m_standardColors[0]  = QColor("#272822");
    theme.m_standardColors[1]  = QColor("#F92672");
    theme.m_standardColors[2]  = QColor("#A6E22E");
    theme.m_standardColors[3]  = QColor("#F4BF75");
    theme.m_standardColors[4]  = QColor("#66D9EF");
    theme.m_standardColors[5]  = QColor("#AE81FF");
    theme.m_standardColors[6]  = QColor("#A1EFE4");
    theme.m_standardColors[7]  = QColor("#F8F8F2");
    theme.m_standardColors[8]  = QColor("#75715E");
    theme.m_standardColors[9]  = QColor("#F92672");
    theme.m_standardColors[10] = QColor("#A6E22E");
    theme.m_standardColors[11] = QColor("#F4BF75");
    theme.m_standardColors[12] = QColor("#66D9EF");
    theme.m_standardColors[13] = QColor("#AE81FF");
    theme.m_standardColors[14] = QColor("#A1EFE4");
    theme.m_standardColors[15] = QColor("#F8F8F2");
    return theme;
}