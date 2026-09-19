#include "tui/session.hpp"

#include "disassembler/format/detect.hpp"

#include <cstdio>

namespace tui {

using voidwalk::make_disassembler;
using voidwalk::determine_filetype;


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
                 std::string filePath) {
	std::string format;
	if (disassembler) {
		bool is_elf = false, is_pe = false;
		determine_filetype(*space, is_elf, is_pe);
		format = is_elf ? "ELF" : (is_pe ? "PE" : "?");
	}
	adopt(std::move(space), std::move(disassembler), std::move(filePath), std::move(format));
	refresh();
}

// Drops the previous sweep's rows so refresh() appends from zero.
void Session::onDecodeStarted() {
	disasmLines_.clear();
	builtInstrs_ = 0;
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
