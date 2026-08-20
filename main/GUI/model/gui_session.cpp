#include "gui_session.h"

#include "../../miscellaneous/loader.hpp"

#include <cstdio>

namespace gui {

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
const Registers kZeroRegisters{};
const std::vector<uint64_t> kEmptyStack{};

} // namespace

bool Session::open(const std::string& path) {
	std::shared_ptr<AddressSpace> newSpace;
	std::shared_ptr<Disassembler> newDisassembler;
	// std::exception, not runtime_error: AddressSpace throws length_error for
	// malformed files (the TUI's Open path missed that and crashed).
	try {
		newSpace = std::make_shared<AddressSpace>(path);
		make_disassembler(*newSpace, &newDisassembler);
	}
	catch (const std::exception& e) {
		setStatus(std::string("Open failed: ") + e.what());
		return false;
	}

	bool is_elf = false, is_pe = false;
	determine_filetype(*newSpace, is_elf, is_pe);

	// replace both together: the disassembler references the address space
	space_ = std::move(newSpace);
	disassembler_ = std::move(newDisassembler);
	filePath_ = path;
	format_ = is_elf ? "ELF" : "PE";
	setStatus("Loaded " + path);
	runDecode();
	refresh();
	return true;
}

void Session::runDecode() {
	decodeNote_.clear();
	if (!disassembler_) return;
	try {
		disassembler_->decode();
	}
	catch (const std::exception& e) {
		// Partial results survive: whatever decoded before the throw is kept.
		decodeNote_ = e.what();
	}
}

std::string Session::architecture() const {
	return loaded() ? disassembler_->getArchitecture() : "";
}

bool Session::is64bit() const {
	// The core's arch strings are "x86" / "ARM32" / "x86_64" / "AArch64"; the
	// 64-bit ones are exactly the two carrying "64". Format-agnostic.
	return architecture().find("64") != std::string::npos;
}

const Registers& Session::registers() const {
	return loaded() ? disassembler_->getRegisters() : kZeroRegisters;
}

const std::vector<uint64_t>& Session::stack() const {
	return loaded() ? disassembler_->getVirtStack() : kEmptyStack;
}

std::vector<uint8_t> Session::bytes(uint64_t offset, size_t count) const {
	std::vector<uint8_t> out;
	if (!space_) return out;
	size_t max = space_->size();
	if (offset >= max) return out;
	if (count > max - offset) count = max - offset;
	out.reserve(count);
	for (size_t i = 0; i < count; ++i)
		out.push_back(space_->read_u8(offset + i));
	return out;
}

size_t Session::binarySize() const {
	return space_ ? space_->size() : 0;
}

uint64_t Session::textOffset() const {
	return loaded() ? disassembler_->getSections()._text.getOffset() : 0;
}

uint64_t Session::textVaddr() const {
	return loaded() ? disassembler_->getSections()._text.getVaddr() : 0;
}

std::vector<SectionInfo> Session::sections() const {
	std::vector<SectionInfo> out;
	if (!loaded()) return out;
	const Sections& s = disassembler_->getSections();
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
	disasmRows_.clear();
	decodedForReal_ = false;
	if (!loaded()) return;

	const auto& decoded = disassembler_->getDecodedInstructions();
	const auto& addresses = disassembler_->getInstructionAddresses();
	const Header& text = disassembler_->getSections()._text;
	const uint64_t textEnd = text.getOffset() + text.getSize();

	if (!decoded.empty()) {
		decodedForReal_ = true;
		disasmRows_.reserve(decoded.size());
		for (size_t i = 0; i < decoded.size(); ++i) {
			DisasmRow row;
			// addresses is parallel to decoded, but stay defensive: a
			// decodeLine that throws mid-push could leave it short.
			row.vaddr = (i < addresses.size()) ? addresses[i] : 0;
			row.text = formatDisasmText(decoded[i]->decodeLineString());

			// Instruction length = distance to the next instruction (the core
			// records start addresses only). Last one is clamped to .text end.
			if (i < addresses.size() && row.vaddr >= text.getVaddr()) {
				uint64_t off = text.getOffset() + (row.vaddr - text.getVaddr());
				uint64_t next = (i + 1 < addresses.size())
					? text.getOffset() + (addresses[i + 1] - text.getVaddr())
					: textEnd;
				if (next > off && off < textEnd) {
					uint64_t len = next - off;
					if (len > 16) len = 16; // desynced decode; don't dump runs
					auto raw = bytes(off, static_cast<size_t>(len));
					for (size_t b = 0; b < raw.size(); ++b)
						row.bytes += (b ? " " : "") + hexByte(raw[b]);
				}
			}
			disasmRows_.push_back(std::move(row));
		}
		return;
	}

	// Fallback: decoder for this arch is a stub — show raw .text bytes, 8 per
	// row, so the pane still has content. decodeNote()/decodedForReal() tell
	// the pane to explain why. Capped so a huge .text can't flood the model.
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
		// The Instruction column needs text here too. Leaving it empty renders as
		// a blank column for the whole table, which reads as a broken pane rather
		// than as "no decoder for this arch" - the banner says the latter. "db" is
		// the usual spelling for a run of bytes no decoder claimed.
		row.text = "db ";
		for (size_t b = 0; b < n; ++b)
			row.text += (b ? ", 0x" : "0x") + hexByte(raw[i + b]);
		disasmRows_.push_back(std::move(row));
	}
}

} // namespace gui
