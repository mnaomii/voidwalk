#ifndef GUI_THEME_H
#define GUI_THEME_H

#include <QColor>
#include <QFont>
#include <QHash>
#include <QString>

namespace gui {

// The font every code-like pane uses (disassembly, memory, registers, stack,
// symbols). Prefers a real programming mono with a tall x-height and
// unambiguous 0/O/1/l; the bundled JetBrains Mono (:/fonts, registered by
// registerBundledFonts()) guarantees the same rendering on every host instead
// of falling through to Courier New on Windows. Size is left at the application
// default so it still scales with DPI.
QFont monoFont();

// Registers the fonts under :/fonts with the application font database. Call
// once from main() BEFORE the first widget is constructed; monoFont() asks for
// the bundled family first and only falls back to host fonts if this was never
// called (or the resource is missing).
void registerBundledFonts();

// Color definition for one theme ("quiet chrome + symbols", dark + light).
// Every color the QSS template, the palette, the icons, the symbol sidebar and
// the disassembly delegate use comes from here, so the two themes can never
// drift apart.
struct Theme {
	QString id; // "dark" | "light" — persisted in AppSettings::theme

	// Chrome
	QColor bgBase;        // deepest background: tables, editors, central pane
	QColor bgPanel;       // toolbar / menubar / dock chrome / status bar
	QColor bgSidebar;     // symbol sidebar body — one step off bgBase
	QColor border;        // hairline separators
	QColor control;       // buttons on hover (quiet chrome: no rest background)
	QColor controlHover;
	QColor controlBorder;

	// Text
	QColor text;          // default foreground
	QColor textBright;    // headings, active tab, hovered controls
	QColor textMuted;     // menus, secondary labels, ptr keywords
	QColor textDim;       // addresses in the disassembly, symbol addresses
	QColor textFaint;     // column headers, hex bytes, disabled
	QColor textGhost;     // notes column, section headers in the sidebar

	// Accent (gray-blue)
	QColor accent;        // primary accent: eip, focus borders, tab underline
	QColor accentText;    // text on accentBg rows (current instruction)
	QColor accentBg;      // current-instruction / selection background
	QColor runBg;         // emphasized Run button background

	// Semantics
	QColor ok;            // "loaded" status dot
	QColor breakpoint;    // breakpoint marker in the gutter

	// Disassembly syntax
	QColor synMnemonic;   // mov/push/add…
	QColor synJump;       // jmp/jcc/call/ret
	QColor synRegister;   // eax, ebp…
	QColor synImmediate;  // 0x8, -0x4
	QColor synTarget;     // jump/call targets and resolved symbol names
	QColor synString;     // string literals (sidebar, memory ASCII gutter)
	QColor synPunct;      // commas, brackets, comments

	// QSS template substitution: every field above as "%name%" -> "#rrggbb".
	QHash<QString, QString> placeholderMap() const;

	static Theme dark();
	static Theme light();
	static Theme byId(const QString& id); // falls back to dark()
};

} // namespace gui

#endif
