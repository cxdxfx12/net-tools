#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QFont>

enum class AppTheme {
    Dark,
    Light
};

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    void applyDarkTheme();
    void applyLightTheme();
    void toggleTheme();
    AppTheme currentTheme() const { return m_currentTheme; }

    // Color palette — adapts to current theme
    static QColor bgPrimary()     { return instance().m_currentTheme == AppTheme::Dark ? QColor("#16171B") : QColor("#F0F2F5"); }
    static QColor bgSecondary()   { return instance().m_currentTheme == AppTheme::Dark ? QColor("#1E1F23") : QColor("#FFFFFF"); }
    static QColor bgTertiary()    { return instance().m_currentTheme == AppTheme::Dark ? QColor("#25262B") : QColor("#F7F8FA"); }
    static QColor bgInput()       { return instance().m_currentTheme == AppTheme::Dark ? QColor("#2B2D31") : QColor("#F0F2F5"); }
    static QColor accentColor()   { return QColor("#409EFF"); }
    static QColor accentHover()   { return QColor("#66B1FF"); }
    static QColor accentPressed() { return QColor("#3A8EE6"); }
    static QColor dangerColor()   { return QColor("#F56C6C"); }
    static QColor warningColor()  { return QColor("#E6A23C"); }
    static QColor successColor()  { return QColor("#67C23A"); }
    static QColor infoColor()     { return QColor("#909399"); }
    static QColor textPrimary()   { return instance().m_currentTheme == AppTheme::Dark ? QColor("#E8E8ED") : QColor("#1D2129"); }
    static QColor textSecondary() { return instance().m_currentTheme == AppTheme::Dark ? QColor("#909399") : QColor("#86909C"); }
    static QColor textDisabled()  { return instance().m_currentTheme == AppTheme::Dark ? QColor("#5C5E66") : QColor("#C9CDD4"); }
    static QColor borderColor()   { return instance().m_currentTheme == AppTheme::Dark ? QColor("#3C3F45") : QColor("#E5E6EB"); }
    static QColor borderFocus()   { return QColor("#409EFF"); }
    static QColor hoverOverlay()  { return instance().m_currentTheme == AppTheme::Dark ? QColor("#FFFFFF") : QColor("#000000"); }
    static QColor selectionColor(){ return QColor("#409EFF"); }
    static QColor scrollbarBg()   { return QColor(Qt::transparent); }
    static QColor scrollbarHandle(){ return instance().m_currentTheme == AppTheme::Dark ? QColor("#3C3F4580") : QColor("#C9CDD480"); }

    static QFont uiFont() {
        QFont f("PingFang SC, Microsoft YaHei, Noto Sans, sans-serif", 11);
        f.setStyleStrategy(QFont::PreferAntialias);
        return f;
    }
    static QFont monoFont() {
        QFont f("JetBrains Mono, SF Mono, Consolas, Menlo, monospace", 10);
        f.setStyleStrategy(QFont::PreferAntialias);
        return f;
    }
    static QFont titleFont() {
        QFont f("PingFang SC, Microsoft YaHei, Noto Sans, sans-serif", 12, QFont::Medium);
        return f;
    }

    QString styleSheet() const;

signals:
    void themeChanged(AppTheme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    AppTheme m_currentTheme = AppTheme::Dark;
};