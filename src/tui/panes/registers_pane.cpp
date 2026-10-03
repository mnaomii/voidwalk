#include "tui/panes/panes.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace tui {

using namespace ftxui;

namespace {
const char* const kTitles[8] = {
	"General Purpose", "Instruction Pointer", "Segment", "Flags", "MMX", "SSE", "AVX", "AVX-512",
};
} // namespace

std::vector<RegistersPane::Row> RegistersPane::build() const {
	const Registers_x86_64& r = s_.registers();
	const bool is64 = s_.is64bit();
	const int gp = is64 ? 16 : 8;
	std::vector<Row> rows;
	auto cat = [&](int c) { rows.push_back({c, {}, {}, {}, {}}); return open_[c]; };
	auto reg = [&](std::string name, uint64_t v, int digits) { rows.push_back({-1, std::move(name), hex0x(v, digits), {}, {}}); };

	if (cat(0)) {
		const std::array<std::pair<const char*, uint64_t>, 8> gpr = {{
			{"ax", r.rax}, {"bx", r.rbx}, {"cx", r.rcx}, {"dx", r.rdx},
			{"si", r.rsi}, {"di", r.rdi}, {"bp", r.rbp}, {"sp", r.rsp},
		}};
		for (const auto& [n, v] : gpr) reg(std::string(is64 ? "r" : "e") + n, v, gp);
	}
	if (cat(1)) reg(is64 ? "rip" : "eip", r.rip, gp);
	if (cat(2)) {
		// Two selectors per row: six 16-bit values don't need six rows of a 32-cell dock.
		const std::pair<const char*, uint64_t> segs[] = {
			{"cs", r.cs}, {"ds", r.ds}, {"ss", r.ss}, {"es", r.es}, {"fs", r.fs}, {"gs", r.gs},
		};
		for (int i = 0; i < 6; i += 2)
			rows.push_back({-1, segs[i].first, hex0x(segs[i].second, 4), segs[i + 1].first, hex0x(segs[i + 1].second, 4)});
	}
	if (cat(3)) reg("flags", r.flags, 2);
	if (cat(4))
		for (size_t i = 0; i < voidwalk::kMmRegs.size(); ++i)
			reg("mm" + std::to_string(i), r.*voidwalk::kMmRegs[i], 16);

	auto vecs = [&]<size_t N, size_t M>(int c, const char* prefix,
	                                    const std::array<voidwalk::ExtendedType<N> Registers_x86_64::*, M>& regs) {
		if (!cat(c)) return;
		for (size_t i = 0; i < M; ++i) {
			std::string v = "0x";
			for (size_t q = N / 8; q-- > 0;)
				v += hex0x((r.*regs[i]).qword(q), 16).substr(2);
			rows.push_back({-1, prefix + std::to_string(i), std::move(v), {}, {}});
		}
	};
	vecs(5, "xmm", voidwalk::kXmmRegs);
	vecs(6, "ymm", voidwalk::kYmmRegs);
	vecs(7, "zmm", voidwalk::kZmmRegs);
	return rows;
}

ScrollState RegistersPane::scroll() const {
	return {total_, top_, static_cast<size_t>(vp_.rows())};
}

bool RegistersPane::onEvent(const Event& e) {
	const std::vector<Row> rows = build();
	total_ = rows.size();
	if (moveRows(e, cursor_, total_, vp_.rows())) return true;
	if (cursor_ >= rows.size()) return false;
	const Row& row = rows[cursor_];
	if (row.category >= 0) {
		bool& open = open_[row.category];
		if (e == Event::Return || e == Event::Character(' ')) { open = !open; return true; }
		if (e == Event::ArrowRight) { open = true; return true; }
		if (e == Event::ArrowLeft) { open = false; return true; }
	} else if (e == Event::ArrowLeft) {
		while (cursor_ > 0 && rows[cursor_].category < 0) --cursor_;   // up to the group
		return true;
	}
	return false;
}

bool RegistersPane::click(int, int y) {
	const std::vector<Row> rows = build();
	size_t i = 0;
	if (!rowAt(vp_, y, top_, i) || i >= rows.size()) return false;
	cursor_ = i;
	if (rows[i].category >= 0) open_[rows[i].category] = !open_[rows[i].category];
	return true;
}

Element RegistersPane::render(bool focused) {
	const Glyphs& g = t_.g();
	const std::vector<Row> rows = build();
	total_ = rows.size();
	follow(cursor_, top_, total_, vp_.rows());
	const Role cap = focused ? Role::Muted : Role::Faint;

	Elements out;
	out.push_back(hbox({ text(" "), fixed(t_.ink("Register", cap), 9), t_.ink("Value", cap) }));

	Elements lines;
	const size_t end = std::min(rows.size(), top_ + static_cast<size_t>(vp_.rows()));
	for (size_t i = top_; i < end; ++i) {
		const Row& r = rows[i];
		const bool sel = focused && i == cursor_;
		Element line;
		if (r.category >= 0) {
			line = hbox({
				text(" "), t_.ink(open_[r.category] ? g.open : g.closed, Role::Muted), text(" "),
				t_.ink(kTitles[r.category], sel ? Role::Bright : Role::Text) | bold, filler(),
			});
		} else if (!r.name2.empty()) {
			line = hbox({
				text("   "), fixed(t_.ink(r.name, Role::Muted), 3), fixed(t_.ink(r.value, sel ? Role::AccentText : Role::Text), 9),
				fixed(t_.ink(r.name2, Role::Muted), 3), t_.ink(r.value2, sel ? Role::AccentText : Role::Text), filler(),
			});
		} else {
			line = hbox({
				text("   "), fixed(t_.ink(r.name, sel ? Role::Bright : Role::Muted), 7),
				t_.ink(r.value, sel ? Role::AccentText : Role::Text), filler(),
			});
		}
		if (sel) line = line | t_.bg(Role::AccentBg);
		lines.push_back(std::move(line));
	}
	out.push_back(vbox(std::move(lines)) | yflex | reflect(vp_.box));
	return vbox(std::move(out));
}

} // namespace tui
