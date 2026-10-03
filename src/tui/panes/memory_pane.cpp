#include "tui/panes/panes.hpp"

#include <algorithm>
#include <cstdio>

namespace tui {

using namespace ftxui;

namespace {

std::string sectionEntry(const SectionInfo& s) {
	char buf[96];
	std::snprintf(buf, sizeof(buf), "%s  (off 0x%llx, %llu bytes)", s.name.c_str(),
	              static_cast<unsigned long long>(s.offset), static_cast<unsigned long long>(s.size));
	return buf;
}

} // namespace

void MemoryPane::sync() {
	const uint64_t gen = s_.loaded() ? s_.decodeGeneration() : 0;
	if (gen == gen_) return;
	gen_ = gen;
	sections_ = s_.sections();
	chosen_ = -1;
	picking_ = editing_ = false;
	at_ = Rows;
	cursor_ = top_ = 0;   // MemoryPane::refresh() scrolls to the top on a new binary
}

size_t MemoryPane::totalRows() const {
	return s_.loaded() ? (s_.binarySize() + 15) / 16 : 0;
}

void MemoryPane::gotoOffset(uint64_t offset) {
	sync();
	const size_t rows = totalRows();
	if (rows == 0) return;
	cursor_ = static_cast<size_t>(std::min<uint64_t>(offset / 16, rows - 1));
	top_ = cursor_;   // PositionAtTop, as the Qt pane
}

ScrollState MemoryPane::scroll() const {
	return {totalRows(), top_, static_cast<size_t>(vp_.rows())};
}

bool MemoryPane::tab(int d) {
	const int next = at_ + d;
	if (next < Section || next > Rows) return false;
	at_ = static_cast<Stop>(next);
	return true;
}

bool MemoryPane::onEvent(const Event& e) {
	sync();
	if (picking_) {
		if (e == Event::Escape) { picking_ = false; return true; }
		if (e == Event::ArrowUp) { if (pick_ > 0) --pick_; return true; }
		if (e == Event::ArrowDown) { if (pick_ + 1 < sections_.size()) ++pick_; return true; }
		if (e == Event::Return) {
			chosen_ = static_cast<int>(pick_);
			picking_ = false;
			gotoOffset(sections_[pick_].offset);
		}
		return true;
	}
	if (editing_) {
		if (e == Event::Escape) { editing_ = false; return true; }
		if (e == Event::Return) {
			editing_ = false;
			std::string v = offset_.value;
			if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0) v = v.substr(2);
			try {
				size_t used = 0;
				const uint64_t off = std::stoull(v, &used, 16);
				if (used == v.size()) gotoOffset(off);   // not hex: ignored, as in Qt
			} catch (...) {}
			return true;
		}
		offset_.handle(e);
		return true;
	}
	if (!s_.loaded()) return false;
	if (at_ != Rows) {
		// On a field: Enter/Space opens it, Left/Right swaps fields, Down goes to the rows.
		if (e == Event::Return || e == Event::Character(' ')) return onEvent(Event::Character(at_ == Section ? 's' : 'f'));
		if (e == Event::ArrowLeft || e == Event::ArrowRight) { at_ = at_ == Section ? Offset : Section; return true; }
		if (e == Event::ArrowDown) { at_ = Rows; return true; }
	}
	if (e == Event::Character('s') && !sections_.empty()) {
		at_ = Section;
		picking_ = true;
		pick_ = chosen_ >= 0 ? static_cast<size_t>(chosen_) : 0;
		return true;
	}
	if (e == Event::Character('f')) { at_ = Offset; editing_ = true; offset_.set(""); return true; }
	if (moveRows(e, cursor_, totalRows(), vp_.rows())) { at_ = Rows; follow(cursor_, top_, totalRows(), vp_.rows()); return true; }
	return false;
}

bool MemoryPane::click(int x, int y) {
	sync();
	if (!s_.loaded()) return false;
	if (sectionBox_.Contain(x, y)) {
		editing_ = false;
		at_ = Section;
		if (picking_) { picking_ = false; return true; }
		return onEvent(Event::Character('s'));
	}
	if (offsetBox_.Contain(x, y)) {
		picking_ = false;
		at_ = Offset;
		if (!editing_) { editing_ = true; offset_.set(""); }
		return true;
	}
	editing_ = false;
	size_t i = 0;
	if (!rowAt(vp_, y, picking_ ? pickTop_ : top_, i)) return false;
	if (picking_) {
		if (i >= sections_.size()) return false;
		pick_ = i;
		return onEvent(Event::Return);
	}
	if (i >= totalRows()) return false;
	at_ = Rows;
	cursor_ = i;
	return true;
}

Element MemoryPane::render(bool focused) {
	sync();
	const Glyphs& g = t_.g();
	const bool loaded = s_.loaded();
	const std::string label = chosen_ >= 0 ? sectionEntry(sections_[static_cast<size_t>(chosen_)])
	                                       : std::string("Jump to") + g.ellipsis;

	auto stop = [&](Stop s, Element el) { return focused && at_ == s ? el | inverted : el; };

	Elements out;
	out.push_back(hbox({
		text(" "), t_.ink("Memory", focused ? Role::Bright : Role::Muted), text("   "),
		t_.ink("Section:", Role::Text), text(" "),
		stop(Section, field(t_, hbox({ fixed(t_.ink(clip(label, 34, g.clip), Role::Text), 35), t_.ink(g.drop, Role::Text) }), loaded)) | reflect(sectionBox_),
		text(" "), t_.ink("s", Role::Faint), text("   "),
		t_.ink("Go to offset:", Role::Text), text(" "),
		stop(Offset, field(t_, offset_.render(t_, "0x0", editing_, 12), loaded)) | reflect(offsetBox_),
		text(" "), t_.ink("f", Role::Faint), filler(),
	}));

	Elements rows;
	if (picking_) {
		follow(pick_, pickTop_, sections_.size(), vp_.rows());
		const size_t end = std::min(sections_.size(), pickTop_ + static_cast<size_t>(vp_.rows()));
		for (size_t i = pickTop_; i < end; ++i) {
			const bool sel = i == pick_;
			Element row = hbox({ text("   "), t_.ink(sectionEntry(sections_[i]), sel ? Role::Bright : Role::Text), filler() });
			if (sel) row = row | t_.bg(Role::AccentBg);
			rows.push_back(std::move(row));
		}
	} else if (loaded) {
		follow(cursor_, top_, totalRows(), vp_.rows());
		const size_t end = std::min(totalRows(), top_ + static_cast<size_t>(vp_.rows()));
		for (size_t r = top_; r < end; ++r) {
			const std::vector<uint8_t> raw = s_.bytes(r * 16, 16);
			std::string hex, ascii;
			for (size_t c = 0; c < 16; ++c) {
				if (c > 0) hex += ' ';
				if (c == 8) hex += ' ';   // gap between the two 8-byte groups
				if (c < raw.size()) {
					hex += hexUpper(raw[c], 2);
					ascii += (raw[c] >= 0x20 && raw[c] <= 0x7e) ? static_cast<char>(raw[c]) : '.';
				} else {
					hex += "  ";
				}
			}
			const bool sel = r == cursor_;
			const bool hi = sel && focused && at_ == Rows;
			Element row = hbox({
				text(" "), t_.ink(hexUpper(r * 16, 8), hi ? Role::AccentText : (sel ? Role::Bright : Role::Dim)),
				text("  "), t_.ink(hex, hi ? Role::AccentText : (sel ? Role::Bright : Role::Text)),
				text("  "), t_.ink("|" + ascii + "|", hi ? Role::AccentText : (sel ? Role::Bright : Role::Muted)),
				filler(),
			});
			if (hi) row = row | t_.bg(Role::AccentBg);
			rows.push_back(std::move(row));
		}
	}
	out.push_back(vbox(std::move(rows)) | yflex | reflect(vp_.box));
	return vbox(std::move(out));
}

} // namespace tui
