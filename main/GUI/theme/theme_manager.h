#ifndef GUI_THEME_MANAGER_H
#define GUI_THEME_MANAGER_H

#include "theme.h"

#include <QIcon>
#include <QString>

namespace gui {

// Applies a Theme to the whole application: Fusion style + QPalette + the QSS
// skin built from Theme::placeholderMap(). Call apply() before the first widget
// paints and again whenever the theme changes in Settings.
class ThemeManager {
public:
	static void apply(const Theme& theme);
	static const Theme& current();

	// The symbol-sidebar toggle glyph, recolored for `theme`. Lives here rather
	// than in :/icons because it is chrome, not an action icon, and it needs the
	// same recolor path as the QSS glyphs.
	static QIcon sidebarIcon(const Theme& theme);

	static QString buildStylesheet(const Theme& theme);
};

} // namespace gui

#endif
