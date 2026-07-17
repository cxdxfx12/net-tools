#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QFont>

enum class AppTheme {
    Ocean,    // 暗夜蓝 — 深蓝底色+冰蓝强调，专业沉稳
    Forest,   // 森林绿 — 深绿底色+翡翠强调，护眼自然
    Sunset    // 日落橙 — 暖灰底色+琥珀强调，温暖活力
};

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    void applyOceanTheme();
    void applyForestTheme();
    void applySunsetTheme();
    void cycleTheme();
    AppTheme currentTheme() const { return m_currentTheme; }

    // ── Color Palette ──────────────────────────────────────────────
    // Backgrounds
    static QColor bgPrimary();
    static QColor bgSecondary();
    static QColor bgTertiary();
    static QColor bgInput();
    static QColor bgCard();
    static QColor bgHover();

    // Accent
    static QColor accentColor();
    static QColor accentHover();
    static QColor accentPressed();
    static QColor accentMuted();

    // Semantic
    static QColor dangerColor();
    static QColor dangerHover();
    static QColor warningColor();
    static QColor successColor();
    static QColor infoColor();

    // Text
    static QColor textPrimary();
    static QColor textSecondary();
    static QColor textDisabled();

    // Borders
    static QColor borderColor();
    static QColor borderFocus();
    static QColor borderSubtle();

    // Overlays
    static QColor hoverOverlay();
    static QColor hoverBackground();
    static QColor selectionColor();

    // Scrollbar
    static QColor scrollbarBg();
    static QColor scrollbarHandle();

    // ── Fonts ──────────────────────────────────────────────────────
    static QFont uiFont() {
        QFont f("Inter, SF Pro Text, PingFang SC, Microsoft YaHei, sans-serif", 12);
        f.setStyleStrategy(QFont::PreferAntialias);
        f.setHintingPreference(QFont::PreferNoHinting);
        return f;
    }
    static QFont monoFont() {
        QFont f("JetBrains Mono, SF Mono, Cascadia Code, Consolas, Menlo, monospace", 12);
        f.setStyleStrategy(QFont::PreferAntialias);
        return f;
    }
    static QFont titleFont() {
        QFont f("Inter, SF Pro Display, PingFang SC, Microsoft YaHei, sans-serif", 14, QFont::DemiBold);
        return f;
    }
    static QFont captionFont() {
        QFont f("Inter, SF Pro Text, PingFang SC, Microsoft YaHei, sans-serif", 10);
        return f;
    }

    QString styleSheet() const;
    static QString themeName(AppTheme theme);

signals:
    void themeChanged(AppTheme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // Theme palette data
    struct Palette {
        QColor bgPrimary, bgSecondary, bgTertiary, bgInput, bgCard, bgHover;
        QColor accent, accentHover, accentPressed;
        QColor danger, dangerHover, warning, success, info;
        QColor textPrimary, textSecondary, textDisabled;
        QColor border, borderSubtle;
        QColor hoverBackground, selection;
        QColor scrollbarHandle;
    };
    Palette paletteForTheme(AppTheme theme) const;

    AppTheme m_currentTheme = AppTheme::Ocean;
};
