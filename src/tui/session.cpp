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

// Drops the previous sweep's rows.
void Session::onDecodeStarted() {
	shownInstrs_ = 0;
	extraLines_.clear();
}

std::string Session::disassemblyRow(size_t i) const {
	if (i >= shownInstrs_)
		return i - shownInstrs_ < extraLines_.size() ? extraLines_[i - shownInstrs_] : std::string();
	// i < shownInstrs_ <= the worker's published count, so both stores hold row i.
	return "  " + hexAddr(disassembler_->getInstructionAddresses()[i]) + "  "
		+ disassembler_->getDecodedInstructions()[i]->decodeLineString();
}

void Session::refresh() {
	throwIfFileChanged();
	regRows_.clear();
	for (auto& rows : vecRows_) rows.clear();
	stackRows_.clear();
	extraLines_.clear();
	shownInstrs_ = 0;

	if (!loaded()) {
		extraLines_.push_back("  [no binary loaded - use Open]");
		regRows_.push_back("  [no binary loaded]");
		stackRows_.push_back("  [no binary loaded]");
		return;
	}

	// --- disassembly ---------------------------------------------------
	// Real path: the strings the mnemonic layer produced during runDecode().
	// Fallback path: the decoder for this arch is still a stub, so show the raw
	// .text bytes and say why nothing decoded rather than silently showing "??".
	// Only [0, ready) is safe to read while decode() runs on the worker; this
	// acquire pairs with the worker's release store of readyCount.
	const size_t ready = disassembler_->readyInstructions();
	// The note is only safe to read once the worker has stopped (see isDecoding).
	const std::string note = (!isDecoding() && decodeState_) ? decodeState_->note : std::string();
	if (ready > 0) {
		shownInstrs_ = ready;   // the rows themselves are read through on demand
		if (isDecoding())
			extraLines_.push_back("  ... decoding");
		else if (!note.empty())
			extraLines_.push_back("  ... decode stopped: " + note);
	}
	else if (isDecoding()) {
		extraLines_.push_back("  [decoding...]");
	}
	else {
		// Every .text section, in table order. The row cap is shared across all of them.
		const std::vector<Header>& texts = disassembler_->getSections().text;
		uint64_t size = 0;
		for (const Header& text : texts)
			size += text.getSize();
		if (size == 0) {
			extraLines_.push_back("  [.text section not found or empty]");
		}
		else {
			extraLines_.push_back(note.empty()
				? "  [no instructions decoded - decodeLine() is a stub for " + architecture() + "]"
				: "  [decode failed: " + note + "]");
			extraLines_.push_back("  [raw .text bytes follow]");
			extraLines_.push_back("");

			constexpr uint64_t kMaxPlaceholderRows = 512;
			uint64_t rows = 0; // bytes requested so far, across sections
			for (const Header& text : texts) {
				if (rows == kMaxPlaceholderRows) break;
				const uint64_t budget = kMaxPlaceholderRows - rows;
				const uint64_t want = text.getSize() < budget ? text.getSize() : budget;
				auto raw = bytes(text.getOffset(), want);
				uint64_t vaddr = text.getVaddr();
				for (size_t i = 0; i < raw.size(); ++i) {
					char line[64];
					std::snprintf(line, sizeof(line), "%s  %02X", hexAddr(vaddr + i).c_str(), raw[i]);
					extraLines_.push_back(line);
				}
				rows += want;
			}
			if (size > rows)
				extraLines_.push_back("  ... (" + std::to_string(size - rows) + " more bytes)");
		}
	}

	// --- registers ------------------------------------------------------
	// Values are the core's emulated Registers struct (all zero until the
	// debugger/emulator exists) - real plumbing, placeholder semantics.
	const Registers_x86_64& r = disassembler_->getRegisters();
	const bool wide = is64bit();   // rax.. for a 64-bit target, eax.. otherwise
	auto reg = [&](const char* name, uint64_t v) {
		regRows_.push_back((wide ? "r" : "e") + std::string(name) + " " + hex64(v));
	};
	reg("ax", r.rax); reg("bx", r.rbx); reg("cx", r.rcx); reg("dx", r.rdx);
	reg("si", r.rsi); reg("di", r.rdi); reg("bp", r.rbp); reg("sp", r.rsp);
	reg("ip", r.rip);
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

	// Vector registers: one wide hex number each, most significant qword first.
	for (size_t i = 0; i < voidwalk::kMmRegs.size(); ++i)
		vecRows_[0].push_back("mm" + std::to_string(i) + " " + hex64(r.*voidwalk::kMmRegs[i]));
	auto vecs = [&]<size_t N, size_t M>(std::vector<std::string>& rows, const char* prefix,
	                                    const std::array<voidwalk::ExtendedType<N> Registers_x86_64::*, M>& regs) {
		for (size_t i = 0; i < M; ++i) {
			std::string row = prefix + std::to_string(i) + " 0x";
			for (size_t q = N / 8; q-- > 0;)
				row += hex64((r.*regs[i]).qword(q)).substr(2);
			rows.push_back(std::move(row));
		}
	};
	vecs(vecRows_[1], "xmm", voidwalk::kXmmRegs);
	vecs(vecRows_[2], "ymm", voidwalk::kYmmRegs);
	vecs(vecRows_[3], "zmm", voidwalk::kZmmRegs);

	// --- stack ------------------------------------------------------------
	// The core no longer keeps a simulated stack (Disassembler::virtStack was
	// removed), so there is nothing to list until execution exists.
	stackRows_.push_back("  <empty>");
	stackRows_.push_back("");
	stackRows_.push_back("  [stack simulation WIP -");
	stackRows_.push_back("   fills once the debugger");
	stackRows_.push_back("   can execute instructions]");
}

} // namespace tui
