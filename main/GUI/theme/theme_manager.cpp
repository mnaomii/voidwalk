#include "theme_manager.h"

#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyleFactory>
#include <QSvgRenderer>

namespace gui {

namespace {
Theme g_current = Theme::dark();

// Glyphs the QSS needs but Fusion stops drawing once we style the widgets: the
// checkbox tick, the spin/combo arrows, and the sidebar's disclosure triangles.
// Drawn with #000000 so we can recolor per theme exactly like the toolbar icons
// (Icons::icon), rendered to PNG and referenced from the QSS by file path.
const char kCheckSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><path d="M3.5 8.6 L6.6 11.5 L12.5 4.8" fill="none" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)SVG";
const char kChevronUpSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><path d="M4 9.8 L8 5.8 L12 9.8" fill="none" stroke="#000000" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg>)SVG";
const char kChevronDownSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><path d="M4 6.2 L8 10.2 L12 6.2" fill="none" stroke="#000000" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg>)SVG";
// Solid triangles for QTreeView::branch — smaller and quieter than the chevrons.
const char kTriRightSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><path d="M6 4 L11 8 L6 12 Z" fill="#000000"/></svg>)SVG";
const char kTriDownSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><path d="M4 6 L12 6 L8 11 Z" fill="#000000"/></svg>)SVG";
// Sidebar toggle: a panel with a rule where the sidebar sits.
const char kSidebarSvg[] =
	R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><rect x="1.7" y="2.7" width="12.6" height="10.6" rx="1.5" fill="none" stroke="#000000" stroke-width="1.4"/><rect x="5.6" y="2.7" width="1.4" height="10.6" fill="#000000"/></svg>)SVG";

// Recolor a glyph to `color`, cache it as a PNG under the temp dir, and return a
// QSS-friendly (forward-slash) path. Rendered at exactly `px` — the size the QSS
// draws it at. The filename keys on glyph + size + color so a theme switch
// reuses files. On failure returns an empty string; the QSS url() then simply
// draws nothing (no worse than the missing glyph before).
QString makeGlyph(const char* svg, const QColor& color, const QString& name, int px) {
	QByteArray data(svg);
	data.replace("#000000", color.name(QColor::HexRgb).toLatin1());

	QSvgRenderer renderer(data);
	QPixmap pm(px, px);
	pm.fill(Qt::transparent);
	QPainter painter(&pm);
	renderer.render(&painter);
	painter.end();

	const QString dir = QDir::tempPath() + QStringLiteral("/voidwalk-glyphs");
	QDir().mkpath(dir);
	const QString path = QStringLiteral("%1/%2-%3-%4.png")
		.arg(dir, name).arg(px).arg(color.name(QColor::HexRgb).mid(1)); // drop the '#'
	if (!pm.save(path, "PNG"))
		return QString();
	return QDir::fromNativeSeparators(path);
}

// The whole app skin, written against the Fusion style. %tokens% are replaced
// from Theme::placeholderMap(). Keep every color a token — no literals here.
//
// "Quiet chrome": toolbar buttons have no box at rest (the toolbar is a row of
// glyphs, not a row of chiclets) and only Run keeps a filled accent box; dock
// tabs are underlines rather than folder tabs; every separator is one hairline
// in %border%. The symbol sidebar sits one step off the editor background so
// the eye reads three planes — sidebar, code, docks — without extra rules.
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

/* ---- toolbar: flat by default, hover reveals the target ---- */
QToolBar { background: %bgPanel%; border: none; border-bottom: 1px solid %border%; padding: 7px 8px; spacing: 2px; }
QToolBar::separator { width: 1px; background: %controlBorder%; margin: 5px 8px; }
QToolButton {
	background: transparent; border: 1px solid transparent; border-radius: 6px;
	padding: 5px 10px; color: %text%;
}
QToolButton:hover { background: %control%; color: %textBright%; }
QToolButton:pressed { background: %accentBg%; }
QToolButton:checked { background: %control%; color: %textBright%; }
QToolButton:disabled { color: %textGhost%; background: transparent; border-color: transparent; }
/* The one emphasized action. */
QToolButton#runButton { background: %runBg%; border-color: %accent%; color: %accentText%; padding: 5px 12px; }
QToolButton#runButton:hover { background: %accentBg%; }
QToolButton#runButton:disabled { background: transparent; border-color: %border%; color: %textGhost%; }
QToolButton#settingsButton, QToolButton#sidebarButton { padding: 6px; }

/* Toolbar address field: sits right-aligned, quiet until focused. */
QLineEdit#gotoField {
	background: %bgBase%; border: 1px solid %controlBorder%; border-radius: 6px;
	padding: 4px 9px; min-width: 170px; color: %text%;
}
QLineEdit#gotoField:focus { border-color: %accent%; }

/* ---- docks ---- */
QDockWidget { color: %textMuted%; titlebar-close-icon: none; titlebar-normal-icon: none; }
QDockWidget::title { background: %bgBase%; padding: 8px 12px; border-bottom: 1px solid %border%; text-align: left; }
QMainWindow::separator { background: %border%; width: 1px; height: 1px; }
QMainWindow::separator:hover { background: %accent%; }

/* ---- dock tab bar: underline, not folder tabs ---- */
QTabBar { background: transparent; qproperty-drawBase: 0; }
QTabBar::tab {
	background: transparent; color: %textDim%;
	padding: 8px 10px; margin: 0 2px 0 0;
	border: none; border-bottom: 2px solid transparent;
}
QTabBar::tab:selected { color: %textBright%; border-bottom-color: %accent%; }
QTabBar::tab:hover:!selected { color: %textMuted%; }

/* ---- item views ---- */
QTableWidget, QTableView, QTreeWidget, QTreeView {
	background: %bgBase%;
	alternate-background-color: %bgBase%;
	border: none;
	gridline-color: transparent;
	selection-background-color: %accentBg%;
	selection-color: %textBright%;
}
QTableView::item, QTreeView::item { padding: 2px 8px; border: none; }
QTableView::item:selected, QTreeView::item:selected { background: %accentBg%; color: %textBright%; }
QHeaderView { background: %bgBase%; }
QHeaderView::section {
	background: %bgBase%; color: %textFaint%;
	border: none; border-bottom: 1px solid %border%;
	padding: 7px 10px; font-size: 10px; letter-spacing: 1.2px;
}
QTableCornerButton::section { background: %bgBase%; border: none; }

/* ---- symbol sidebar ---- */
QWidget#symbolsPane { background: %bgSidebar%; border-right: 1px solid %border%; }
QLabel#symbolsHeader {
	color: %textFaint%; font-size: 10px; letter-spacing: 1.2px;
	padding: 8px 12px 0 12px;
}
QLabel#symbolsCount { color: %textGhost%; font-size: 10px; padding: 8px 12px 0 0; }
QLineEdit#symbolsFilter {
	background: %bgBase%; border: 1px solid %controlBorder%; border-radius: 5px;
	padding: 4px 8px; margin: 8px 10px; color: %text%; font-size: 11px;
}
QLineEdit#symbolsFilter:focus { border-color: %accent%; }
QTreeWidget#symbolsTree {
	background: %bgSidebar%; border: none;
	show-decoration-selected: 1;
}
QTreeWidget#symbolsTree::item { padding: 3px 6px; }
QTreeWidget#symbolsTree::item:hover:!selected { background: %bgBase%; }
QTreeWidget#symbolsTree::branch { background: transparent; }
QTreeWidget#symbolsTree::branch:has-children:closed { image: url("%triRight%"); }
QTreeWidget#symbolsTree::branch:has-children:open { image: url("%triDown%"); }

/* ---- text / inputs ---- */
QPlainTextEdit, QTextEdit { background: %bgBase%; border: 1px solid %border%; border-radius: 6px; color: %text%; selection-background-color: %accentBg%; selection-color: %textBright%; }
QLineEdit, QSpinBox, QComboBox { background: %bgBase%; border: 1px solid %controlBorder%; border-radius: 5px; padding: 4px 8px; color: %text%; selection-background-color: %accentBg%; selection-color: %textBright%; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus, QTextEdit:focus { border-color: %accent%; }
QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled { color: %textFaint%; background: %bgPanel%; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow { image: url("%downGlyph%"); width: 12px; height: 12px; }
QComboBox QAbstractItemView { background: %bgPanel%; border: 1px solid %controlBorder%; border-radius: 6px; selection-background-color: %accentBg%; selection-color: %textBright%; }

/* Section chips in the memory pane's header row. */
QToolButton#sectionChip {
	background: transparent; border: none; border-radius: 5px;
	padding: 3px 9px; color: %textDim%; font-size: 11px;
}
QToolButton#sectionChip:hover { background: %control%; color: %text%; }
QToolButton#sectionChip:checked { background: %accentBg%; color: %accentText%; }

/* Spin buttons: Fusion stops drawing the arrows once the box is styled, so give
   them explicit sub-control geometry and glyph images. */
QSpinBox::up-button, QSpinBox::down-button {
	subcontrol-origin: border; width: 17px; background: transparent;
	border-left: 1px solid %controlBorder%;
}
QSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: 5px; }
QSpinBox::down-button { subcontrol-position: bottom right; border-top: 1px solid %controlBorder%; border-bottom-right-radius: 5px; }
QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: %control%; }
QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background: %accentBg%; }
QSpinBox::up-arrow { image: url("%upGlyph%"); width: 12px; height: 12px; }
QSpinBox::down-arrow { image: url("%downGlyph%"); width: 12px; height: 12px; }
QSpinBox::up-button:disabled, QSpinBox::down-button:disabled { border-color: %border%; }

/* ---- buttons / checkboxes / group boxes ---- */
QPushButton { background: %control%; border: 1px solid %controlBorder%; border-radius: 6px; padding: 6px 16px; color: %text%; }
QPushButton:hover { background: %controlHover%; color: %textBright%; }
QPushButton:pressed { background: %accentBg%; }
QPushButton:default { background: %runBg%; border-color: %accent%; color: %accentText%; }
QPushButton:disabled { color: %textGhost%; background: transparent; }
QPushButton:flat { background: transparent; border-color: transparent; }
QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %controlBorder%; border-radius: 4px; background: %bgBase%; }
QCheckBox::indicator:hover { border-color: %accent%; }
QCheckBox::indicator:checked { background: %accentBg%; border-color: %accent%; image: url("%checkGlyph%"); }
QGroupBox { border: 1px solid %border%; border-radius: 8px; margin-top: 14px; padding-top: 10px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; color: %textFaint%; font-size: 10px; letter-spacing: 1.2px; }

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
QStatusBar::item { border: none; }
QStatusBar QLabel { color: %textDim%; padding: 0 12px; border-left: 1px solid %border%; }
QStatusBar QLabel#archLabel { color: %accent%; }
QToolTip { background: %bgPanel%; color: %text%; border: 1px solid %controlBorder%; padding: 4px 8px; }

/* ---- welcome (empty state) ---- */
QLabel#welcomeBadge { background: %accentBg%; border: 1px solid %accent%; border-radius: 16px; color: %accent%; font-size: 22px; font-weight: 700; }
QLabel#welcomeTitle { color: %textBright%; font-size: 20px; font-weight: 600; }
QLabel#welcomeHint { color: %textMuted%; font-size: 14px; }
QLabel#welcomeChip { background: transparent; border: 1px solid %controlBorder%; border-radius: 12px; color: %textDim%; padding: 4px 12px; font-size: 12px; }
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
	p.setColor(QPalette::PlaceholderText, theme.textGhost);
	p.setColor(QPalette::Button, theme.control);
	p.setColor(QPalette::ButtonText, theme.text);
	p.setColor(QPalette::BrightText, theme.textBright);
	p.setColor(QPalette::Mid, theme.textMuted); // the panes' "color: palette(mid)" banners
	p.setColor(QPalette::Highlight, theme.accentBg);
	p.setColor(QPalette::HighlightedText, theme.textBright);
	p.setColor(QPalette::Link, theme.accent);
	p.setColor(QPalette::ToolTipBase, theme.bgPanel);
	p.setColor(QPalette::ToolTipText, theme.text);
	p.setColor(QPalette::Disabled, QPalette::Text, theme.textGhost);
	p.setColor(QPalette::Disabled, QPalette::ButtonText, theme.textGhost);
	p.setColor(QPalette::Disabled, QPalette::WindowText, theme.textGhost);
	qApp->setPalette(p);

	qApp->setStyleSheet(buildStylesheet(theme));
}

const Theme& ThemeManager::current() {
	return g_current;
}

QIcon ThemeManager::sidebarIcon(const Theme& theme) {
	const QString path = makeGlyph(kSidebarSvg, theme.text, QStringLiteral("sidebar"), 16);
	return path.isEmpty() ? QIcon() : QIcon(path);
}

QString ThemeManager::buildStylesheet(const Theme& theme) {
	QString qss = QString::fromLatin1(kQss);
	const auto map = theme.placeholderMap();
	for (auto it = map.constBegin(); it != map.constEnd(); ++it)
		qss.replace(QLatin1Char('%') + it.key() + QLatin1Char('%'), it.value());

	// Glyph images can't come from the color map (they're recolored PNGs, not hex
	// strings), so splice their paths in after: the tick in the accent color, the
	// arrows in the body text color, the sidebar triangles in the faintest text
	// so a collapsed group reads as structure rather than as a control.
	qss.replace(QStringLiteral("%checkGlyph%"), makeGlyph(kCheckSvg, theme.accent, QStringLiteral("check"), 16));
	qss.replace(QStringLiteral("%upGlyph%"), makeGlyph(kChevronUpSvg, theme.text, QStringLiteral("up"), 12));
	qss.replace(QStringLiteral("%downGlyph%"), makeGlyph(kChevronDownSvg, theme.text, QStringLiteral("down"), 12));
	qss.replace(QStringLiteral("%triRight%"), makeGlyph(kTriRightSvg, theme.textGhost, QStringLiteral("tri-r"), 12));
	qss.replace(QStringLiteral("%triDown%"), makeGlyph(kTriDownSvg, theme.textGhost, QStringLiteral("tri-d"), 12));
	return qss;
}

} // namespace gui
