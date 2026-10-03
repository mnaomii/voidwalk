#include "tui/panes/panes.hpp"

#include <algorithm>
#include <cctype>

namespace tui {

using namespace ftxui;

namespace {
std::string lower(std::string s) {
	for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}
} // namespace

void SymbolsPane::rebuild() {
	const bool scanning = s_.scanningSymbols();
	if (builtGen_ == s_.decodeGeneration() && builtFilter_ == filter_.value && builtScanning_ == scanning)
		return;
	const bool sameList = builtGen_ == s_.decodeGeneration() && builtFilter_ == filter_.value;
	builtGen_ = s_.decodeGeneration();
	builtFilter_ = filter_.value;
	builtScanning_ = scanning;

	items_.clear();
	shown_ = 0;
	if (scanning) return;

	struct Bucket { SymbolInfo::Kind kind; const char* title; Role role; };
	const Bucket buckets[] = {
		{SymbolInfo::Kind::Function, ".TEXT", Role::Text},
		{SymbolInfo::Kind::Import, "IMPORTS", Role::Text},
		{SymbolInfo::Kind::String, "STRINGS", Role::SynString},
	};
	const std::string needle = lower(filter_.value);
	for (const Bucket& b : buckets) {
		std::vector<const SymbolInfo*> matches;
		for (const SymbolInfo& sym : s_.symbols()) {
			if (sym.kind != b.kind) continue;
			if (!needle.empty() && lower(sym.name).find(needle) == std::string::npos) continue;
			matches.push_back(&sym);
		}
		if (matches.empty()) continue;   // an empty group is noise, not information
		items_.push_back({true, b.title, std::to_string(matches.size()), Role::Faint, 0, false});
		for (const SymbolInfo* m : matches)
			items_.push_back({false, m->name, m->detail, b.role, m->addr, m->kind == SymbolInfo::Kind::String});
		shown_ += matches.size();
	}
	if (!sameList) { cursor_ = 0; top_ = 0; }
	if (!items_.empty() && items_[std::min(cursor_, items_.size() - 1)].group) moveCursor(+1);
}

void SymbolsPane::moveCursor(int delta) {
	if (items_.empty()) return;
	if (cursor_ >= items_.size()) cursor_ = items_.size() - 1;
	long i = static_cast<long>(std::min(cursor_, items_.size() - 1));
	const long n = static_cast<long>(items_.size());
	do { i += delta; } while (i >= 0 && i < n && items_[static_cast<size_t>(i)].group);
	if (i >= 0 && i < n) cursor_ = static_cast<size_t>(i);
	else if (items_[cursor_].group) {   // ran off the end from a header: search the other way
		i = static_cast<long>(cursor_);
		do { i -= delta; } while (i >= 0 && i < n && items_[static_cast<size_t>(i)].group);
		if (i >= 0 && i < n) cursor_ = static_cast<size_t>(i);
	}
}

ScrollState SymbolsPane::scroll() const {
	return {items_.size(), top_, static_cast<size_t>(vp_.rows())};
}

bool SymbolsPane::onEvent(const Event& e) {
	rebuild();
	if (filtering_) {
		if (e == Event::Escape) { filtering_ = false; filter_.set(""); return true; }
		if (e == Event::Return || e == Event::ArrowDown) { filtering_ = false; return true; }
		filter_.handle(e);
		return true;   // the filter owns the keyboard while it is open
	}
	if (items_.empty()) return false;
	const int page = std::max(1, vp_.rows() - 1);
	if (e == Event::ArrowUp)   { moveCursor(-1); return true; }
	if (e == Event::ArrowDown) { moveCursor(+1); return true; }
	if (e == Event::PageUp)    { for (int k = 0; k < page; ++k) moveCursor(-1); return true; }
	if (e == Event::PageDown)  { for (int k = 0; k < page; ++k) moveCursor(+1); return true; }
	if (e == Event::Home)      { cursor_ = 0; moveCursor(+1); return true; }
	if (e == Event::End)       { cursor_ = items_.size() - 1; if (items_[cursor_].group) moveCursor(-1); return true; }
	if (e == Event::Return) {
		const Item& it = items_[cursor_];
		if (it.group) return true;
		if (it.string) { if (onMemory) onMemory(it.addr); }
		else if (onNavigate) onNavigate(it.addr);
		return true;
	}
	return false;
}

bool SymbolsPane::click(int x, int y) {
	rebuild();
	if (filterBox_.Contain(x, y)) { beginFilter(); return true; }
	filtering_ = false;
	size_t i = 0;
	if (!rowAt(vp_, y, top_, i) || i >= items_.size() || items_[i].group) return false;
	if (i == cursor_) return onEvent(Event::Return);   // clicking the selected row activates it
	cursor_ = i;
	return true;
}

Element SymbolsPane::render(bool focused) {
	rebuild();
	const Glyphs& g = t_.g();
	const int cols = vp_.cols(22);

	Elements out;
	out.push_back(hbox({
		text(" "), t_.ink("SYMBOLS", focused ? Role::Muted : Role::Faint), filler(),
		t_.ink(shown_ ? std::to_string(shown_) : std::string(), Role::Ghost), text(" "),
	}));
	out.push_back(hbox({ text(" "), field(t_, filter_.render(t_, "/ Filter", filtering_, std::max(4, cols - 6))) | reflect(filterBox_) }));
	out.push_back(text(""));

	Elements rows;
	if (s_.scanningSymbols()) {
		rows.push_back(t_.ink(std::string("  Scanning") + g.ellipsis, Role::Ghost));
	} else {
		follow(cursor_, top_, items_.size(), vp_.rows());
		const size_t end = std::min(items_.size(), top_ + static_cast<size_t>(vp_.rows()));
		for (size_t i = top_; i < end; ++i) {
			const Item& it = items_[i];
			if (it.group) {
				rows.push_back(hbox({
					text(" "), t_.ink(std::string(g.open) + " " + it.name, Role::Faint), filler(),
					t_.ink(it.detail, Role::Ghost), text(" "),
				}));
				continue;
			}
			const bool sel = i == cursor_;
			const bool hi = sel && focused;
			const int nameW = std::max(1, cols - 3 - static_cast<int>(it.detail.size()) - 2);
			Element row = hbox({
				text("   "),
				t_.ink(clip(it.name, static_cast<size_t>(nameW), g.clip), sel ? Role::Bright : it.role),
				filler(),
				t_.ink(it.detail, hi ? Role::AccentText : Role::Ghost),
				text(" "),
			});
			if (hi) row = row | t_.bg(Role::AccentBg);
			rows.push_back(std::move(row));
		}
	}
	out.push_back(vbox(std::move(rows)) | yflex | reflect(vp_.box));
	return vbox(std::move(out));
}

} // namespace tui
