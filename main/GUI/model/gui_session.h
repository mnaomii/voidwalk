#ifndef GUI_SESSION_H
#define GUI_SESSION_H

#include "../../address-space/address_space.hpp"
#include "../../disassembler/disassembler.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gui {

// One decoded instruction as the GUI shows it. `bytes` is the hex dump of the
// instruction's own bytes ("8B 4D 08"); empty when the raw-bytes fallback is
// active and a row covers a byte run instead of a real instruction.
struct DisasmRow {
	uint64_t vaddr = 0;
	std::string bytes;
	std::string text;
};

// One section the core registered, as the memory pane needs it: a name plus the
// file offset (where the memory pane seeks) and the size. Handed out by
// Session::sections() so the pane can offer a jump target per section without
// touching the Disassembler.
struct SectionInfo {
	std::string name;
	uint64_t offset = 0;
	uint64_t vaddr = 0;
	uint64_t size = 0;
};

// View-model between the analysis core and the Qt panes — same seam as
// tui::Session, but with structured rows instead of preformatted strings so
// the item models can put address/bytes/mnemonic in separate columns.
// Panes and models never touch AddressSpace/Disassembler directly.
//
// Lifetime rule (inherited from the core): disassembler_ holds an
// AddressSpace&, so space_ and disassembler_ are always replaced together.
class Session {
public:
	Session() = default;

	// Loads a binary. On failure returns false, keeps the previous binary
	// (if any) and puts the reason in status().
	bool open(const std::string& path);

	bool loaded() const { return disassembler_ != nullptr; }
	const std::string& filePath() const { return filePath_; }
	const std::string& format() const { return format_; } // "ELF" / "PE" / ""
	std::string architecture() const;
	// True when the loaded target is 64-bit (x86_64 / AArch64). The registers pane
	// uses it to show the 64-bit register names (rax/rbx…/rip).
	bool is64bit() const;

	const std::string& status() const { return status_; }
	void setStatus(std::string s) { status_ = std::move(s); }

	// Why the decode pass produced nothing / stopped early ("" if it ran clean).
	// The disassembly pane surfaces this as a banner instead of a silent "??".
	const std::string& decodeNote() const { return decodeNote_; }
	// True when disassembly() holds real decoded instructions, false when it
	// holds the raw-bytes fallback (unimplemented arch decoder).
	bool decodedForReal() const { return decodedForReal_; }

	const std::vector<DisasmRow>& disassembly() const { return disasmRows_; }

	// Core's emulated register file (all zero until the debugger exists) and
	// simulated stack (empty until execution exists). Valid only when loaded().
	const Registers& registers() const;
	const std::vector<uint64_t>& stack() const;

	// Raw file bytes for the memory pane (clamped at EOF; empty if not loaded).
	// NOTE: memory pane addresses are *file offsets*, disassembly addresses are
	// *virtual addresses* — same known inconsistency as the TUI, rational until
	// a debugger provides a loaded image.
	std::vector<uint8_t> bytes(uint64_t offset, size_t count) const;
	size_t binarySize() const;
	uint64_t textOffset() const;
	uint64_t textVaddr() const;

	// The sections the core populated (.text/.data/.rodata/.bss), in file order,
	// for the memory pane's section-jump list. Empty when not loaded; unpopulated
	// sections (all-zero) are dropped so the pane offers no dead targets.
	std::vector<SectionInfo> sections() const;

	// "Recompile" seam. `edits` pairs a disassembly() row index with the new
	// instruction text typed into the pane. The assembler backend does not
	// exist yet, so this validates nothing and returns a human-readable stub
	// result; edits stay pending in the pane. When the assembler lands it will
	// be called from here, keeping the GUI unaware of the encoding details.
	std::string applyPatches(const std::vector<std::pair<size_t, std::string>>& edits);

	// Re-derives disasmRows_ from core state. Call after open() and after any
	// future debugger step mutates core state.
	void refresh();

private:
	void runDecode(); // Disassembler::decode() with the throw-on-stub-arch guard

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	std::string filePath_;
	std::string format_;
	std::string status_;
	std::string decodeNote_;
	bool decodedForReal_ = false;

	std::vector<DisasmRow> disasmRows_;
};

} // namespace gui

#endif
