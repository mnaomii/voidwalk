#pragma once
#include "tui/session.hpp"
#include "tui/theme/palette.hpp"
#include "tui/widgets.hpp"

#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Panes are plain view objects, not ftxui Components. UI owns focus and routes
// keys itself, so the Tab order is the Qt dock order (Symbols -> Disassembly ->
// right dock -> Memory) rather than whatever FTXUI's containers would derive.
// A pane only answers "draw yourself" and "did you use this key".

namespace tui {

class Pane {
public:
	virtual ~Pane() = default;
	virtual ftxui::Element render(bool focused) = 0;
	virtual bool onEvent(const ftxui::Event& e) = 0;
	virtual ScrollState scroll() const { return {}; }
	// True while the pane is editing text or has a popup open and wants every key.
	virtual bool capturing() const { return false; }
	// A left click at screen cell (x, y) inside the pane. True if it did something.
	virtual bool click(int /*x*/, int /*y*/) { return false; }
	// Tab stops inside the pane (its fields). tab() moves one stop in direction d and
	// returns false at the edge, so focus leaves the pane; enter() picks the first
	// (d > 0) or last stop when focus arrives.
	virtual bool tab(int /*d*/) { return false; }
	virtual void enter(int /*d*/) {}
};

// ADDRESS | BYTES | INSTRUCTION | NOTES, as gui::DisassemblyPane.
class DisassemblyPane : public Pane {
public:
	DisassemblyPane(Session& session, const Theme& theme) : s_(session), t_(theme) {}
	ftxui::Element render(bool focused) override;
	bool onEvent(const ftxui::Event& e) override;
	bool click(int x, int y) override;
	ScrollState scroll() const override;
	void navigateTo(uint64_t vaddr);

private:
	void sync();
	void clamp();
	Session& s_;
	const Theme& t_;
	Viewport vp_;
	size_t cursor_ = 0, top_ = 0;
	uint64_t gen_ = ~0ull;
};

// SYMBOLS header, "/ Filter" field, .TEXT / IMPORTS / STRINGS groups.
class SymbolsPane : public Pane {
public:
	SymbolsPane(Session& session, const Theme& theme) : s_(session), t_(theme) {}
	ftxui::Element render(bool focused) override;
	bool onEvent(const ftxui::Event& e) override;
	bool click(int x, int y) override;
	ScrollState scroll() const override;
	bool capturing() const override { return filtering_; }
	void beginFilter() { filtering_ = true; filter_.cursor = filter_.value.size(); }

	std::function<void(uint64_t vaddr)> onNavigate;   // functions
	std::function<void(uint64_t offset)> onMemory;    // strings

private:
	struct Item {
		bool group = false;
		std::string name, detail;
		Role role = Role::Text;
		uint64_t addr = 0;
		bool string = false;
	};
	void rebuild();
	void moveCursor(int delta);
	Session& s_;
	const Theme& t_;
	Viewport vp_;
	LineEdit filter_;
	ftxui::Box filterBox_;
	bool filtering_ = false;
	std::vector<Item> items_;
	size_t shown_ = 0;
	size_t cursor_ = 0, top_ = 0;
	std::string builtFilter_;
	uint64_t builtGen_ = ~0ull;
	bool builtScanning_ = false;
};

// Register | Value tree: General Purpose, Instruction Pointer, Segment, Flags open;
// MMX / SSE / AVX / AVX-512 collapsed - gui::RegistersPane's defaults.
class RegistersPane : public Pane {
public:
	RegistersPane(Session& session, const Theme& theme) : s_(session), t_(theme) {}
	ftxui::Element render(bool focused) override;
	bool onEvent(const ftxui::Event& e) override;
	bool click(int x, int y) override;
	ScrollState scroll() const override;

private:
	struct Row {
		int category = -1;         // >= 0: a category header
		std::string name, value;
		std::string name2, value2; // segment rows carry two registers
	};
	std::vector<Row> build() const;
	Session& s_;
	const Theme& t_;
	Viewport vp_;
	bool open_[8] = {true, true, true, true, false, false, false, false};
	size_t cursor_ = 0, top_ = 0, total_ = 0;
};

class StackPane : public Pane {
public:
	StackPane(Session& session, const Theme& theme) : s_(session), t_(theme) {}
	ftxui::Element render(bool focused) override;
	bool onEvent(const ftxui::Event&) override { return false; }

private:
	Session& s_;
	const Theme& t_;
};

// Header row (Section: [Jump to...] / Go to offset: [0x0]) over the hexdump,
// as gui::MemoryPane. 's' opens the section list, 'f' edits the offset.
class MemoryPane : public Pane {
public:
	MemoryPane(Session& session, const Theme& theme) : s_(session), t_(theme) {}
	ftxui::Element render(bool focused) override;
	bool onEvent(const ftxui::Event& e) override;
	bool click(int x, int y) override;
	ScrollState scroll() const override;
	bool capturing() const override { return picking_ || editing_; }
	bool tab(int d) override;
	void enter(int d) override { at_ = d > 0 ? Section : Rows; }
	void gotoOffset(uint64_t offset);

private:
	void sync();
	size_t totalRows() const;
	Session& s_;
	const Theme& t_;
	Viewport vp_;
	std::vector<SectionInfo> sections_;
	int chosen_ = -1;
	bool picking_ = false;
	size_t pick_ = 0, pickTop_ = 0;   // the section list scrolls on its own, not the hexdump's top_
	bool editing_ = false;
	LineEdit offset_;
	ftxui::Box sectionBox_, offsetBox_;
	enum Stop { Section, Offset, Rows } at_ = Rows;   // which tab stop has focus
	size_t cursor_ = 0, top_ = 0;
	uint64_t gen_ = ~0ull;
};

} // namespace tui
