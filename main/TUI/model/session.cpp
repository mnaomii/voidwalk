#include "session.h"

#include "../../miscellaneous/loader.hpp"

#include <cstdio>

namespace tui {

namespace {

std::string hex64(uint64_t v) {
	char buf[19];
	std::snprintf(buf, sizeof(buf), "0x%016llx", static_cast<unsigned long long>(v));
	return buf;
}

// 8 digits is the minimum width, not the maximum: the buffer has to hold a full
// 64-bit address ("0x" + 16 digits + NUL) or snprintf truncates it to something
// that still looks like a valid address - a PE32+ 0x140001000 became 0x14000100.
std::string hexAddr(uint64_t v) {
	char buf[19];
	std::snprintf(buf, sizeof(buf), "0x%08llx", static_cast<unsigned long long>(v));
	return buf;
}

} // namespace

Session::Session(std::shared_ptr<AddressSpace> space,
                 std::shared_ptr<Disassembler> disassembler,
                 std::string filePath)
	: space_(std::move(space)), disassembler_(std::move(disassembler)),
	  filePath_(std::move(filePath)) {
	if (disassembler_) {
		bool is_elf = false, is_pe = false;
		determine_filetype(*space_, is_elf, is_pe);
		format_ = is_elf ? "ELF" : (is_pe ? "PE" : "?");
	}
	runDecode();
	refresh();
}

void Session::runDecode() {
	if (!disassembler_) return;
	if (!decodeState_) decodeState_ = std::make_shared<DecodeState>();
	decodeState_->note.clear();
	// New sweep: drop the previous binary's lines so refresh() appends from zero.
	disasmLines_.clear();
	builtInstrs_ = 0;
	decodeState_->running.store(true, std::memory_order_release);

	// Capture shared_ptr copies (never `this` — the Session gets moved into the UI):
	// they keep the disassembler and its address space alive for the worker
	// regardless of where the Session lives or a later re-open. Move-assigning the
	// jthread stops+joins any previous decode; decode() polls the stop_token.
	decodeThread_ = std::jthread(
		[disasm = disassembler_, space = space_, state = decodeState_](std::stop_token st) {
			(void)space; // held only to keep the AddressSpace alive under the worker
			try {
				disasm->decode(st);
			}
			catch (const std::exception& e) {
				// Partial results survive: whatever decoded before the throw is kept.
				state->note = e.what();
			}
			// Release so a reader that sees running==false also sees note above.
			state->running.store(false, std::memory_order_release);
		});
}

bool Session::open(const std::string& path) {
	std::shared_ptr<AddressSpace> newSpace;
	std::shared_ptr<Disassembler> newDisassembler;
	try {
		newSpace = std::make_shared<AddressSpace>(path);
		make_disassembler(*newSpace, &newDisassembler);
	}
	// std::exception, not runtime_error: AddressSpace::initialize throws
	// length_error (a logic_error) for a truncated or malformed file, which a
	// runtime_error catch lets through - straight out of the FTXUI event loop.
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

std::string Session::architecture() const {
	return loaded() ? disassembler_->getArchitecture() : "";
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
	if (!loaded()) return 0;
	return disassembler_->getSections()._text.getOffset();
}

void Session::refresh() {
	regRows_.clear();
	stackRows_.clear();

	if (!loaded()) {
		disasmLines_.clear();
		builtInstrs_ = 0;
		disasmLines_.push_back("  [no binary loaded - use Open]");
		regRows_.push_back("  [no binary loaded]");
		stackRows_.push_back("  [no binary loaded]");
		return;
	}

	// --- disassembly ---------------------------------------------------
	// Real path: the strings the mnemonic layer produced during runDecode().
	// Fallback path: the decoder for this arch is still a stub, so show the raw
	// .text bytes and say why nothing decoded rather than silently showing "??".
	const auto& decoded = disassembler_->getDecodedInstructions();
	const auto& addresses = disassembler_->getInstructionAddresses();
	// Only [0, ready) is safe to read while decode() runs on the worker; this
	// acquire pairs with the worker's release store of readyCount.
	const size_t ready = disassembler_->readyInstructions();
	// The note is only safe to read once the worker has stopped (see isDecoding).
	const std::string note = (!isDecoding() && decodeState_) ? decodeState_->note : std::string();
	if (ready > 0) {
		// APPEND-ONLY. The instruction lines in [0, builtInstrs_) were built on a
		// previous poll and never change, so drop only the trailing status line from
		// last time (resize to builtInstrs_) and append the new instructions. This is
		// what stops every frame during decode from rebuilding the whole vector —
		// the O(n²) that made a large binary crawl in the TUI.
		disasmLines_.resize(builtInstrs_);
		for (size_t i = builtInstrs_; i < ready; ++i) {
			// Both vectors have >= ready elements (readyCount is published only once
			// they are consistent), so index directly. Do NOT call addresses.size()
			// here — that would race the worker's push_back on the same vector.
			std::string addr = hexAddr(addresses[i]) + "  ";
			disasmLines_.push_back("  " + addr + decoded[i]->decodeLineString());
		}
		builtInstrs_ = ready;
		if (isDecoding())
			disasmLines_.push_back("  ... decoding");
		else if (!note.empty())
			disasmLines_.push_back("  ... decode stopped: " + note);
	}
	else if (isDecoding()) {
		disasmLines_.clear();
		builtInstrs_ = 0;
		disasmLines_.push_back("  [decoding...]");
	}
	else {
		disasmLines_.clear();
		builtInstrs_ = 0;
		const Header& text = disassembler_->getSections()._text;
		uint64_t size = text.getSize();
		if (size == 0) {
			disasmLines_.push_back("  [.text section not found or empty]");
		}
		else {
			disasmLines_.push_back(note.empty()
				? "  [no instructions decoded - decodeLine() is a stub for " + architecture() + "]"
				: "  [decode failed: " + note + "]");
			disasmLines_.push_back("  [raw .text bytes follow]");
			disasmLines_.push_back("");

			constexpr uint64_t kMaxPlaceholderRows = 512;
			uint64_t rows = size < kMaxPlaceholderRows ? size : kMaxPlaceholderRows;
			auto raw = bytes(text.getOffset(), rows);
			uint64_t vaddr = text.getVaddr();
			for (size_t i = 0; i < raw.size(); ++i) {
				char line[64];
				std::snprintf(line, sizeof(line), "%s  %02X", hexAddr(vaddr + i).c_str(), raw[i]);
				disasmLines_.push_back(line);
			}
			if (size > rows)
				disasmLines_.push_back("  ... (" + std::to_string(size - rows) + " more bytes)");
		}
	}

	// --- registers ------------------------------------------------------
	// Values are the core's emulated Registers struct (all zero until the
	// debugger/emulator exists) - real plumbing, placeholder semantics.
	const Registers_x86_64& r = disassembler_->getRegisters();
	regRows_.push_back("eax " + hex64(r.rax));
	regRows_.push_back("ebx " + hex64(r.rbx));
	regRows_.push_back("ecx " + hex64(r.rcx));
	regRows_.push_back("edx " + hex64(r.rdx));
	regRows_.push_back("esi " + hex64(r.rsi));
	regRows_.push_back("edi " + hex64(r.rdi));
	regRows_.push_back("ebp " + hex64(r.rbp));
	regRows_.push_back("esp " + hex64(r.rsp));
	regRows_.push_back("eip " + hex64(r.rip));
	regRows_.push_back("");
	regRows_.push_back("cs  " + hex64(r.cs) + "   ds  " + hex64(r.ds));
	regRows_.push_back("ss  " + hex64(r.ss) + "   es  " + hex64(r.es));
	regRows_.push_back("fs  " + hex64(r.fs) + "   gs  " + hex64(r.gs));
	regRows_.push_back("");
	{
		char flagRow[32];
		std::snprintf(flagRow, sizeof(flagRow), "flags 0x%02X", r.flags);
		regRows_.push_back(flagRow);
	}
	regRows_.push_back("[emulated - debugger WIP]");

	// --- stack ------------------------------------------------------------
	// virtStack is the core's simulated stack; empty until execution exists.
	const auto& stack = disassembler_->getVirtStack();
	if (stack.empty()) {
		stackRows_.push_back("  <empty>");
		stackRows_.push_back("");
		stackRows_.push_back("  [stack simulation WIP -");
		stackRows_.push_back("   fills once the debugger");
		stackRows_.push_back("   can execute instructions]");
	}
	else {
		for (size_t i = stack.size(); i-- > 0;) {
			std::string marker = (i == stack.size() - 1) ? " <- esp" : "";
			stackRows_.push_back(hexAddr(i * 8) + "  " + hex64(stack[i]) + marker);
		}
	}
}

} // namespace tui
