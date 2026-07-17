#include "ui/ThemeManager.h"
#include <QApplication>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

QString ThemeManager::themeName(AppTheme theme)
{
    switch (theme) {
    case AppTheme::Ocean:  return QStringLiteral("暗夜蓝");
    case AppTheme::Forest: return QStringLiteral("森林绿");
    case AppTheme::Sunset: return QStringLiteral("日落橙");
    }
    return QStringLiteral("未知");
}

// ═══════════════════════════════════════════════════════════════════════════
// Palette Definitions
// ═══════════════════════════════════════════════════════════════════════════

ThemeManager::Palette ThemeManager::paletteForTheme(AppTheme theme) const
{
    switch (theme) {

    // ── 暗夜蓝 (Ocean) — 深海蓝底+冰蓝强调，专业冷静，适合长时间工作 ──
    case AppTheme::Ocean:
        return {
            /* bgPrimary      */ QColor("#0D1117"),
            /* bgSecondary    */ QColor("#161B22"),
            /* bgTertiary     */ QColor("#1C2128"),
            /* bgInput        */ QColor("#1C2128"),
            /* bgCard         */ QColor("#161B22"),
            /* bgHover        */ QColor("#21262D"),
            /* accent         */ QColor("#58A6FF"),
            /* accentHover    */ QColor("#79C0FF"),
            /* accentPressed  */ QColor("#388BFD"),
            /* danger         */ QColor("#F85149"),
            /* dangerHover    */ QColor("#FF7B72"),
            /* warning        */ QColor("#D29922"),
            /* success        */ QColor("#3FB950"),
            /* info           */ QColor("#8B949E"),
            /* textPrimary    */ QColor("#E6EDF3"),
            /* textSecondary  */ QColor("#8B949E"),
            /* textDisabled   */ QColor("#484F58"),
            /* border         */ QColor("#30363D"),
            /* borderSubtle   */ QColor("#21262D"),
            /* hoverBackground*/ QColor(255, 255, 255, 12),
            /* selection      */ QColor(88, 166, 255, 40),
            /* scrollbarHandle*/ QColor("#30363D"),
        };

    // ── 森林绿 (Forest) — 松露绿底+翡翠强调，护眼自然，适合夜间编码 ──
    case AppTheme::Forest:
        return {
            /* bgPrimary      */ QColor("#0D1A12"),
            /* bgSecondary    */ QColor("#14281C"),
            /* bgTertiary     */ QColor("#1A3323"),
            /* bgInput        */ QColor("#1A3323"),
            /* bgCard         */ QColor("#14281C"),
            /* bgHover        */ QColor("#1E3A2A"),
            /* accent         */ QColor("#3FB950"),
            /* accentHover    */ QColor("#56D364"),
            /* accentPressed  */ QColor("#2EA043"),
            /* danger         */ QColor("#F85149"),
            /* dangerHover    */ QColor("#FF7B72"),
            /* warning        */ QColor("#D29922"),
            /* success        */ QColor("#3FB950"),
            /* info           */ QColor("#7D9B87"),
            /* textPrimary    */ QColor("#D2E8D8"),
            /* textSecondary  */ QColor("#7D9B87"),
            /* textDisabled   */ QColor("#4A6B54"),
            /* border         */ QColor("#2B4A34"),
            /* borderSubtle   */ QColor("#1E3A2A"),
            /* hoverBackground*/ QColor(63, 185, 80, 20),
            /* selection      */ QColor(63, 185, 80, 35),
            /* scrollbarHandle*/ QColor("#2B4A34"),
        };

    // ── 日落橙 (Sunset) — 暖灰底+琥珀强调，温暖活力，适合晨间工作 ──
    case AppTheme::Sunset:
        return {
            /* bgPrimary      */ QColor("#1A1410"),
            /* bgSecondary    */ QColor("#241D17"),
            /* bgTertiary     */ QColor("#2D241C"),
            /* bgInput        */ QColor("#2D241C"),
            /* bgCard         */ QColor("#241D17"),
            /* bgHover        */ QColor("#352B21"),
            /* accent         */ QColor("#D29922"),
            /* accentHover    */ QColor("#E2A83F"),
            /* accentPressed  */ QColor("#B8860E"),
            /* danger         */ QColor("#F85149"),
            /* dangerHover    */ QColor("#FF7B72"),
            /* warning        */ QColor("#D29922"),
            /* success        */ QColor("#3FB950"),
            /* info           */ QColor("#A0907A"),
            /* textPrimary    */ QColor("#F0E6D8"),
            /* textSecondary  */ QColor("#A0907A"),
            /* textDisabled   */ QColor("#5E5448"),
            /* border         */ QColor("#3D342A"),
            /* borderSubtle   */ QColor("#2D241C"),
            /* hoverBackground*/ QColor(210, 153, 34, 18),
            /* selection      */ QColor(210, 153, 34, 35),
            /* scrollbarHandle*/ QColor("#3D342A"),
        };
    }
    // unreachable
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════
// Color Getters (delegate to current theme palette)
// ═══════════════════════════════════════════════════════════════════════════

#define THEME_PALETTE instance().paletteForTheme(instance().m_currentTheme)

QColor ThemeManager::bgPrimary()       { return THEME_PALETTE.bgPrimary; }
QColor ThemeManager::bgSecondary()     { return THEME_PALETTE.bgSecondary; }
QColor ThemeManager::bgTertiary()      { return THEME_PALETTE.bgTertiary; }
QColor ThemeManager::bgInput()         { return THEME_PALETTE.bgInput; }
QColor ThemeManager::bgCard()          { return THEME_PALETTE.bgCard; }
QColor ThemeManager::bgHover()         { return THEME_PALETTE.bgHover; }
QColor ThemeManager::accentColor()     { return THEME_PALETTE.accent; }
QColor ThemeManager::accentHover()     { return THEME_PALETTE.accentHover; }
QColor ThemeManager::accentPressed()   { return THEME_PALETTE.accentPressed; }
QColor ThemeManager::accentMuted()     {
    QColor c = THEME_PALETTE.accent;
    c.setAlpha(30);
    return c;
}
QColor ThemeManager::dangerColor()     { return THEME_PALETTE.danger; }
QColor ThemeManager::dangerHover()     { return THEME_PALETTE.dangerHover; }
QColor ThemeManager::warningColor()    { return THEME_PALETTE.warning; }
QColor ThemeManager::successColor()    { return THEME_PALETTE.success; }
QColor ThemeManager::infoColor()       { return THEME_PALETTE.info; }
QColor ThemeManager::textPrimary()     { return THEME_PALETTE.textPrimary; }
QColor ThemeManager::textSecondary()   { return THEME_PALETTE.textSecondary; }
QColor ThemeManager::textDisabled()    { return THEME_PALETTE.textDisabled; }
QColor ThemeManager::borderColor()     { return THEME_PALETTE.border; }
QColor ThemeManager::borderFocus()     { return THEME_PALETTE.accent; }
QColor ThemeManager::borderSubtle()    { return THEME_PALETTE.borderSubtle; }
QColor ThemeManager::hoverBackground() { return THEME_PALETTE.hoverBackground; }
QColor ThemeManager::selectionColor()  { return THEME_PALETTE.selection; }
QColor ThemeManager::scrollbarBg()     { return QColor(Qt::transparent); }
QColor ThemeManager::scrollbarHandle() { return THEME_PALETTE.scrollbarHandle; }
QColor ThemeManager::hoverOverlay()    { return THEME_PALETTE.textPrimary; }

#undef THEME_PALETTE

// ═══════════════════════════════════════════════════════════════════════════
// Theme Application
// ═══════════════════════════════════════════════════════════════════════════

void ThemeManager::applyOceanTheme()
{
    m_currentTheme = AppTheme::Ocean;
    qApp->setStyleSheet(styleSheet());
    qApp->setFont(uiFont());
    emit themeChanged(AppTheme::Ocean);
}

void ThemeManager::applyForestTheme()
{
    m_currentTheme = AppTheme::Forest;
    qApp->setStyleSheet(styleSheet());
    qApp->setFont(uiFont());
    emit themeChanged(AppTheme::Forest);
}

void ThemeManager::applySunsetTheme()
{
    m_currentTheme = AppTheme::Sunset;
    qApp->setStyleSheet(styleSheet());
    qApp->setFont(uiFont());
    emit themeChanged(AppTheme::Sunset);
}

void ThemeManager::cycleTheme()
{
    switch (m_currentTheme) {
    case AppTheme::Ocean:  applyForestTheme(); break;
    case AppTheme::Forest: applySunsetTheme(); break;
    case AppTheme::Sunset: applyOceanTheme();  break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// QSS Stylesheet
// ═══════════════════════════════════════════════════════════════════════════

static QString colorStr(const QColor& c) {
    if (c.alpha() == 255) {
        return c.name(QColor::HexRgb);
    } else {
        return QString("rgba(%1, %2, %3, %4%)")
            .arg(c.red())
            .arg(c.green())
            .arg(c.blue())
            .arg(c.alpha() * 100 / 255);
    }
}

QString ThemeManager::styleSheet() const
{
    // Color variables
    QString vBg1   = colorStr(bgPrimary());       // %1
    QString vBg2   = colorStr(bgSecondary());     // %2
    QString vBg3   = colorStr(bgTertiary());      // %3
    QString vBgInp = colorStr(bgInput());         // %4
    QString vBgCard= colorStr(bgCard());          // %5
    QString vBgHov = colorStr(bgHover());         // %6
    QString vTxt1  = colorStr(textPrimary());     // %7
    QString vTxt2  = colorStr(textSecondary());   // %8
    QString vTxtD  = colorStr(textDisabled());    // %9
    QString vBdr   = colorStr(borderColor());     // %10
    QString vBdrS  = colorStr(borderSubtle());    // %11
    QString vAcc   = colorStr(accentColor());     // %12
    QString vAccH  = colorStr(accentHover());     // %13
    QString vAccP  = colorStr(accentPressed());   // %14
    QString vAccM  = colorStr(accentMuted());     // %15
    QString vDang  = colorStr(dangerColor());     // %16
    QString vSucc  = colorStr(successColor());    // %17
    QString vWarn  = colorStr(warningColor());    // %18
    QString vInfo  = colorStr(infoColor());       // %19
    QString vSclBg = colorStr(scrollbarBg());     // %20
    QString vSclH  = colorStr(scrollbarHandle()); // %21
    QString vHovBg = colorStr(hoverBackground()); // %22
    QString vSel   = colorStr(selectionColor());  // %23

    return QStringLiteral(
        // ═══════════════════════════════════════════════════════════════
        // GLOBAL
        // ═══════════════════════════════════════════════════════════════
        "* {"
        "  outline: none;"
        "}"
        "QWidget {"
        "  background-color: %1;"
        "  color: %7;"
        "  font-family: \"Inter\", \"SF Pro Text\", \"PingFang SC\", \"Microsoft YaHei\", sans-serif;"
        "  font-size: 12px;"
        "  selection-background-color: %23;"
        "  selection-color: %7;"
        "}"
        "QMainWindow {"
        "  background-color: %1;"
        "}"
        "QMainWindow::separator {"
        "  background-color: %10;"
        "  width: 1px;"
        "  height: 1px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // MENUBAR
        // ═══════════════════════════════════════════════════════════════
        "QMenuBar {"
        "  background-color: %2;"
        "  color: %7;"
        "  border-bottom: 1px solid %11;"
        "  padding: 2px 0px;"
        "  font-size: 12px;"
        "}"
        "QMenuBar::item {"
        "  background: transparent;"
        "  padding: 6px 12px;"
        "  border-radius: 6px;"
        "  margin: 2px 4px;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: %6;"
        "}"
        "QMenuBar::item:pressed {"
        "  background-color: %22;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // MENU
        // ═══════════════════════════════════════════════════════════════
        "QMenu {"
        "  background-color: %2;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 8px;"
        "  padding: 4px 0px;"
        "  font-size: 12px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 28px 6px 14px;"
        "  margin: 2px 6px;"
        "  border-radius: 5px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: %6;"
        "}"
        "QMenu::item:disabled {"
        "  color: %9;"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background-color: %10;"
        "  margin: 4px 10px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // TOOLBAR
        // ═══════════════════════════════════════════════════════════════
        "QToolBar {"
        "  background-color: %2;"
        "  border-bottom: 1px solid %11;"
        "  padding: 4px 8px;"
        "  spacing: 4px;"
        "}"
        "QToolBar::separator {"
        "  width: 1px;"
        "  background-color: %10;"
        "  margin: 4px 6px;"
        "}"
        "QToolButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 12px;"
        "  color: %8;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QToolButton:hover {"
        "  background-color: %6;"
        "  color: %7;"
        "}"
        "QToolButton:pressed {"
        "  background-color: %22;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // TREEWIDGET (Navigation sidebar)
        // ═══════════════════════════════════════════════════════════════
        "QTreeWidget {"
        "  background-color: %2;"
        "  color: %8;"
        "  border: none;"
        "  outline: none;"
        "  font-size: 12px;"
        "  padding: 4px;"
        "}"
        "QTreeWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "  margin: 1px 0px;"
        "}"
        "QTreeWidget::item:selected {"
        "  background-color: %23;"
        "  color: %12;"
        "}"
        "QTreeWidget::item:hover {"
        "  background-color: %6;"
        "  color: %7;"
        "}"
        "QTreeWidget::branch {"
        "  background-color: %2;"
        "}"
        "QTreeWidget::branch:has-children:!has-siblings:closed,"
        "QTreeWidget::branch:closed:has-children:has-siblings {"
        "  border-image: none;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // DOCKWIDGET
        // ═══════════════════════════════════════════════════════════════
        "QDockWidget {"
        "  color: %7;"
        "  titlebar-close-icon: none;"
        "  titlebar-normal-icon: none;"
        "}"
        "QDockWidget::title {"
        "  background-color: %2;"
        "  padding: 8px 12px;"
        "  border-bottom: 1px solid %11;"
        "  font-weight: 600;"
        "  font-size: 11px;"
        "  color: %8;"
        "  text-transform: uppercase;"
        "  letter-spacing: 0.5px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // TABWIDGET
        // ═══════════════════════════════════════════════════════════════
        "QTabWidget::pane {"
        "  background-color: %1;"
        "  border: none;"
        "  border-top: 1px solid %11;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: transparent;"
        "  color: %8;"
        "  padding: 8px 16px;"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "  margin-right: 0px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QTabBar::tab:selected {"
        "  color: %7;"
        "  border-bottom: 2px solid %12;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  color: %7;"
        "  background-color: %6;"
        "  border-bottom: 2px solid %10;"
        "}"
        "QTabBar::close-button {"
        "  image: none;"
        "  background: %8;"
        "  border-radius: 2px;"
        "  margin: 2px;"
        "}"
        "QTabBar::close-button:hover {"
        "  background: %7;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // PUSHBUTTON
        // ═══════════════════════════════════════════════════════════════
        "QPushButton {"
        "  background-color: %3;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  padding: 6px 16px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "  min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %6;"
        "  border-color: %10;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %22;"
        "}"
        "QPushButton:disabled {"
        "  background-color: %11;"
        "  color: %9;"
        "  border-color: %11;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // LINEEDIT
        // ═══════════════════════════════════════════════════════════════
        "QLineEdit {"
        "  background-color: %4;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  min-height: 18px;"
        "  font-size: 12px;"
        "  selection-background-color: %23;"
        "}"
        "QLineEdit:focus {"
        "  border-color: %12;"
        "  box-shadow: 0 0 0 2px %15;"
        "}"
        "QLineEdit:disabled {"
        "  background-color: %11;"
        "  color: %9;"
        "}"
        "QLineEdit[readOnly=\"true\"] {"
        "  background-color: %3;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // COMBOBOX
        // ═══════════════════════════════════════════════════════════════
        "QComboBox {"
        "  background-color: %4;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  padding: 7px 28px 7px 12px;"
        "  min-height: 18px;"
        "  font-size: 12px;"
        "}"
        "QComboBox:hover {"
        "  border-color: %12;"
        "}"
        "QComboBox:focus {"
        "  border-color: %12;"
        "  box-shadow: 0 0 0 2px %15;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "  subcontrol-position: center right;"
        "  subcontrol-origin: padding;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 4px solid transparent;"
        "  border-right: 4px solid transparent;"
        "  border-top: 5px solid %8;"
        "  margin-right: 4px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %2;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  selection-background-color: %6;"
        "  outline: none;"
        "  padding: 4px;"
        "  font-size: 12px;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  padding: 6px 12px;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "  background-color: %23;"
        "  color: %12;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // SPINBOX
        // ═══════════════════════════════════════════════════════════════
        "QSpinBox, QDoubleSpinBox {"
        "  background-color: %4;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  min-height: 18px;"
        "  font-size: 12px;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border-color: %12;"
        "  box-shadow: 0 0 0 2px %15;"
        "}"
        "QSpinBox::up-button, QDoubleSpinBox::up-button {"
        "  border: none;"
        "  border-left: 1px solid %10;"
        "  border-bottom: 1px solid %10;"
        "  border-top-right-radius: 5px;"
        "  background-color: %3;"
        "  width: 22px;"
        "}"
        "QSpinBox::down-button, QDoubleSpinBox::down-button {"
        "  border: none;"
        "  border-left: 1px solid %10;"
        "  border-bottom-right-radius: 5px;"
        "  background-color: %3;"
        "  width: 22px;"
        "}"
        "QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,"
        "QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {"
        "  background-color: %6;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // PLAINTEXTEDIT / TEXTEDIT
        // ═══════════════════════════════════════════════════════════════
        "QPlainTextEdit, QTextEdit {"
        "  background-color: %1;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  selection-background-color: %23;"
        "  font-family: \"JetBrains Mono\", \"SF Mono\", \"Cascadia Code\", \"Consolas\", \"Menlo\", monospace;"
        "  font-size: 12px;"
        "  padding: 8px;"
        "}"
        "QPlainTextEdit:focus, QTextEdit:focus {"
        "  border-color: %12;"
        "  box-shadow: 0 0 0 2px %15;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // TABLEVIEW / TABLEWIDGET
        // ═══════════════════════════════════════════════════════════════
        "QTableView, QTableWidget {"
        "  background-color: %2;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 8px;"
        "  gridline-color: %11;"
        "  selection-background-color: %23;"
        "  selection-color: %7;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "QTableView::item, QTableWidget::item {"
        "  padding: 6px 12px;"
        "  border-bottom: 1px solid %11;"
        "}"
        "QTableView::item:selected, QTableWidget::item:selected {"
        "  background-color: %23;"
        "  color: %7;"
        "}"
        "QHeaderView::section {"
        "  background-color: %3;"
        "  color: %8;"
        "  padding: 8px 12px;"
        "  border: none;"
        "  border-bottom: 1px solid %10;"
        "  font-weight: 600;"
        "  font-size: 11px;"
        "  text-transform: uppercase;"
        "  letter-spacing: 0.5px;"
        "}"
        "QHeaderView::section:hover {"
        "  background-color: %6;"
        "  color: %7;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // SCROLLBAR
        // ═══════════════════════════════════════════════════════════════
        "QScrollBar:vertical {"
        "  background: %20;"
        "  width: 8px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %21;"
        "  border-radius: 4px;"
        "  min-height: 30px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: %8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "  background: %20;"
        "  height: 8px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: %21;"
        "  border-radius: 4px;"
        "  min-width: 30px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: %8;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // STATUSBAR
        // ═══════════════════════════════════════════════════════════════
        "QStatusBar {"
        "  background-color: %2;"
        "  color: %8;"
        "  border-top: 1px solid %11;"
        "  font-size: 11px;"
        "  padding: 2px 4px;"
        "  min-height: 24px;"
        "}"
        "QStatusBar::item {"
        "  border: none;"
        "}"
        "QStatusBar QLabel {"
        "  padding: 2px 10px;"
        "  color: %8;"
        "  background: transparent;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // SPLITTER
        // ═══════════════════════════════════════════════════════════════
        "QSplitter::handle {"
        "  background-color: %10;"
        "}"
        "QSplitter::handle:horizontal {"
        "  width: 1px;"
        "}"
        "QSplitter::handle:vertical {"
        "  height: 1px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // LABEL
        // ═══════════════════════════════════════════════════════════════
        "QLabel {"
        "  color: %7;"
        "  background: transparent;"
        "  font-size: 12px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // GROUPBOX
        // ═══════════════════════════════════════════════════════════════
        "QGroupBox {"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 8px;"
        "  margin-top: 16px;"
        "  padding: 18px 14px 14px 14px;"
        "  font-weight: 600;"
        "  font-size: 12px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  padding: 0px 10px;"
        "  left: 12px;"
        "  color: %7;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // CHECKBOX / RADIOBUTTON
        // ═══════════════════════════════════════════════════════════════
        "QCheckBox, QRadioButton {"
        "  color: %7;"
        "  spacing: 8px;"
        "  font-size: 12px;"
        "}"
        "QCheckBox::indicator, QRadioButton::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // PROGRESSBAR
        // ═══════════════════════════════════════════════════════════════
        "QProgressBar {"
        "  background-color: %4;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  text-align: center;"
        "  color: %7;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "  height: 6px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %12;"
        "  border-radius: 5px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // TOOLTIP
        // ═══════════════════════════════════════════════════════════════
        "QToolTip {"
        "  background-color: %2;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "  font-size: 11px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // LISTWIDGET
        // ═══════════════════════════════════════════════════════════════
        "QListWidget {"
        "  background-color: %2;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 8px;"
        "  outline: none;"
        "  font-size: 12px;"
        "  padding: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 4px;"
        "  margin: 1px 0px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: %23;"
        "  color: %12;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: %6;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // DATEEDIT / TIMEEDIT
        // ═══════════════════════════════════════════════════════════════
        "QDateEdit, QTimeEdit {"
        "  background-color: %4;"
        "  color: %7;"
        "  border: 1px solid %10;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  font-size: 12px;"
        "}"
        "QDateEdit:focus, QTimeEdit:focus {"
        "  border-color: %12;"
        "  box-shadow: 0 0 0 2px %15;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // DIALOG
        // ═══════════════════════════════════════════════════════════════
        "QDialog {"
        "  background-color: %2;"
        "  color: %7;"
        "  border-radius: 10px;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // SLIDER
        // ═══════════════════════════════════════════════════════════════
        "QSlider::groove:horizontal {"
        "  background: %10;"
        "  height: 4px;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: %12;"
        "  width: 14px;"
        "  height: 14px;"
        "  margin: -5px 0;"
        "  border-radius: 7px;"
        "  border: 2px solid %2;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: %13;"
        "}"

        // ═══════════════════════════════════════════════════════════════
        // FRAME / CARD
        // ═══════════════════════════════════════════════════════════════
        "QFrame[card=\"true\"] {"
        "  background-color: %5;"
        "  border: 1px solid %10;"
        "  border-radius: 8px;"
        "}"
    ).arg(
        vBg1, vBg2, vBg3, vBgInp, vBgCard, vBgHov,
        vTxt1, vTxt2, vTxtD, vBdr, vBdrS,
        vAcc, vAccH, vAccP, vAccM,
        vDang, vSucc, vWarn, vInfo,
        vSclBg, vSclH, vHovBg, vSel
    );
}
