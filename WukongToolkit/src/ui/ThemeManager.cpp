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

void ThemeManager::applyDarkTheme()
{
    m_currentTheme = AppTheme::Dark;
    qApp->setStyleSheet(styleSheet());
    qApp->setFont(uiFont());
    emit themeChanged(AppTheme::Dark);
}

void ThemeManager::applyLightTheme()
{
    m_currentTheme = AppTheme::Light;
    qApp->setStyleSheet(styleSheet());
    qApp->setFont(uiFont());
    emit themeChanged(AppTheme::Light);
}

void ThemeManager::toggleTheme()
{
    if (m_currentTheme == AppTheme::Dark) {
        applyLightTheme();
    } else {
        applyDarkTheme();
    }
}

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
    bool isDark = (m_currentTheme == AppTheme::Dark);

    QString bg1       = colorStr(bgPrimary());
    QString bg2       = colorStr(bgSecondary());
    QString bg3       = colorStr(bgTertiary());
    QString bgInp     = colorStr(bgInput());
    QString txt1      = colorStr(textPrimary());
    QString txt2      = colorStr(textSecondary());
    QString txtD      = colorStr(textDisabled());
    QString bdr       = colorStr(borderColor());
    QString acc       = colorStr(accentColor());
    QString accH      = colorStr(accentHover());
    QString accP      = colorStr(accentPressed());
    QString dang      = colorStr(dangerColor());
    QString succ      = colorStr(successColor());
    QString warn      = colorStr(warningColor());
    QString infoG     = colorStr(infoColor());
    QString hovBg     = colorStr(hoverOverlay());
    QString sclBg     = colorStr(scrollbarBg());
    QString sclH      = colorStr(scrollbarHandle());

    QString hoverAlpha = isDark ? QStringLiteral("0.08") : QStringLiteral("0.06");

    QString sheet = QStringLiteral(
        /* ── Global ── */
        "QWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: \"PingFang SC\", \"Microsoft YaHei\", \"Noto Sans\", sans-serif;"
        "  font-size: 11px;"
        "}"
        "QMainWindow { background-color: %1; }"
        "QMainWindow::separator {"
        "  background-color: %4;"
        "  width: 1px; height: 1px;"
        "}"
        /* ── MenuBar ── */
        "QMenuBar {"
        "  background-color: %3;"
        "  color: %2;"
        "  border-bottom: 1px solid %4;"
        "  padding: 3px 0px;"
        "}"
        "QMenuBar::item {"
        "  background: transparent;"
        "  padding: 7px 14px;"
        "  border-radius: 6px;"
        "  margin: 2px 4px;"
        "}"
        "QMenuBar::item:selected { background-color: %5%6; }"
        /* ── Menu ── */
        "QMenu {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  padding: 6px 0px;"
        "}"
        "QMenu::item {"
        "  padding: 7px 32px 7px 18px;"
        "  margin: 2px 6px;"
        "  border-radius: 6px;"
        "}"
        "QMenu::item:selected { background-color: %5%6; }"
        "QMenu::separator {"
        "  height: 1px;"
        "  background-color: %4;"
        "  margin: 6px 12px;"
        "}"
        /* ── ToolBar ── */
        "QToolBar {"
        "  background-color: %3;"
        "  border-bottom: 1px solid %4;"
        "  padding: 4px 6px;"
        "  spacing: 4px;"
        "}"
        "QToolBar::separator {"
        "  width: 1px;"
        "  background-color: %4;"
        "  margin: 6px 6px;"
        "}"
        "QToolButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 10px;"
        "  color: %2;"
        "  font-size: 11px;"
        "}"
        "QToolButton:hover { background-color: %5%6; }"
        "QToolButton:pressed { background-color: %5%6; }"
        /* ── TreeWidget ── */
        "QTreeWidget {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: none;"
        "  outline: none;"
        "  font-size: 11px;"
        "}"
        "QTreeWidget::item {"
        "  padding: 7px 10px;"
        "  border-radius: 6px;"
        "}"
        "QTreeWidget::item:selected { background-color: %5%6; }"
        "QTreeWidget::item:hover { background-color: %5%6; }"
        "QTreeWidget::branch { background-color: %3; }"
        /* ── DockWidget ── */
        "QDockWidget { color: %2; }"
        "QDockWidget::title {"
        "  background-color: %3;"
        "  padding: 8px 14px;"
        "  border-bottom: 1px solid %4;"
        "  font-weight: 500;"
        "}"
        /* ── TabWidget ── */
        "QTabWidget::pane {"
        "  background-color: %1;"
        "  border: 1px solid %4;"
        "  border-radius: 0 8px 8px 8px;"
        "}"
        "QTabBar::tab {"
        "  background-color: %3;"
        "  color: %8;"
        "  padding: 8px 18px;"
        "  border: 1px solid %4;"
        "  border-bottom: none;"
        "  border-radius: 8px 8px 0 0;"
        "  margin-right: 2px;"
        "  font-size: 11px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: %1;"
        "  color: %7;"
        "  border-bottom: 2px solid %7;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: %5%6;"
        "  color: %2;"
        "}"
        /* ── Buttons ── */
        "QPushButton {"
        "  background-color: %7;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 18px;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: %9; }"
        "QPushButton:pressed { background-color: %10; }"
        "QPushButton:disabled {"
        "  background-color: %4;"
        "  color: %8;"
        "}"
        /* ── LineEdit ── */
        "QLineEdit {"
        "  background-color: %11;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  min-height: 20px;"
        "  font-size: 11px;"
        "  selection-background-color: %7;"
        "}"
        "QLineEdit:focus {"
        "  border-color: %7;"
        "  border-width: 1.5px;"
        "}"
        /* ── ComboBox ── */
        "QComboBox {"
        "  background-color: %11;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  min-height: 20px;"
        "  font-size: 11px;"
        "  padding-right: 28px;"
        "}"
        "QComboBox:focus { border-color: %7; }"
        "QComboBox::drop-down {"
        "  border: none;"
        "  padding-right: 10px;"
        "  width: 24px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  selection-background-color: %5%6;"
        "  outline: none;"
        "  padding: 4px;"
        "}"
        /* ── SpinBox ── */
        "QSpinBox, QDoubleSpinBox {"
        "  background-color: %11;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  min-height: 20px;"
        "  font-size: 11px;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus { border-color: %7; }"
        /* ── TextEdit ── */
        "QPlainTextEdit, QTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  selection-background-color: %7;"
        "  font-family: \"JetBrains Mono\", \"SF Mono\", \"Consolas\", \"Menlo\", monospace;"
        "  font-size: 10px;"
        "  padding: 6px;"
        "}"
        /* ── TableView ── */
        "QTableView, QTableWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  gridline-color: %4;"
        "  selection-background-color: %5%6;"
        "  font-size: 11px;"
        "  outline: none;"
        "}"
        "QTableView::item, QTableWidget::item { padding: 5px 10px; }"
        "QHeaderView::section {"
        "  background-color: %3;"
        "  color: %2;"
        "  padding: 7px 12px;"
        "  border: none;"
        "  border-right: 1px solid %4;"
        "  border-bottom: 1px solid %4;"
        "  font-weight: 500;"
        "  font-size: 11px;"
        "}"
        /* ── ScrollBar ── */
        "QScrollBar:vertical {"
        "  background: %12;"
        "  width: 8px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %13;"
        "  border-radius: 4px;"
        "  min-height: 30px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: %13; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal {"
        "  background: %12;"
        "  height: 8px;"
        "  border: none;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: %13;"
        "  border-radius: 4px;"
        "  min-width: 30px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover { background: %13; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        /* ── StatusBar ── */
        "QStatusBar {"
        "  background-color: %3;"
        "  color: %8;"
        "  border-top: 1px solid %4;"
        "  font-size: 10px;"
        "  padding: 2px;"
        "}"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { padding: 0px 8px; }"
        /* ── Splitter ── */
        "QSplitter::handle { background-color: %4; }"
        "QSplitter::handle:horizontal { width: 1px; }"
        "QSplitter::handle:vertical { height: 1px; }"
        /* ── Label ── */
        "QLabel { color: %2; background: transparent; font-size: 11px; }"
        /* ── GroupBox ── */
        "QGroupBox {"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  margin-top: 14px;"
        "  padding: 18px 12px 12px 12px;"
        "  font-weight: 500;"
        "  font-size: 11px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  padding: 0px 10px;"
        "  left: 12px;"
        "}"
        /* ── CheckBox / RadioButton ── */
        "QCheckBox, QRadioButton {"
        "  color: %2;"
        "  spacing: 8px;"
        "  font-size: 11px;"
        "}"
        "QCheckBox::indicator, QRadioButton::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "}"
        /* ── ProgressBar ── */
        "QProgressBar {"
        "  background-color: %11;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  text-align: center;"
        "  color: %2;"
        "  font-size: 10px;"
        "  height: 18px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %7;"
        "  border-radius: 5px;"
        "}"
        /* ── ToolTip ── */
        "QToolTip {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "  font-size: 11px;"
        "}"
        /* ── ListWidget ── */
        "QListWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  outline: none;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:selected { background-color: %5%6; }"
        "QListWidget::item:hover { background-color: %5%6; }"
        /* ── DateEdit / TimeEdit ── */
        "QDateEdit, QTimeEdit {"
        "  background-color: %11;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 6px;"
        "  padding: 7px 12px;"
        "  font-size: 11px;"
        "}"
        "QDateEdit:focus, QTimeEdit:focus { border-color: %7; }"
        /* ── Dialog ── */
        "QDialog {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: 10px;"
        "}"
        /* ── Slider ── */
        "QSlider::groove:horizontal {"
        "  background: %4;"
        "  height: 4px;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: %7;"
        "  width: 14px;"
        "  height: 14px;"
        "  margin: -5px 0;"
        "  border-radius: 7px;"
        "}"
        "QSlider::handle:horizontal:hover { background: %9; }"
    ).arg(
        bg1, txt1, bg3, bdr, hovBg, hoverAlpha,
        acc, txt2, accH, accP, bgInp,
        sclBg, sclH
    );

    return sheet;
}