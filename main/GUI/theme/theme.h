#ifndef GUI_THEME_H
#define GUI_THEME_H

#include <QColor>
#include <QFont>
#include <QHash>
#include <QString>

namespace gui {

// The font every code-like pane uses (disassembly, memory, registers, stack).
// QFontDatabase::systemFont(FixedFont) resolves to Courier New on Windows, which
// renders thin and aliased at UI sizes; this picks the first real programming
// mono that exists on the host and asks for antialiased rendering. Size is left
// at the application default so it still scales with DPI.
QFont monoFont();

// Color definition for one theme (option "1b — modern IDE", dark + light).
// Every color the QSS template, the palette, the icons and the disassembly
// delegate use comes from here, so the two themes can never drift apart.
struct Theme {
	QString id; // "dark" | "light" — persisted in AppSettings::theme

	// Chrome
	QColor bgBase;        // deepest background: tables, editors, central pane
	QColor bgPanel;       // toolbar / menubar / dock chrome
	QColor border;        // hairline separators
	QColor control;       // buttons at rest
	QColor controlHover;
	QColor controlBorder;

	// Text
	QColor text;          // default foreground
	QColor textBright;    // headings, active tab, hovered controls
	QColor textMuted;     // menus, secondary labels
	QColor textDim;       // column headers, hex bytes
	QColor textFaint;     // addresses, disabled

	// Accent (gray-blue)
	QColor accent;        // primary accent: mnemonics, eip, focus borders
	QColor accentText;    // text on accentBg rows (current instruction)
	QColor accentBg;      // current-instruction / selection background
	QColor runBg;         // emphasized Run button background

	// Semantics
	QColor ok;            // "loaded" status dot, ASCII gutter

	// Disassembly syntax
	QColor synMnemonic;   // mov/push/add…
	QColor synJump;       // jmp/jcc/call/ret
	QColor synRegister;   // eax, ebp…
	QColor synImmediate;  // 0x8, -0x4
	QColor synTarget;     // jump/call target addresses
	QColor synPunct;      // commas, brackets, comments

	// QSS template substitution: every field above as "%name%" -> "#rrggbb".
	QHash<QString, QString> placeholderMap() const;

	static Theme dark();
	static Theme light();
	static Theme byId(const QString& id); // falls back to dark()
};

} // namespace gui

#endif
