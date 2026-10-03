#include "tui/widgets.hpp"

#include <algorithm>
#include <cstdio>

namespace tui {

using namespace ftxui;

namespace {
bool continuation(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }
} // namespace

bool LineEdit::handle(const Event& e) {
	auto prev = [this] { std::size_t p = cursor; while (p > 0 && continuation(value[--p])) {} return p; };
	auto next = [this] { std::size_t p = cursor + 1; while (p < value.size() && continuation(value[p])) ++p; return p; };

	if (e.is_character()) {
		const std::string c = e.character();
		value.insert(cursor, c);
		cursor += c.size();
		return true;
	}
	if (e == Event::Backspace) {
		if (cursor > 0) { const std::size_t p = prev(); value.erase(p, cursor - p); cursor = p; }
		return true;
	}
	if (e == Event::Delete) {
		if (cursor < value.size()) value.erase(cursor, next() - cursor);
		return true;
	}
	if (e == Event::ArrowLeft) { if (cursor > 0) cursor = prev(); return true; }
	if (e == Event::ArrowRight) { if (cursor < value.size()) cursor = next(); return true; }
	if (e == Event::Home) { cursor = 0; return true; }
	if (e == Event::End) { cursor = value.size(); return true; }
	return false;
}

Element LineEdit::render(const Theme& t, const std::string& placeholder, bool active, int width) const {
	if (!active && value.empty())
		return fixed(t.ink(clip(placeholder, static_cast<std::size_t>(width), t.g().clip), Role::Ghost), width);

	// Keep the cursor in view: show the tail of a value longer than the field.
	std::string shown = value;
	std::size_t cur = cursor;
	const std::size_t w = static_cast<std::size_t>(std::max(1, width));
	if (shown.size() >= w) {
		std::size_t cut = shown.size() - w + 1;
		if (cur < cut) cut = cur;
		shown = shown.substr(cut);
		cur -= cut;
	}
	if (!active) return fixed(t.ink(shown, Role::Text), width);

	std::size_t end = cur + 1;
	while (end < shown.size() && continuation(shown[end])) ++end;
	const std::string at = cur < shown.size() ? shown.substr(cur, end - cur) : " ";
	const std::string after = cur < shown.size() ? shown.substr(end) : "";
	return fixed(hbox({ text(shown.substr(0, cur)), text(at) | inverted, text(after) }) | t.fg(Role::Text), width);
}

std::string repeat(const char* glyph, int n) {
	std::string s;
	for (int i = 0; i < n; ++i) s += glyph;
	return s;
}

std::string clip(const std::string& s, std::size_t width, const char* clipGlyph) {
	if (s.size() <= width) return s;
	if (width == 0) return {};
	return s.substr(0, width - 1) + clipGlyph;   // the clip glyph is one cell
}

std::string hex0x(uint64_t v, int digits) {
	char buf[24];
	std::snprintf(buf, sizeof(buf), "0x%0*llx", digits, static_cast<unsigned long long>(v));
	return buf;
}

std::string hexUpper(uint64_t v, int digits) {
	char buf[24];
	std::snprintf(buf, sizeof(buf), "%0*llX", digits, static_cast<unsigned long long>(v));
	return buf;
}

Element fixed(Element e, int width) {
	return e | size(WIDTH, EQUAL, width);
}

Element rule(const Theme& t) {
	return separatorCharacter(t.g().h) | t.fg(Role::Line);
}

Element scrollRule(const Theme& t, int height, ScrollState s) {
	if (height < 1) height = 1;
	int thumbTop = -1, thumbLen = 0;
	if (s.visible > 0 && s.total > s.visible) {
		thumbLen = std::max(1, static_cast<int>(static_cast<double>(height) * s.visible / s.total));
		const std::size_t range = s.total - s.visible;
		thumbTop = static_cast<int>(static_cast<double>(height - thumbLen) * std::min(s.top, range) / range);
	}
	Elements cells;
	cells.reserve(static_cast<std::size_t>(height));
	for (int y = 0; y < height; ++y) {
		const bool thumb = y >= thumbTop && y < thumbTop + thumbLen;
		cells.push_back(thumb ? t.ink(t.g().thumb, Role::Muted) : t.ink(t.g().v, Role::Line));
	}
	return vbox(std::move(cells)) | size(WIDTH, EQUAL, 1);
}

Element field(const Theme& t, Element inner, bool enabled) {
	const Role frame = enabled ? Role::Line : Role::Ghost;
	return hbox({ t.ink("[", frame), text(" "), std::move(inner), text(" "), t.ink("]", frame) });
}

} // namespace tui
