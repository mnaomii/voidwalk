#ifndef GUI_ICONS_H
#define GUI_ICONS_H

#include <QColor>
#include <QIcon>
#include <QString>

namespace gui {

// Flat monochrome SVG icon set, recolored at runtime to match the theme.
// The SVGs in :/icons/ are drawn in #000000; icon() string-replaces that with
// the theme text color (textFaint for the Disabled state) and renders through
// QSvgRenderer at several device sizes. Requires the Qt6::Svg module.
//
// Usage:
//   Icons::setColors(theme.text, theme.textFaint);   // on every theme switch
//   action->setIcon(Icons::icon("run"));
class Icons {
public:
	static void setColors(const QColor& normal, const QColor& disabled);
	static QIcon icon(const QString& name); // name without path/extension, e.g. "step-into"

private:
	Icons() = delete;
};

} // namespace gui

#endif
