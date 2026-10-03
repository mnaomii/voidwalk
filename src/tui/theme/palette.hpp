#pragma once

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace tui {

// The gui::Theme roles, one-to-one and in the same order as Theme::placeholderMap().
// The GUI turns a role into a QColor; the TUI turns it into whatever the terminal
// can draw. Panes name roles, never colours, so the two frontends cannot drift:
// a role that changes meaning changes in both.
enum class Role : uint8_t {
	Base, Panel, Sidebar, Control, Line,
	Text, Bright, Muted, Dim, Faint, Ghost,
	Accent, AccentText, AccentBg, RunBg,
	Ok, Breakpoint,
	SynMnemonic, SynJump, SynRegister, SynImmediate, SynTarget, SynString, SynPunct,
};
inline constexpr std::size_t kRoleCount = static_cast<std::size_t>(Role::SynPunct) + 1;

// Every non-ASCII glyph the TUI draws, so a tier that cannot show one swaps the
// whole set rather than individual call sites guessing.
struct Glyphs {
	const char* h;          // horizontal rule
	const char* v;          // vertical rule
	const char* tabRule;    // under the active dock tab
	const char* open;       // tree group, expanded
	const char* closed;     // tree group, collapsed
	const char* run;        // Run button
	const char* breakpoint; // gutter marker
	const char* clip;       // a cell that ran out of room (one column)
	const char* ellipsis;   // "..." in copy: Open..., Jump to...
	const char* sidebar;    // sidebar toggle
	const char* drop;       // combo arrow
	const char* dot;        // "ELF · x86-64"
	const char* thumb;      // scroll thumb inside a vertical rule
	const char* dash;       // em dash in copy
	const char* arrows;     // up/down hint
	const char* check;      // checked menu item
};

// 16-colour ANSI with CP437 glyphs. Runs on the Linux VT, Windows
// conhost and tmux/screen without RGB. Only blue (bg 4) is ever used as a fill:
// every 16-colour terminal can draw it, and it is what carries selection and the
// one primary action (QToolButton#runButton / QPushButton:default in the QSS).
class Theme {
public:
	struct Ink {
		ftxui::Color color = ftxui::Color::Default;
		bool bold = false;
		bool underline = false;
	};

	static const Theme& ansi16() {
		static const Theme t = [] {
			using C = ftxui::Color;
			Theme th;
			auto set = [&th](Role r, C c, bool b = false, bool u = false) {
				th.fg_[idx(r)] = Ink{c, b, u};
			};
			set(Role::Text, C::Default);
			// Default, not White: white text vanishes on a light terminal background.
			set(Role::Bright, C::Default, true);
			set(Role::Muted, C::Default);
			set(Role::Dim, C::GrayDark);
			set(Role::Faint, C::GrayDark);
			set(Role::Ghost, C::GrayDark);
			set(Role::Line, C::GrayDark);
			set(Role::Accent, C::BlueLight);
			set(Role::AccentText, C::White, true);
			set(Role::Ok, C::Green);
			// synJump and breakpoint share a hex value in Theme::dark(); keep them together.
			set(Role::Breakpoint, C::RedLight);
			set(Role::SynMnemonic, C::BlueLight);
			set(Role::SynJump, C::RedLight);
			set(Role::SynRegister, C::Cyan);
			set(Role::SynImmediate, C::Yellow);
			set(Role::SynTarget, C::Green);
			set(Role::SynString, C::Green);
			set(Role::SynPunct, C::GrayDark);
			for (auto& c : th.bg_) c = C::Default;   // planes are drawn with rules, not fills
			th.bg_[idx(Role::AccentBg)] = C::Blue;
			th.bg_[idx(Role::RunBg)] = C::Blue;
			th.glyphs_ = Glyphs{
				"─", "│", "═", "▼", "►", "►", "•", "»", "...", "≡", "▼", "∙", "█", "-", "↑↓", "√",
			};
			return th;
		}();
		return t;
	}

	const Glyphs& g() const { return glyphs_; }
	ftxui::Color colorOf(Role r) const { return fg_[idx(r)].color; }

	// Foreground colour plus the role's attributes (bold for Bright, etc).
	ftxui::Decorator fg(Role r) const {
		const Ink ink = fg_[idx(r)];
		return [ink](ftxui::Element e) {
			e = e | ftxui::color(ink.color);
			if (ink.bold) e = e | ftxui::bold;
			if (ink.underline) e = e | ftxui::underlined;
			return e;
		};
	}

	// Background fill, or nothing at all for a role this tier does not fill.
	ftxui::Decorator bg(Role r) const {
		const ftxui::Color c = bg_[idx(r)];
		if (c == ftxui::Color(ftxui::Color::Default)) return ftxui::nothing;
		return ftxui::bgcolor(c);
	}

	ftxui::Element ink(std::string s, Role r) const { return ftxui::text(std::move(s)) | fg(r); }

private:
	static constexpr std::size_t idx(Role r) { return static_cast<std::size_t>(r); }
	std::array<Ink, kRoleCount> fg_{};
	std::array<ftxui::Color, kRoleCount> bg_{};
	Glyphs glyphs_{};
};

} // namespace tui
