#include "session.h"

#include "../../miscellaneous/loader.h"

#include <cstdio>

namespace tui {

namespace {

std::string hex64(uint64_t v) {
	char buf[19];
	std::snprintf(buf, sizeof(buf), "0x%016llx", static_cast<unsigned long long>(v));
	return buf;
}

std::string hexAddr(uint64_t v) {
	char buf[11];
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

bool Session::open(const std::string& path) {
	std::shared_ptr<AddressSpace> newSpace;
	std::shared_ptr<Disassembler> newDisassembler;
	try {
		newSpace = std::make_shared<AddressSpace>(path);
		make_disassembler(*newSpace, &newDisassembler);
	}
	catch (std::runtime_error& e) {
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
	disasmLines_.clear();
	regRows_.clear();
	stackRows_.clear();

	if (!loaded()) {
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
	if (!decoded.empty()) {
		for (size_t i = 0; i < decoded.size(); ++i) {
			// addresses is parallel to decoded, but stay defensive: a decodeLine
			// that throws mid-push could leave it short.
			std::string addr = (i < addresses.size()) ? hexAddr(addresses[i]) + "  "
			                                          : std::string(12, ' ');
			disasmLines_.push_back("  " + addr + decoded[i]->decodeLineString());
		}
		if (!decodeNote_.empty())
			disasmLines_.push_back("  ... decode stopped: " + decodeNote_);
	}
	else {
		const Header& text = disassembler_->getSections()._text;
		uint64_t size = text.getSize();
		if (size == 0) {
			disasmLines_.push_back("  [.text section not found or empty]");
		}
		else {
			disasmLines_.push_back(decodeNote_.empty()
				? "  [no instructions decoded - decodeLine() is a stub for " + architecture() + "]"
				: "  [decode failed: " + decodeNote_ + "]");
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
	const Registers& r = disassembler_->getRegisters();
	regRows_.push_back("eax " + hex64(r.eax));
	regRows_.push_back("ebx " + hex64(r.ebx));
	regRows_.push_back("ecx " + hex64(r.ecx));
	regRows_.push_back("edx " + hex64(r.edx));
	regRows_.push_back("esi " + hex64(r.esi));
	regRows_.push_back("edi " + hex64(r.edi));
	regRows_.push_back("ebp " + hex64(r.ebp));
	regRows_.push_back("esp " + hex64(r.esp));
	regRows_.push_back("eip " + hex64(r.eip));
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
