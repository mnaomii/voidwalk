#include "tui/ui.hpp"

#include "tui/panes/panes.hpp"
#include "tui/theme/palette.hpp"
#include "tui/widgets.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/terminal.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace tui {

using namespace ftxui;
namespace fs = std::filesystem;

namespace {

struct MenuItem {
	std::string label;
	std::string shortcut;
	std::function<void()> run;
	std::function<bool()> enabled;   // null: always enabled
	std::function<bool()> checked;   // null: not checkable
	bool separator = false;
};

struct Menu {
	std::string title;
	std::vector<MenuItem> items;
};

struct Completion {
	std::string path;
	bool dir = false;
	uintmax_t size = 0;
};

MenuItem sep() { MenuItem m; m.separator = true; return m; }

Decorator when(bool on, Decorator d) { return on ? d : Decorator(nothing); }

std::string lower(std::string s) {
	for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

std::string trim(const std::string& s) {
	const auto a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return {};
	return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

// A dropped file arrives as pasted text, often quoted ('/path/with space').
std::string unquote(const std::string& raw) {
	std::string s = trim(raw);
	if (s.size() >= 2 && (s.front() == '\'' || s.front() == '"') && s.back() == s.front())
		s = s.substr(1, s.size() - 2);
	return s;
}

std::string expandHome(const std::string& p) {
	if (p == "~" || p.rfind("~/", 0) == 0) {
		const char* home = std::getenv("HOME");
		if (!home) home = std::getenv("USERPROFILE");
		if (home) return std::string(home) + p.substr(1);
	}
	return p;
}

std::string humanSize(uintmax_t b) {
	if (b < 1024) return std::to_string(b) + " B";
	return std::to_string((b + 1023) / 1024) + " KiB";
}

// The Qt dialog's "Executables (*.exe *.dll *.so *.elf *.bin)" filter, widened by
// one rule so it is usable on Unix: extensionless files with the exec bit.
bool looksExecutable(const fs::path& p) {
	const std::string ext = lower(p.extension().string());
	if (ext == ".exe" || ext == ".dll" || ext == ".so" || ext == ".elf" || ext == ".bin") return true;
	if (p.filename().string().find(".so.") != std::string::npos) return true;
	std::error_code ec;
	const auto st = fs::status(p, ec);
	if (ec) return false;
	return ext.empty() && (st.permissions() & fs::perms::owner_exec) != fs::perms::none;
}

int altMenu(const Event& e) {
	if (e == Event::AltF) return 0;
	if (e == Event::AltD) return 1;
	if (e == Event::AltE) return 2;
	if (e == Event::AltV) return 3;
	if (e == Event::AltT) return 4;
	return -1;
}

} // namespace

// The whole screen as one component. UI-owned focus (see panes.hpp) means every key
// passes through OnEvent in a fixed order: modal, menu, toolbar field, a pane that
// is capturing text, global shortcuts, then the focused pane.
class AppView : public ComponentBase {
public:
	AppView(Session& session, ScreenInteractive& screen)
		: s_(session), screen_(screen), t_(Theme::ansi16()),
		  disasm_(session, t_), symbols_(session, t_), regs_(session, t_),
		  stack_(session, t_), memory_(session, t_) {
		symbols_.onNavigate = [this](uint64_t vaddr) {
			disasm_.navigateTo(vaddr);
			focus_ = Focus::Disassembly;
			s_.setStatus("Jumped to " + hex0x(vaddr, 8));
		};
		symbols_.onMemory = [this](uint64_t offset) {
			memoryOn_ = true;
			memory_.gotoOffset(offset);
			s_.setStatus("Showing file offset " + hex0x(offset, 8));
		};
		buildMenus();
	}

	bool Focusable() const override { return true; }
	bool ticking() const { return ticking_.load(std::memory_order_relaxed); }

	Element OnRender() override {
		// decode() runs on a worker: re-derive the feeds each frame while it fills in,
		// plus once after it finishes so the final rows and any note land.
		if (s_.isDecoding() || refreshedGen_ != s_.decodeGeneration()) {
			const bool done = !s_.isDecoding();
			s_.refresh();
			if (done) refreshedGen_ = s_.decodeGeneration();
		}
		ticking_.store(refreshedGen_ != s_.decodeGeneration(), std::memory_order_relaxed);
		validateFocus();
		hits_ = Hits{};   // anything not drawn this frame cannot be clicked

		Elements rows = { menuBar(), toolBar(), rule(t_), body() | yflex };
		if (memoryOn_) {
			rows.push_back(rule(t_));
			rows.push_back(memory_.render(focus_ == Focus::Memory) | size(HEIGHT, EQUAL, 9) | reflect(hits_.pane[idx(Focus::Memory)]));
		}
		rows.push_back(rule(t_));
		rows.push_back(statusBar());
		Element screen = vbox(std::move(rows));

		if (menuOpen_ >= 0) screen = dbox({ screen, menuOverlay() });
		if (showOpen_) screen = dbox({ screen | dim, openDialog() | clear_under | center });
		return screen;
	}

	bool OnEvent(Event e) override {
		if (e == Event::Custom) return false;
		if (e.is_mouse()) return onMouse(e.mouse());
		if (showOpen_) return onOpenEvent(e);
		if (menuOpen_ >= 0) return onMenuEvent(e);
		if (gotoActive_) {
			if (e == Event::Escape) { gotoActive_ = false; return true; }
			if (e == Event::Return) { submitGoto(); return true; }
			goto_.handle(e);
			return true;
		}
		if (Pane* p = focusedPane(); p && p->capturing()) return p->onEvent(e);

		// --- shortcuts: the Qt set, plus single-key aliases the old TUI had ---
		if (e == Event::CtrlO || e == Event::Character('o')) { beginOpen(""); return true; }
		if (e == Event::CtrlQ || e == Event::Character('q')) { screen_.ExitLoopClosure()(); return true; }
		if (e == Event::F5) { stub("Run"); return true; }
		if (e == Event::F7) { stub("Step Into"); return true; }
		if (e == Event::F8) { stub("Step Over"); return true; }
		if (e == Event::F9) { stub("Continue"); return true; }
		if (e == Event::CtrlB) { sidebarOn_ = !sidebarOn_; return true; }
		if (e == Event::F10) { openMenu(0); return true; }
		if (const int m = altMenu(e); m >= 0) { openMenu(m); return true; }
		if (e == Event::Tab) { cycleFocus(+1); return true; }
		if (e == Event::TabReverse) { cycleFocus(-1); return true; }
		if (e == Event::Character('[') || e == Event::Character(']')) { switchTab(); return true; }
		if (focus_ == Focus::MenuBar && onMenuBarEvent(e)) return true;
		if (focus_ == Focus::Toolbar && onToolbarEvent(e)) return true;

		if (!s_.loaded()) {
			// Empty state: Enter is the "Open binary..." button. Any other typed text
			// is most likely a path the terminal pasted for a dropped file, so it
			// opens the dialog with that text already in the field.
			if (e == Event::Return) { beginOpen(""); return true; }
			if (e.is_character()) { beginOpen(e.character()); return true; }
			return false;
		}

		if (e == Event::CtrlG || e == Event::Character('g')) { gotoActive_ = true; goto_.set(""); return true; }
		if (e == Event::Character('/') && sidebarOn_) {
			focus_ = Focus::Symbols;
			symbols_.beginFilter();
			return true;
		}
		if (Pane* p = focusedPane()) return p->onEvent(e);
		return false;
	}

private:
	enum class Focus { MenuBar, Toolbar, Symbols, Disassembly, Right, Memory };
	enum class Tab { Registers, Stack };
	// Toolbar buttons, left to right.
	enum Tool { TSidebar, TOpen, TRun, TStepInto, TStepOver, TContinue, TPause, TReset, TRecompile, TGoto, TSettings, kTools };
	static constexpr size_t idx(Focus f) { return static_cast<size_t>(f); }
	static constexpr size_t idx(Tab t) { return static_cast<size_t>(t); }

	// Where clickable things were drawn on the last frame (filled by ftxui::reflect).
	struct Hits {
		static constexpr Box kNone{-1, -1, -1, -1};
		Box menu[5]{kNone, kNone, kNone, kNone, kNone}, drop = kNone;
		Box tool[kTools]{kNone, kNone, kNone, kNone, kNone, kNone, kNone, kNone, kNone, kNone, kNone};
		Box pane[6]{kNone, kNone, kNone, kNone, kNone, kNone}, tab[2]{kNone, kNone};
		Box welcome = kNone, comps = kNone, openBtn = kNone, cancelBtn = kNone, fileType = kNone;
	};

	// --- actions ---------------------------------------------------------------
	void stub(const std::string& name) {
		s_.setStatus(name + ": not implemented yet " + t_.g().dash + " the execution engine is WIP.");
	}

	void settings() {
		s_.setStatus("Settings: not available in the terminal yet " + std::string(t_.g().dash) + " change them in the GUI.");
	}

	void runTool(int i) { if (tools_[static_cast<size_t>(i)]) tools_[static_cast<size_t>(i)](); }

	// Left/Right pick a menu title, Enter/Space/Down opens it.
	bool onMenuBarEvent(const Event& e) {
		const int n = static_cast<int>(menus_.size());
		if (e == Event::ArrowLeft) { barSel_ = (barSel_ + n - 1) % n; return true; }
		if (e == Event::ArrowRight) { barSel_ = (barSel_ + 1) % n; return true; }
		if (e == Event::Return || e == Event::Character(' ') || e == Event::ArrowDown) { openMenu(barSel_); return true; }
		return false;
	}

	// Left/Right walk the enabled buttons, Enter/Space presses one.
	bool onToolbarEvent(const Event& e) {
		const int d = e == Event::ArrowLeft ? -1 : (e == Event::ArrowRight ? 1 : 0);
		if (d) {
			do toolSel_ = (toolSel_ + kTools + d) % kTools; while (!tools_[static_cast<size_t>(toolSel_)]);
			return true;
		}
		if (e == Event::Return || e == Event::Character(' ')) { runTool(toolSel_); return true; }
		return false;
	}

	// Left press: menus, toolbar, dock tabs, then the pane under the cursor (which
	// takes focus). Wheel: focus the pane under the cursor and move 3 rows a notch.
	bool onMouse(const Mouse& m) {
		const bool press = m.button == Mouse::Left && m.motion == Mouse::Pressed;
		const int wheel = m.button == Mouse::WheelUp ? -1 : (m.button == Mouse::WheelDown ? 1 : 0);
		if (!press && !wheel) return false;
		auto in = [&m](const Box& b) { return b.Contain(m.x, m.y); };

		if (showOpen_) {
			if (wheel) {
				if (wheel < 0 && compSel_ > 0) --compSel_;
				if (wheel > 0 && compSel_ + 1 < comps_.size()) ++compSel_;
			} else if (in(hits_.comps)) {
				constexpr size_t kShown = 5;
				const size_t i = (compSel_ >= kShown ? compSel_ - kShown + 1 : 0) + static_cast<size_t>(m.y - hits_.comps.y_min);
				if (i >= comps_.size()) return true;
				if (i != compSel_) { compSel_ = i; return true; }
				const Completion c = comps_[i];   // clicking the selected entry opens it
				openPath_.set(c.path);
				refreshCompletions();
				if (!c.dir) submitOpen();
			}
			else if (in(hits_.openBtn)) submitOpen();
			else if (in(hits_.cancelBtn)) showOpen_ = false;
			else if (in(hits_.fileType)) { execsOnly_ = !execsOnly_; refreshCompletions(); }
			return true;
		}
		if (press) {
			for (int i = 0; i < static_cast<int>(menus_.size()); ++i)
				if (in(hits_.menu[i])) {
					if (menuOpen_ == i) menuOpen_ = -1; else openMenu(i);
					return true;
				}
		}
		if (menuOpen_ >= 0) {
			if (!press) return true;
			// Inside the border, one row per item (a separator is one row too).
			const int i = m.y - hits_.drop.y_min - 1;
			const auto& items = menus_[static_cast<size_t>(menuOpen_)].items;
			if (in(hits_.drop) && i >= 0 && i < static_cast<int>(items.size()) && !items[static_cast<size_t>(i)].separator) {
				menuSel_ = static_cast<size_t>(i);
				return onMenuEvent(Event::Return);
			}
			menuOpen_ = -1;   // a click outside closes the menu
			return true;
		}
		if (press) {
			gotoActive_ = false;
			for (int i = 0; i < kTools; ++i)
				if (in(hits_.tool[i])) { runTool(i); return true; }
			for (const Tab t : {Tab::Registers, Tab::Stack})
				if (in(hits_.tab[idx(t)])) { tab_ = t; focus_ = Focus::Right; return true; }
			if (in(hits_.welcome)) { beginOpen(""); return true; }
		}
		for (const Focus f : {Focus::Symbols, Focus::Disassembly, Focus::Right, Focus::Memory}) {
			if (!in(hits_.pane[idx(f)])) continue;
			Pane* p = paneFor(f);
			focus_ = f;
			if (wheel)
				for (int k = 0; k < 3; ++k) p->onEvent(wheel < 0 ? Event::ArrowUp : Event::ArrowDown);
			else
				p->click(m.x, m.y);
			return true;
		}
		return false;
	}

	void submitGoto() {
		gotoActive_ = false;
		const std::string raw = trim(goto_.value);
		if (raw.empty()) return;
		// Hex either way, as MainWindow::onGotoSubmitted: "401000", "0x401000".
		const std::string digits = (raw.rfind("0x", 0) == 0 || raw.rfind("0X", 0) == 0) ? raw.substr(2) : raw;
		try {
			size_t used = 0;
			const uint64_t vaddr = std::stoull(digits, &used, 16);
			if (used != digits.size()) throw 0;
			disasm_.navigateTo(vaddr);
			focus_ = Focus::Disassembly;
		} catch (...) {
			s_.setStatus("\"" + raw + "\" is not a hex address.");
		}
	}

	void beginOpen(const std::string& seed) {
		showOpen_ = true;
		openFailed_ = false;
		openPath_.set(seed);
		refreshCompletions();
	}

	void refreshCompletions() {
		comps_.clear();
		compSel_ = 0;
		const std::string typed = unquote(openPath_.value);
		const auto slash = typed.find_last_of("/\\");
		const std::string base = slash == std::string::npos ? std::string() : typed.substr(0, slash + 1);
		const std::string prefix = slash == std::string::npos ? typed : typed.substr(slash + 1);
		std::error_code ec;
		fs::directory_iterator it(expandHome(base.empty() ? "." : base), fs::directory_options::skip_permission_denied, ec);
		if (ec) return;
		for (; it != fs::directory_iterator(); it.increment(ec)) {
			if (ec) break;
			const std::string name = it->path().filename().string();
			if (name.rfind(prefix, 0) != 0) continue;
			if (!name.empty() && name[0] == '.' && (prefix.empty() || prefix[0] != '.')) continue;
			std::error_code e2;
			const bool isDir = it->is_directory(e2);
			if (!isDir) {
				if (!it->is_regular_file(e2)) continue;
				if (execsOnly_ && !looksExecutable(it->path())) continue;
			}
			uintmax_t sz = 0;
			if (!isDir) { sz = it->file_size(e2); if (e2) sz = 0; }
			comps_.push_back({base + name + (isDir ? "/" : ""), isDir, sz});
			if (comps_.size() >= 500) break;
		}
		std::sort(comps_.begin(), comps_.end(), [](const Completion& a, const Completion& b) { return a.path < b.path; });
	}

	void submitOpen() {
		const std::string typed = unquote(openPath_.value);
		std::string target = typed;
		std::error_code ec;
		if (typed.empty() || !fs::is_regular_file(expandHome(typed), ec)) {
			if (!comps_.empty()) {
				const Completion c = comps_[compSel_];
				if (c.dir) { openPath_.set(c.path); refreshCompletions(); return; }
				target = c.path;
			}
			else if (typed.empty()) return;
		}
		if (s_.open(expandHome(target))) {
			showOpen_ = false;
			openFailed_ = false;
			focus_ = Focus::Disassembly;
		} else {
			openFailed_ = true;   // Session::open put the reason in status()
		}
	}

	// --- focus -----------------------------------------------------------------
	std::vector<Focus> focusOrder() const {
		std::vector<Focus> o = {Focus::MenuBar, Focus::Toolbar};
		if (s_.loaded() && sidebarOn_) o.push_back(Focus::Symbols);
		o.push_back(Focus::Disassembly);
		if (regsOn_ || stackOn_) o.push_back(Focus::Right);
		if (memoryOn_) o.push_back(Focus::Memory);
		return o;
	}

	void validateFocus() {
		const auto o = focusOrder();
		if (std::find(o.begin(), o.end(), focus_) == o.end()) focus_ = Focus::Disassembly;
		if (tab_ == Tab::Registers && !regsOn_ && stackOn_) tab_ = Tab::Stack;
		if (tab_ == Tab::Stack && !stackOn_ && regsOn_) tab_ = Tab::Registers;
	}

	void cycleFocus(int d) {
		if (Pane* p = focusedPane(); p && p->tab(d)) return;   // the pane's own fields first
		const auto o = focusOrder();
		auto it = std::find(o.begin(), o.end(), focus_);
		size_t i = it == o.end() ? 0 : static_cast<size_t>(it - o.begin());
		i = (i + o.size() + static_cast<size_t>(d + static_cast<int>(o.size()))) % o.size();
		focus_ = o[i];
		if (Pane* p = focusedPane()) p->enter(d);
	}

	void switchTab() {
		if (regsOn_ && stackOn_) tab_ = tab_ == Tab::Registers ? Tab::Stack : Tab::Registers;
		if (regsOn_ || stackOn_) focus_ = Focus::Right;
	}

	Pane* focusedPane() { return paneFor(focus_); }

	Pane* paneFor(Focus f) {
		switch (f) {
		case Focus::MenuBar:
		case Focus::Toolbar: return nullptr;
		case Focus::Symbols: return &symbols_;
		case Focus::Disassembly: return &disasm_;
		case Focus::Right: return tab_ == Tab::Registers ? static_cast<Pane*>(&regs_) : &stack_;
		case Focus::Memory: return &memory_;
		}
		return nullptr;
	}

	// --- menus (MainWindow::buildMenus) ----------------------------------------
	void buildMenus() {
		const std::string e3 = t_.g().ellipsis;
		auto item = [](std::string l, std::string k, std::function<void()> run,
		               std::function<bool()> en = nullptr, std::function<bool()> chk = nullptr) {
			return MenuItem{std::move(l), std::move(k), std::move(run), std::move(en), std::move(chk), false};
		};
		auto st = [this](const char* n) { return [this, n] { stub(n); }; };
		menus_ = {
			{"File", { item("Open" + e3, "Ctrl+O", [this] { beginOpen(""); }), sep(),
			           item("Quit", "Ctrl+Q", [this] { screen_.ExitLoopClosure()(); }) }},
			{"Debug", { item("Run", "F5", st("Run")), item("Step Into", "F7", st("Step Into")),
			            item("Step Over", "F8", st("Step Over")), item("Continue", "F9", st("Continue")),
			            item("Pause", "", st("Pause")), item("Reset", "", st("Reset")) }},
			// Enabled in Qt once an instruction is edited; the TUI has no editor yet.
			{"Edit", { item("Recompile", "", nullptr, [] { return false; }) }},
			{"View", { item("Symbol Sidebar", "Ctrl+B", [this] { sidebarOn_ = !sidebarOn_; }, nullptr, [this] { return sidebarOn_; }), sep(),
			           item("Registers", "", [this] { regsOn_ = !regsOn_; }, nullptr, [this] { return regsOn_; }),
			           item("Stack", "", [this] { stackOn_ = !stackOn_; }, nullptr, [this] { return stackOn_; }),
			           item("Memory", "", [this] { memoryOn_ = !memoryOn_; }, nullptr, [this] { return memoryOn_; }) }},
			{"Tools", { item("Settings" + e3, "", [this] { settings(); }) }},
		};
		// Indexed by Tool; null is disabled (Recompile: the TUI has no editor yet).
		tools_ = {
			[this] { sidebarOn_ = !sidebarOn_; }, [this] { beginOpen(""); },
			st("Run"), st("Step Into"), st("Step Over"), st("Continue"), st("Pause"), st("Reset"),
			nullptr, [this] { if (s_.loaded()) { gotoActive_ = true; goto_.set(""); } }, [this] { settings(); },
		};
	}

	void openMenu(int i) {
		menuOpen_ = barSel_ = i;
		menuSel_ = 0;
		const auto& items = menus_[static_cast<size_t>(i)].items;
		while (menuSel_ < items.size() && items[menuSel_].separator) ++menuSel_;
	}

	bool onMenuEvent(const Event& e) {
		const int n = static_cast<int>(menus_.size());
		auto& items = menus_[static_cast<size_t>(menuOpen_)].items;
		const size_t count = items.size();
		if (e == Event::Escape || e == Event::F10) { menuOpen_ = -1; return true; }
		if (e == Event::ArrowLeft) { openMenu((menuOpen_ + n - 1) % n); return true; }
		if (e == Event::ArrowRight) { openMenu((menuOpen_ + 1) % n); return true; }
		if (const int m = altMenu(e); m >= 0) { openMenu(m); return true; }
		if (e == Event::ArrowUp || e == Event::ArrowDown) {
			const size_t step = e == Event::ArrowUp ? count - 1 : 1;
			size_t i = menuSel_;
			do { i = (i + step) % count; } while (items[i].separator && i != menuSel_);
			menuSel_ = i;
			return true;
		}
		if (e == Event::Return) {
			const MenuItem it = items[menuSel_];
			menuOpen_ = -1;
			if ((!it.enabled || it.enabled()) && it.run) it.run();
			return true;
		}
		return true;   // the open menu owns the keyboard
	}

	bool onOpenEvent(const Event& e) {
		if (e == Event::Escape) { showOpen_ = false; return true; }
		if (e == Event::Return) { submitOpen(); return true; }
		if (e == Event::Tab) {
			if (!comps_.empty()) { openPath_.set(comps_[compSel_].path); refreshCompletions(); }
			return true;
		}
		if (e == Event::ArrowUp) { if (compSel_ > 0) --compSel_; return true; }
		if (e == Event::ArrowDown) { if (compSel_ + 1 < comps_.size()) ++compSel_; return true; }
		if (e == Event::CtrlT) { execsOnly_ = !execsOnly_; refreshCompletions(); return true; }
		if (openPath_.handle(e)) { openFailed_ = false; refreshCompletions(); }
		return true;   // modal
	}

	// --- rendering ---------------------------------------------------------------
	Element menuBar() {
		Elements row = { text(" ") };
		for (size_t i = 0; i < menus_.size(); ++i) {
			const std::string& title = menus_[i].title;
			const bool active = static_cast<int>(i) == menuOpen_
				|| (menuOpen_ < 0 && focus_ == Focus::MenuBar && static_cast<int>(i) == barSel_);
			Element cell = hbox({ text(" "), text(title.substr(0, 1)) | underlined, text(title.substr(1)), text(" ") })
				| t_.fg(active ? Role::Bright : Role::Muted) | when(active, t_.bg(Role::AccentBg));
			row.push_back(cell | reflect(hits_.menu[i]));
			row.push_back(text(" "));
		}
		row.push_back(filler());
		row.push_back(t_.ink(s_.loaded() ? std::string("voidwalk ") + t_.g().dash + " " + s_.filePath() : "voidwalk", Role::Faint));
		row.push_back(text(" "));
		return hbox(std::move(row));
	}

	Element toolBar() {
		const Glyphs& g = t_.g();
		const bool loaded = s_.loaded();
		const bool hints = Terminal::Size().dimx >= 132;   // F-key hints only when they fit
		auto bar = [&] { return t_.ink(g.v, Role::Line); };
		auto key = [&](const char* k) { return t_.ink(k, Role::Faint); };
		// A button: inverted while it has the toolbar focus, and hit-tested for clicks.
		auto tb = [&](int i, Element el) {
			return el | when(focus_ == Focus::Toolbar && toolSel_ == i, inverted) | reflect(hits_.tool[i]);
		};
		const bool side = loaded && sidebarOn_;

		Elements r = {
			text(" "),
			tb(TSidebar, t_.ink(std::string(" ") + g.sidebar + " ", side ? Role::Bright : Role::Ghost) | when(side, t_.bg(Role::Control))),
			text(" "), bar(), text(" "),
			tb(TOpen, hbox({ t_.ink(std::string("Open") + g.ellipsis, Role::Text), text(" "), key("^O") })), text("  "), bar(), text("  "),
			tb(TRun, hbox({ t_.ink(std::string(" ") + g.run + " Run ", Role::AccentText), t_.ink(hints ? "F5 " : "", Role::Accent) })
				| t_.bg(Role::RunBg)),
			text("  "),
		};
		const std::pair<const char*, const char*> acts[] = {
			{"Step Into", "F7"}, {"Step Over", "F8"}, {"Continue", "F9"}, {"Pause", ""}, {"Reset", ""},
		};
		int i = TStepInto;
		for (const auto& [label, k] : acts) {
			r.push_back(tb(i++, hbox({ t_.ink(label, Role::Text), hints && *k ? hbox({ text(" "), key(k) }) : text("") })));
			r.push_back(text("  "));
		}
		r.push_back(bar());
		r.push_back(text("  "));
		r.push_back(tb(TRecompile, t_.ink("Recompile", Role::Ghost)));
		r.push_back(filler());
		Element gotoInner = gotoActive_
			? goto_.render(t_, "", true, 17)
			: hbox({ fixed(t_.ink("Go to address", Role::Ghost), 15), t_.ink("^G", loaded ? Role::Faint : Role::Ghost) });
		r.push_back(tb(TGoto, field(t_, gotoInner, loaded)));
		r.push_back(text("  "));
		r.push_back(tb(TSettings, t_.ink("Settings", Role::Text)));
		r.push_back(text(" "));
		return hbox(std::move(r));
	}

	Element body() {
		const int h = std::max(1, bodyBox_.y_max - bodyBox_.y_min + 1);
		const bool loaded = s_.loaded();
		Elements cols;
		if (loaded && sidebarOn_) {
			cols.push_back(symbols_.render(focus_ == Focus::Symbols) | size(WIDTH, EQUAL, 22) | reflect(hits_.pane[idx(Focus::Symbols)]));
			cols.push_back(scrollRule(t_, h, symbols_.scroll()));
		}
		cols.push_back((loaded ? disasm_.render(focus_ == Focus::Disassembly) : welcome()) | flex | reflect(hits_.pane[idx(Focus::Disassembly)]));
		if (regsOn_ || stackOn_) {
			cols.push_back(scrollRule(t_, h, loaded ? disasm_.scroll() : ScrollState{}));
			cols.push_back(rightDock() | size(WIDTH, EQUAL, kRightWidth) | reflect(hits_.pane[idx(Focus::Right)]));
		}
		return hbox(std::move(cols)) | reflect(bodyBox_);
	}

	Element rightDock() {
		const Glyphs& g = t_.g();
		std::vector<std::pair<Tab, const char*>> tabs;
		if (regsOn_) tabs.push_back({Tab::Registers, "Registers"});
		if (stackOn_) tabs.push_back({Tab::Stack, "Stack"});

		Elements head = { text(" ") };
		int x = 1, activeX = 1, activeLen = 0;
		for (const auto& [tab, name] : tabs) {
			const bool active = tab == tab_;
			const int len = static_cast<int>(std::strlen(name));
			head.push_back(t_.ink(name, active ? Role::Bright : Role::Dim) | reflect(hits_.tab[idx(tab)]));
			head.push_back(text("   "));
			if (active) { activeX = x; activeLen = len; }
			x += len + 3;
		}
		head.push_back(filler());
		if (tabs.size() > 1) head.push_back(t_.ink("[ ] ", Role::Faint));   // the switch keys
		// Underline, not folder tabs (QTabBar::tab:selected border-bottom: accent).
		const int lead = activeX - 1, mark = activeLen + 2;
		Element under = hbox({
			t_.ink(repeat(g.h, lead), Role::Line),
			t_.ink(repeat(g.tabRule, mark), Role::Accent),
			t_.ink(repeat(g.h, std::max(0, kRightWidth - lead - mark)), Role::Line),
		});
		Pane& pane = tab_ == Tab::Registers ? static_cast<Pane&>(regs_) : static_cast<Pane&>(stack_);
		return vbox({ hbox(std::move(head)), under, pane.render(focus_ == Focus::Right) | flex });
	}

	// WelcomeWidget, cell for cell: badge, title, hint, default button, chips.
	Element welcome() {
		const Glyphs& g = t_.g();
		Elements chips;
		for (const char* ext : {".exe", ".dll", ".so", ".elf"}) {
			chips.push_back(t_.ink("(", Role::Line));
			chips.push_back(t_.ink(std::string(" ") + ext + " ", Role::Dim));
			chips.push_back(t_.ink(")", Role::Line));
			chips.push_back(text("  "));
		}
		return vbox({
			filler(),
			t_.ink("  01  ", Role::Accent) | bold | t_.bg(Role::AccentBg)
				| borderStyled(LIGHT, t_.colorOf(Role::Accent)) | hcenter,
			text(""),
			t_.ink("Open a binary to begin", Role::Bright) | hcenter,
			text(""),
			t_.ink(std::string("voidwalk analyzes PE and ELF executables ") + g.dash + " sections, disassembly,", Role::Muted) | hcenter,
			t_.ink("and (soon) live debugging. Drop a file here or press Ctrl+O.", Role::Muted) | hcenter,
			text(""),
			t_.ink(std::string("  Open binary") + g.ellipsis + "  ", Role::AccentText) | t_.bg(Role::RunBg) | reflect(hits_.welcome) | hcenter,
			text(""),
			hbox(std::move(chips)) | hcenter,
			filler(),
		});
	}

	Element statusBar() {
		const Glyphs& g = t_.g();
		Elements r = { text(" "), t_.ink(s_.status(), Role::Muted), filler() };
		if (s_.loaded()) {
			r.push_back(t_.ink(g.v, Role::Line));
			r.push_back(t_.ink(" " + std::to_string(s_.rowCount()) + " instr ", Role::Dim));
			r.push_back(t_.ink(g.v, Role::Line));
			r.push_back(t_.ink(" " + s_.format() + " " + g.dot + " " + s_.architecture() + " ", Role::Accent));
		}
		return hbox(std::move(r));
	}

	Element menuOverlay() {
		const Glyphs& g = t_.g();
		const Menu& m = menus_[static_cast<size_t>(menuOpen_)];
		int x = 1;
		for (int j = 0; j < menuOpen_; ++j) x += static_cast<int>(menus_[static_cast<size_t>(j)].title.size()) + 3;
		int w = 0;
		for (const MenuItem& it : m.items)
			w = std::max(w, static_cast<int>(it.label.size() + it.shortcut.size()) + 8);

		Elements rows;
		for (size_t i = 0; i < m.items.size(); ++i) {
			const MenuItem& it = m.items[i];
			if (it.separator) { rows.push_back(rule(t_)); continue; }
			const bool en = !it.enabled || it.enabled();
			const bool chk = it.checked && it.checked();
			const bool sel = i == menuSel_;
			Element row = hbox({
				text(" "), text(chk ? g.check : " "), text(" "), text(it.label), filler(),
				t_.ink(it.shortcut, sel && en ? Role::AccentText : Role::Faint), text(" "),
			}) | t_.fg(!en ? Role::Ghost : (sel ? Role::Bright : Role::Text)) | when(sel && en, t_.bg(Role::AccentBg));
			rows.push_back(row | size(WIDTH, EQUAL, w));
		}
		Element box = vbox(std::move(rows)) | borderStyled(LIGHT, t_.colorOf(Role::Line)) | clear_under | reflect(hits_.drop);
		return vbox({
			emptyElement() | size(HEIGHT, EQUAL, 1),
			hbox({ emptyElement() | size(WIDTH, EQUAL, x), box, filler() }),
			filler(),
		});
	}

	Element openDialog() {
		constexpr int W = 70;
		const Glyphs& g = t_.g();
		Elements r;
		r.push_back(hbox({ text(" "), t_.ink("Open binary", Role::Bright), filler(), t_.ink("Esc", Role::Faint), text(" ") }));
		r.push_back(text(""));
		r.push_back(hbox({ text(" "), fixed(t_.ink("Path", Role::Muted), 6), field(t_, openPath_.render(t_, "path to binary", true, W - 12)) }));
		r.push_back(hbox({ text("         "),
			t_.ink(std::string("Tab completes ") + g.dot + " " + g.arrows + " choose " + g.dot + " Enter opens", Role::Faint) }));

		constexpr size_t kShown = 5;
		const size_t start = compSel_ >= kShown ? compSel_ - kShown + 1 : 0;
		Elements list;
		for (size_t k = 0; k < kShown; ++k) {
			const size_t i = start + k;
			if (i >= comps_.size()) { list.push_back(text("")); continue; }
			const Completion& c = comps_[i];
			const bool sel = i == compSel_;
			Element row = hbox({
				text(" "), t_.ink(clip(c.path, W - 26, g.clip), sel ? Role::Bright : (c.dir ? Role::Accent : Role::Text)),
				filler(), t_.ink(c.dir ? "dir" : humanSize(c.size), sel ? Role::AccentText : Role::Ghost), text(" "),
			}) | size(WIDTH, EQUAL, W - 12) | when(sel, t_.bg(Role::AccentBg));
			list.push_back(hbox({ text("         "), row }));
		}
		r.push_back(vbox(std::move(list)) | reflect(hits_.comps));
		r.push_back(openFailed_ ? hbox({ text(" "), t_.ink(s_.status(), Role::Breakpoint) }) : text(""));
		r.push_back(hbox({
			text(" "), t_.ink("Files of type:", Role::Muted), text(" "),
			field(t_, hbox({ fixed(t_.ink(execsOnly_ ? "Executables (*.exe *.dll *.so *.elf *.bin)" : "All files (*)", Role::Text), 43),
			                 t_.ink(g.drop, Role::Text) })) | reflect(hits_.fileType),
			text(" "), t_.ink("^T", Role::Faint),
		}));
		r.push_back(text(""));
		r.push_back(hbox({
			filler(), t_.ink("  Cancel  ", Role::Text) | t_.bg(Role::Control) | reflect(hits_.cancelBtn), text("  "),
			t_.ink("  Open  ", Role::AccentText) | t_.bg(Role::RunBg) | reflect(hits_.openBtn), text(" "),
		}));
		return vbox(std::move(r)) | size(WIDTH, EQUAL, W) | borderStyled(LIGHT, t_.colorOf(Role::Line));
	}

	static constexpr int kRightWidth = 32;

	Session& s_;
	ScreenInteractive& screen_;
	const Theme& t_;
	DisassemblyPane disasm_;
	SymbolsPane symbols_;
	RegistersPane regs_;
	StackPane stack_;
	MemoryPane memory_;

	Focus focus_ = Focus::Disassembly;
	Tab tab_ = Tab::Registers;
	bool sidebarOn_ = true, regsOn_ = true, stackOn_ = true, memoryOn_ = true;

	std::vector<Menu> menus_;
	std::vector<std::function<void()>> tools_;
	int toolSel_ = TOpen;
	Hits hits_;
	int menuOpen_ = -1;
	int barSel_ = 0;   // menu title selected while the menu bar has focus
	size_t menuSel_ = 0;

	bool gotoActive_ = false;
	LineEdit goto_;

	bool showOpen_ = false;
	bool openFailed_ = false;
	bool execsOnly_ = true;
	LineEdit openPath_;
	std::vector<Completion> comps_;
	size_t compSel_ = 0;

	Box bodyBox_;
	uint64_t refreshedGen_ = 0;
	std::atomic<bool> ticking_{true};
};

UI::UI(Session session) : session_(std::move(session)) {}

int UI::start() {
	auto screen = ScreenInteractive::Fullscreen();
	auto app = Make<AppView>(session_, screen);

	// FTXUI redraws only on events and the decode worker produces none. Post one
	// ~10x/s while a decode is unfinished (and once at start, so the panes size
	// themselves from the first frame's layout). Declared after `screen`, so it is
	// joined before the screen is destroyed.
	std::jthread ticker([&screen, app](std::stop_token st) {
		bool first = true;
		while (!st.stop_requested()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (first || app->ticking()) screen.PostEvent(Event::Custom);
			first = false;
		}
	});

	screen.Loop(app);
	return 0;
}

} // namespace tui
