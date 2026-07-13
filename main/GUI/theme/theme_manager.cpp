#include "theme_manager.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace gui {

namespace {
Theme g_current = Theme::dark();

// The whole app skin, written against the Fusion style. %tokens% are replaced
// from Theme::placeholderMap(). Keep every color a token — no literals here.
const char kQss[] = R"QSS(
* { outline: none; }

QMainWindow, QDialog { background: %bgBase%; }
QWidget { color: %text%; }

/* ---- menu bar / menus ---- */
QMenuBar { background: %bgPanel%; border-bottom: 1px solid %border%; padding: 2px 6px; }
QMenuBar::item { padding: 4px 9px; background: transparent; border-radius: 4px; color: %textMuted%; }
QMenuBar::item:selected { background: %control%; color: %textBright%; }
QMenu { background: %bgPanel%; border: 1px solid %controlBorder%; border-radius: 6px; padding: 4px; }
QMenu::item { padding: 5px 26px 5px 12px; border-radius: 4px; color: %text%; }
QMenu::item:selected { background: %accentBg%; color: %textBright%; }
QMenu::item:disabled { color: %textFaint%; }
QMenu::separator { height: 1px; background: %border%; margin: 4px 8px; }

/* ---- toolbar ---- */
QToolBar { background: %bgPanel%; border: none; border-bottom: 1px solid %border%; padding: 6px 8px; spacing: 8px; }
QToolBar::separator { width: 1px; background: %border%; margin: 5px 4px; }
QToolButton { background: %control%; border: 1px solid %controlBorder%; border-radius: 6px; padding: 5px 10px; color: %text%; }
QToolButton:hover { background: %controlHover%; color: %textBright%; }
QToolButton:pressed { background: %accentBg%; }
QToolButton:disabled { color: %textFaint%; background: transparent; border-color: %border%; }
QToolButton#runButton { background: %runBg%; border-color: %accent%; color: %accentText%; }
QToolButton#runButton:disabled { background: transparent; border-color: %border%; color: %textFaint%; }
QToolButton#settingsButton { background: transparent; border: none; }
QToolButton#settingsButton:hover { background: %control%; }

/* ---- docks ---- */
QDockWidget { color: %textMuted%; titlebar-close-icon: none; titlebar-normal-icon: none; }
QDockWidget::title { background: %bgPanel%; padding: 6px 12px; border-bottom: 1px solid %border%; text-align: left; }
QMainWindow::separator { background: %border%; width: 1px; height: 1px; }
QMainWindow::separator:hover { background: %accent%; }

/* ---- dock tab bar ---- */
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: %textMuted%; padding: 6px 12px; border: 1px solid transparent; border-radius: 6px 6px 0 0; margin-right: 2px; }
QTabBar::tab:selected { background: %bgBase%; color: %textBright%; border-color: %border%; }
QTabBar::tab:hover:!selected { color: %textBright%; }

/* ---- item views ---- */
QTableWidget, QTableView {
	background: %bgBase%;
	alternate-background-color: %bgBase%;
	border: none;
	gridline-color: transparent;
	selection-background-color: %accentBg%;
	selection-color: %textBright%;
}
QTableView::item { padding: 2px 8px; }
QHeaderView::section {
	background: %bgBase%; color: %textDim%;
	border: none; border-bottom: 1px solid %border%;
	padding: 6px 10px; font-size: 11px; letter-spacing: 1px;
}
QTableCornerButton::section { background: %bgBase%; border: none; }

/* ---- text / inputs ---- */
QPlainTextEdit, QTextEdit { background: %bgBase%; border: 1px solid %border%; border-radius: 6px; color: %text%; selection-background-color: %accentBg%; selection-color: %textBright%; }
QLineEdit, QSpinBox, QComboBox { background: %bgBase%; border: 1px solid %controlBorder%; border-radius: 5px; padding: 4px 8px; color: %text%; selection-background-color: %accentBg%; selection-color: %textBright%; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus, QTextEdit:focus { border-color: %accent%; }
QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled { color: %textFaint%; background: %bgPanel%; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: %bgPanel%; border: 1px solid %controlBorder%; border-radius: 6px; selection-background-color: %accentBg%; selection-color: %textBright%; }

/* ---- buttons / checkboxes / group boxes ---- */
QPushButton { background: %control%; border: 1px solid %controlBorder%; border-radius: 6px; padding: 6px 16px; color: %text%; }
QPushButton:hover { background: %controlHover%; color: %textBright%; }
QPushButton:pressed { background: %accentBg%; }
QPushButton:default { background: %runBg%; border-color: %accent%; color: %accentText%; }
QPushButton:disabled { color: %textFaint%; background: transparent; }
QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %controlBorder%; border-radius: 4px; background: %bgBase%; }
QCheckBox::indicator:checked { background: %accent%; border-color: %accent%; }
QGroupBox { border: 1px solid %border%; border-radius: 8px; margin-top: 12px; padding-top: 8px; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: %textMuted%; }

/* ---- scrollbars ---- */
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %controlBorder%; border-radius: 4px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: %textFaint%; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: %controlBorder%; border-radius: 4px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: %textFaint%; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* ---- status bar / tooltips ---- */
QStatusBar { background: %bgPanel%; border-top: 1px solid %border%; color: %textMuted%; }
QStatusBar QLabel { color: %accent%; }
QToolTip { background: %bgPanel%; color: %text%; border: 1px solid %controlBorder%; padding: 4px 8px; }

/* ---- welcome (empty state) ---- */
QLabel#welcomeBadge { background: %accentBg%; border: 1px solid %accent%; border-radius: 16px; color: %accent%; font-size: 22px; font-weight: 700; }
QLabel#welcomeTitle { color: %textBright%; font-size: 20px; font-weight: 600; }
QLabel#welcomeHint { color: %textMuted%; font-size: 14px; }
QLabel#welcomeChip { background: %control%; border: 1px solid %controlBorder%; border-radius: 12px; color: %textDim%; padding: 4px 12px; font-size: 12px; }
)QSS";
} // namespace

void ThemeManager::apply(const Theme& theme) {
	g_current = theme;
	QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

	QPalette p;
	p.setColor(QPalette::Window, theme.bgPanel);
	p.setColor(QPalette::WindowText, theme.text);
	p.setColor(QPalette::Base, theme.bgBase);
	p.setColor(QPalette::AlternateBase, theme.bgPanel);
	p.setColor(QPalette::Text, theme.text);
	p.setColor(QPalette::PlaceholderText, theme.textFaint);
	p.setColor(QPalette::Button, theme.control);
	p.setColor(QPalette::ButtonText, theme.text);
	p.setColor(QPalette::BrightText, theme.textBright);
	p.setColor(QPalette::Mid, theme.textMuted); // the panes' "color: palette(mid)" banners
	p.setColor(QPalette::Highlight, theme.accentBg);
	p.setColor(QPalette::HighlightedText, theme.textBright);
	p.setColor(QPalette::Link, theme.accent);
	p.setColor(QPalette::ToolTipBase, theme.bgPanel);
	p.setColor(QPalette::ToolTipText, theme.text);
	p.setColor(QPalette::Disabled, QPalette::Text, theme.textFaint);
	p.setColor(QPalette::Disabled, QPalette::ButtonText, theme.textFaint);
	p.setColor(QPalette::Disabled, QPalette::WindowText, theme.textFaint);
	qApp->setPalette(p);

	qApp->setStyleSheet(buildStylesheet(theme));
}

const Theme& ThemeManager::current() {
	return g_current;
}

QString ThemeManager::buildStylesheet(const Theme& theme) {
	QString qss = QString::fromLatin1(kQss);
	const auto map = theme.placeholderMap();
	for (auto it = map.constBegin(); it != map.constEnd(); ++it)
		qss.replace(QLatin1Char('%') + it.key() + QLatin1Char('%'), it.value());
	return qss;
}

} // namespace gui
