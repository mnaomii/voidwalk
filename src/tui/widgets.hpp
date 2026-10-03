#pragma once
#include "tui/theme/palette.hpp"

#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/box.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace tui {

// What a vertical rule needs to draw a pane's scroll thumb.
struct ScrollState {
	std::size_t total = 0;
	std::size_t top = 0;
	std::size_t visible = 0;
};

// The box a pane's row area was given on the previous frame (via ftxui::reflect).
// Panes size their row window from it, so they build exactly the rows on screen
// and know where the thumb goes. Before the first frame it reports `fallback`.
struct Viewport {
	ftxui::Box box;
	int rows(int fallback = 20) const {
		const int h = box.y_max - box.y_min + 1;
		return (box.y_max == 0 && box.y_min == 0) ? fallback : (h > 0 ? h : 1);
	}
	int cols(int fallback = 80) const {
		const int w = box.x_max - box.x_min + 1;
		return (box.x_max == 0 && box.x_min == 0) ? fallback : (w > 0 ? w : 1);
	}
};

// A one-line text field. UI-owned and drawn by hand, so a field takes keys only
// when UI routes them to it - the old FTXUI Input swallowed 'o'/'q' mid-typing.
class LineEdit {
public:
	std::string value;
	std::size_t cursor = 0;

	void set(std::string s) { value = std::move(s); cursor = value.size(); }
	bool handle(const ftxui::Event& e);   // true if the key edited or moved
	ftxui::Element render(const Theme& t, const std::string& placeholder, bool active, int width) const;
};

// Clamp `cursor` to [0, total) and slide `top` so the cursor stays inside a window
// of `height` rows that never runs past the end.
inline void follow(std::size_t& cursor, std::size_t& top, std::size_t total, int height) {
	const std::size_t h = static_cast<std::size_t>(height > 0 ? height : 1);
	if (total == 0) { cursor = top = 0; return; }
	if (cursor >= total) cursor = total - 1;
	if (cursor < top) top = cursor;
	if (cursor >= top + h) top = cursor - h + 1;
	const std::size_t maxTop = total > h ? total - h : 0;
	if (top > maxTop) top = maxTop;
}

// Up/Down/PageUp/PageDown/Home/End on a row cursor. True if the key was one of them.
inline bool moveRows(const ftxui::Event& e, std::size_t& cursor, std::size_t total, int height) {
	if (total == 0) return false;
	const std::size_t page = static_cast<std::size_t>(height > 1 ? height - 1 : 1);
	using ftxui::Event;
	if (e == Event::ArrowUp)   { if (cursor > 0) --cursor; return true; }
	if (e == Event::ArrowDown) { if (cursor + 1 < total) ++cursor; return true; }
	if (e == Event::PageUp)    { cursor = cursor > page ? cursor - page : 0; return true; }
	if (e == Event::PageDown)  { cursor = cursor + page < total ? cursor + page : total - 1; return true; }
	if (e == Event::Home)      { cursor = 0; return true; }
	if (e == Event::End)       { cursor = total - 1; return true; }
	return false;
}

// The row index under screen line `y`, if `y` falls inside the viewport.
inline bool rowAt(const Viewport& vp, int y, std::size_t top, std::size_t& row) {
	if (y < vp.box.y_min || y > vp.box.y_max) return false;
	row = top + static_cast<std::size_t>(y - vp.box.y_min);
	return true;
}

std::string repeat(const char* glyph, int n);
std::string clip(const std::string& s, std::size_t width, const char* clipGlyph);
std::string hex0x(uint64_t v, int digits);    // "0x0000beef"
std::string hexUpper(uint64_t v, int digits); // "0000BEEF"

ftxui::Element fixed(ftxui::Element e, int width);
ftxui::Element rule(const Theme& t);   // full-width hairline (inside a vbox)
ftxui::Element scrollRule(const Theme& t, int height, ScrollState s);
// "[ inner ]" - a QLineEdit/QComboBox frame in cells.
ftxui::Element field(const Theme& t, ftxui::Element inner, bool enabled = true);

} // namespace tui
