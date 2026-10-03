#include "tui/panes/panes.hpp"

#include "tui/syntax.hpp"

#include <algorithm>

namespace tui {

using namespace ftxui;

void DisassemblyPane::sync() {
	if (gen_ == s_.decodeGeneration()) return;
	gen_ = s_.decodeGeneration();
	cursor_ = top_ = 0;
}

void DisassemblyPane::clamp() {
	follow(cursor_, top_, s_.rowCount(), vp_.rows());
}

ScrollState DisassemblyPane::scroll() const {
	return {s_.rowCount(), top_, static_cast<size_t>(vp_.rows())};
}

void DisassemblyPane::navigateTo(uint64_t vaddr) {
	sync();
	if (s_.rowCount() == 0) return;
	cursor_ = s_.rowIndexFor(vaddr);
	// Centre the row, like scrollTo(..., PositionAtCenter) in the Qt pane.
	const size_t half = static_cast<size_t>(vp_.rows() / 2);
	top_ = cursor_ > half ? cursor_ - half : 0;
	clamp();
}

bool DisassemblyPane::onEvent(const Event& e) {
	sync();
	if (!moveRows(e, cursor_, s_.rowCount(), vp_.rows())) return false;
	clamp();
	return true;
}

bool DisassemblyPane::click(int, int y) {
	sync();
	size_t i = 0;
	if (!rowAt(vp_, y, top_, i) || i >= s_.rowCount()) return false;
	cursor_ = i;
	return true;
}

Element DisassemblyPane::render(bool focused) {
	sync();
	clamp();
	const Glyphs& g = t_.g();
	const Role cap = focused ? Role::Muted : Role::Faint;

	// Columns: 2-cell gutter, ADDRESS 10 (+2), BYTES 20 (+2, seven bytes - a
	// RIP-relative LEA), INSTRUCTION takes the slack, NOTES right-aligned.
	Elements out;
	out.push_back(hbox({
		text("  "), fixed(t_.ink("ADDRESS", cap), 12), fixed(t_.ink("BYTES", cap), 22),
		t_.ink("INSTRUCTION", cap), filler(), t_.ink("NOTES", cap), text("  "),
	}));
	out.push_back(rule(t_));
	if (!s_.banner().empty())
		out.push_back(hbox({ text("  "), paragraph(s_.banner()) | t_.fg(Role::Muted) }));

	const int width = vp_.cols();
	const size_t total = s_.rowCount();
	const size_t end = std::min(total, top_ + static_cast<size_t>(vp_.rows()));
	Elements rows;
	for (size_t i = top_; i < end; ++i) {
		const bool sel = i == cursor_;
		const bool hi = sel && focused;
		const std::string ins = s_.rowText(i);
		const std::string note = s_.rowNote(i);
		const bool branch = s_.rowFlow(i) != Instruction::Flow::None;

		int avail = width - 36 - (note.empty() ? 2 : static_cast<int>(note.size()) + 4);
		if (avail < 1) avail = 1;
		const bool over = static_cast<int>(ins.size()) > avail;
		const int budget = over ? avail - 1 : avail;

		Elements toks;
		int used = 0;
		for (const Token& tk : tokenize(ins, branch)) {
			if (used >= budget) break;
			std::string piece = tk.text;
			if (used + static_cast<int>(piece.size()) > budget)
				piece = piece.substr(0, static_cast<size_t>(budget - used));
			used += static_cast<int>(piece.size());
			// The selected row flattens to bright text: the band carries the emphasis
			// (same rule as DisasmDelegate::paint).
			toks.push_back(t_.ink(piece, sel ? Role::Bright : tk.role));
		}
		if (over) toks.push_back(t_.ink(g.clip, sel ? Role::Bright : Role::SynPunct));

		const Role addrR = hi ? Role::AccentText : (sel ? Role::Bright : Role::Dim);
		const Role bytesR = hi ? Role::AccentText : (sel ? Role::Bright : Role::Faint);
		Element line = hbox({
			text("  "),
			fixed(t_.ink(hex0x(s_.rowVaddr(i), 8), addrR), 12),
			fixed(t_.ink(clip(s_.rowBytes(i), 20, g.clip), bytesR), 22),
			hbox(std::move(toks)),
			filler(),
			t_.ink(note, hi ? Role::Accent : Role::Ghost),
			text("  "),
		});
		if (hi) line = line | t_.bg(Role::AccentBg);
		rows.push_back(std::move(line));
	}
	out.push_back(vbox(std::move(rows)) | yflex | reflect(vp_.box));
	return vbox(std::move(out));
}

} // namespace tui
