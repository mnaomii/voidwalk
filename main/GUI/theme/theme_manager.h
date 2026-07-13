#ifndef GUI_THEME_MANAGER_H
#define GUI_THEME_MANAGER_H

#include "theme.h"

namespace gui {

// Applies a Theme app-wide: forces the Fusion style (so the rendering is
// identical on Windows/macOS/Linux), fills a QPalette from the theme, and
// installs the QSS built from the stylesheet template. Call once at startup
// and again whenever the user switches themes in Settings.
class ThemeManager {
public:
	static void apply(const Theme& theme);
	static const Theme& current(); // last applied theme (dark() before any apply)

private:
	static QString buildStylesheet(const Theme& theme);
};

} // namespace gui

#endif
