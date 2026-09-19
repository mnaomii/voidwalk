#include "gui/session.hpp"

#include "disassembler/format/detect.hpp"

#include <cstdio>

namespace gui {

using voidwalk::make_disassembler;
using voidwalk::determine_filetype;


namespace {

std::string hexByte(uint8_t b) {
	char buf[4];
	std::snprintf(buf, sizeof(buf), "%02X", b);
	return buf;
}

// The core renders a decoded line as "<MNEMONIC> \t<operands>" — a literal space
// followed by a tab (see x86_64::decode). The tab draws unevenly in the Qt
// delegate (QPainter gives it a font-dependent, near-random advance) and leaves
// trailing whitespace on operand-less rows like RET. We can't touch the decoder,
// so tidy the string here: split on the tab, trim both halves, and re-join with
// the mnemonic left-padded to a fixed column so operands line up down the pane.
std::string formatDisasmText(const std::string& raw) {
	std::string mnemonic, operands;
	const auto tab = raw.find('\t');
	if (tab == std::string::npos) {
		mnemonic = raw; // "(bad)" and the like never reach the tab-append
	}
	else {
		mnemonic = raw.substr(0, tab);
		operands = raw.substr(tab + 1);
	}

	auto trim = [](std::string& s) {
		const auto first = s.find_first_not_of(" \t");
		if (first == std::string::npos) { s.clear(); return; }
		const auto last = s.find_last_not_of(" \t");
		s = s.substr(first, last - first + 1);
	};
	trim(mnemonic);
	trim(operands);

	if (operands.empty()) return mnemonic;

	// Pad the mnemonic (possibly "REP MOVSB", prefix included) to a fixed width so
	// the operand columns align; longer fields just get a single separating space.
	constexpr std::size_t kColumn = 7;
	if (mnemonic.size() < kColumn)
		mnemonic.append(kColumn - mnemonic.size(), ' ');
	else
		mnemonic.push_back(' ');
	return mnemonic + operands;
}

// Registers to hand out when nothing is loaded, so panes can render a
// zeroed register file without null-checking the disassembler.
const Registers_x86_64 kZeroRegisters{};
const std::vector<uint64_t> kEmptyStack{};

// The two reads Session and Snapshot both do, factored out so the UI-thread and
// the worker-thread paths can never drift apart. Both are pure reads of core
// state that a running decode does not mutate (the section table is filled
// before decode() starts; the mapping is fixed for the file's lifetime).
std::vector<SectionInfo> sectionsOf(const Disassembler& d) {
	std::vector<SectionInfo> out;
	const Sections& s = d.getSections();
	auto add = [&out](const char* name, const Header& h) {
		// Drop sections the parser never filled in (offset and size both zero) so
		// the pane doesn't list a jump target that goes nowhere.
		if (h.getOffset() == 0 && h.getSize() == 0) return;
		out.push_back({name, h.getOffset(), h.getVaddr(), h.getSize()});
	};
	add(".text", s._text);
	add(".data", s._data);
	add(".rodata", s._ronly);
	add(".bss", s._bss);
	return out;
}

std::vector<uint8_t> bytesOf(AddressSpace& space, uint64_t offset, size_t count) {
	std::vector<uint8_t> out;
	size_t max = space.size();
	if (offset >= max) return out;
	if (count > max - offset) count = max - offset;
	out.reserve(count);
	for (size_t i = 0; i < count; ++i)
		out.push_back(space.read_u8(offset + i));
	return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Snapshot — the off-thread view. Every accessor is bounded by rows_, the count
// that had been published when Session::snapshot() took it, and reads only
// through the shared_ptrs it owns, so nothing here depends on the Session still
// existing or still pointing at the same binary.
// ---------------------------------------------------------------------------

uint64_t Snapshot::rowVaddr(size_t i) const {
	if (!disassembler_ || i >= rows_) return 0;
	return disassembler_->getInstructionAddresses()[i];
}

std::string Snapshot::rowText(size_t i) const {
	if (!disassembler_ || i >= rows_) return {};
	return formatDisasmText(disassembler_->getDecodedInstructions()[i]->decodeLineString());
}

uint64_t Snapshot::textVaddr() const {
	return disassembler_ ? disassembler_->getSections()._text.getVaddr() : 0;
}

std::vector<SectionInfo> Snapshot::sections() const {
	if (!disassembler_) return {};
	return sectionsOf(*disassembler_);
}

std::vector<uint8_t> Snapshot::bytes(uint64_t offset, size_t count) const {
	if (!space_) return {};
	return bytesOf(*space_, offset, count);
}

// Drops the previous sweep's view state and bumps the generation counter so the
// disassembly view resets rather than diffing row counts. Rows are read through to
// the core, so there is nothing else to drop.
void Session::onDecodeStarted() {
	shownRows_ = 0;
	fallbackRows_.clear();
	decodedForReal_ = false;
	++decodeGen_;
}

const Registers_x86_64& Session::registers() const {
	return loaded() ? disassembler_->getRegisters() : kZeroRegisters;
}

const std::vector<uint64_t>& Session::stack() const {
	return loaded() ? disassembler_->getVirtStack() : kEmptyStack;
}

std::vector<SectionInfo> Session::sections() const {
	if (!loaded()) return {};
	return sectionsOf(*disassembler_);
}

Snapshot Session::snapshot() const {
	Snapshot s;
	if (!loaded()) return s;
	s.space_ = space_;
	s.disassembler_ = disassembler_;
	// Only real decoded rows: the stub-arch fallback lives in fallbackRows_, which
	// is a UI-thread-only vector and carries no instructions to walk anyway.
	s.rows_ = decodedForReal_ ? shownRows_ : 0;
	return s;
}

std::string Session::applyPatches(const std::vector<std::pair<size_t, std::string>>& edits) {
	if (!loaded())
		return "Recompile: no binary loaded.";
	if (edits.empty())
		return "Recompile: no edited instructions - nothing to do.";
	// Assembler backend WIP: text -> machine code re-encoding does not exist
	// yet, so report instead of silently pretending. Edits stay pending in the
	// disassembly pane so they survive until the backend lands.
	return "Recompile: assembler backend not implemented yet - "
	       + std::to_string(edits.size()) + " edit(s) left pending.";
}

void Session::refresh() {
	if (!loaded()) {
		decodedForReal_ = false;
		shownRows_ = 0;
		fallbackRows_.clear();
		return;
	}

	// Only [0, ready) is safe to read while decode() runs on the worker; this is
	// the acquire that pairs with the worker's release store of readyCount.
	const size_t ready = disassembler_->readyInstructions();
	if (ready > 0) {
		decodedForReal_ = true;
		shownRows_ = ready; // expose exactly the published rows; row*() read through
		return;
	}

	// Nothing decoded yet. While the worker is still running, show an empty pane for
	// this frame rather than flashing the "no decoder for this arch" fallback; the
	// next poll will have rows. The fallback below is only for a genuine stub.
	if (isDecoding()) return;

	// Decode finished with zero instructions -> unimplemented-arch fallback.
	decodedForReal_ = false;
	buildFallback();
}

// Raw-bytes fallback for a stub arch: no Instruction objects exist to read
// through, so build a small capped set of "db 0x.." rows here. Bounded, so a huge
// .text can't flood it. decodeNote()/decodedForReal() tell the pane to explain why.
void Session::buildFallback() {
	fallbackRows_.clear();
	const Header& text = disassembler_->getSections()._text;
	if (text.getSize() == 0) return;
	constexpr uint64_t kMaxBytes = 4096;
	uint64_t total = text.getSize() < kMaxBytes ? text.getSize() : kMaxBytes;
	auto raw = bytes(text.getOffset(), static_cast<size_t>(total));
	for (size_t i = 0; i < raw.size(); i += 8) {
		DisasmRow row;
		row.vaddr = text.getVaddr() + i;
		size_t n = raw.size() - i < 8 ? raw.size() - i : 8;
		for (size_t b = 0; b < n; ++b)
			row.bytes += (b ? " " : "") + hexByte(raw[i + b]);
		// "db" is the usual spelling for a run of bytes no decoder claimed; leaving
		// the text empty would read as a broken pane rather than "no decoder here".
		row.text = "db ";
		for (size_t b = 0; b < n; ++b)
			row.text += (b ? ", 0x" : "0x") + hexByte(raw[i + b]);
		fallbackRows_.push_back(std::move(row));
	}
}

size_t Session::rowCount() const {
	if (!loaded()) return 0;
	return decodedForReal_ ? shownRows_ : fallbackRows_.size();
}

uint64_t Session::rowVaddr(size_t i) const {
	if (!loaded()) return 0;
	if (!decodedForReal_)
		return i < fallbackRows_.size() ? fallbackRows_[i].vaddr : 0;
	if (i >= shownRows_) return 0; // never read past what the worker published
	return disassembler_->getInstructionAddresses()[i];
}

std::string Session::rowBytes(size_t i) const {
	if (!loaded()) return {};
	if (!decodedForReal_)
		return i < fallbackRows_.size() ? fallbackRows_[i].bytes : std::string{};
	if (i >= shownRows_) return {};
	// Straight from the decoder's recorded machine code — no file IO, no core copy.
	std::string b = disassembler_->getDecodedInstructions()[i]->getMachineCode();
	while (!b.empty() && b.back() == ' ') b.pop_back(); // trim the trailing separator
	return b;
}

std::string Session::rowText(size_t i) const {
	if (!loaded()) return {};
	if (!decodedForReal_)
		return i < fallbackRows_.size() ? fallbackRows_[i].text : std::string{};
	if (i >= shownRows_) return {};
	return formatDisasmText(disassembler_->getDecodedInstructions()[i]->decodeLineString());
}

} // namespace gui
