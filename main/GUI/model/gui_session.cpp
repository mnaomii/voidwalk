#include "gui_session.h"

#include "../../miscellaneous/loader.h"

#include <cstdio>

namespace gui {

namespace {

std::string hexByte(uint8_t b) {
	char buf[4];
	std::snprintf(buf, sizeof(buf), "%02X", b);
	return buf;
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
			row.text = decoded[i]->decodeLineString();

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
		disasmRows_.push_back(std::move(row));
	}
}

} // namespace gui
