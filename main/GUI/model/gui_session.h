#ifndef GUI_SESSION_H
#define GUI_SESSION_H

#include "../../address-space/address_space.hpp"
#include "../../disassembler/disassembler.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gui {

// Cross-thread decode state, heap-allocated so the worker can hold a shared_ptr
// to it independently of the Session's lifetime/address (the Session is a plain
// value that must stay movable, and std::atomic is not movable). `running` is
// the acquire/release flag the UI polls; `note` is the decode's error message,
// written by the worker before it clears `running` and read by the UI only once
// `running` is false.
struct DecodeState {
	std::atomic<bool> running{false};
	std::string note;
};

// One row of the raw-bytes fallback shown when a decoder is unimplemented (a byte
// run rendered as "db 0x.."). Real decoded rows are NOT stored — they are read
// through to the core per row via Session::row*(); this struct backs only the
// small, capped fallback set (fallbackRows_), which has no Instruction objects.
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
	// Empty while the worker is still decoding: the note isn't safe to read until
	// isDecoding() is false (that acquire pairs with the worker's release).
	const std::string& decodeNote() const {
		static const std::string kEmpty;
		if (!decodeState_ || isDecoding()) return kEmpty;
		return decodeState_->note;
	}
	// True while decode() is still running on the worker thread. The window polls
	// this to keep refreshing the pane, and stops once it goes false.
	bool isDecoding() const {
		return decodeState_ && decodeState_->running.load(std::memory_order_acquire);
	}
	// True when the rows are real decoded instructions, false when they are the
	// raw-bytes fallback (unimplemented arch decoder).
	bool decodedForReal() const { return decodedForReal_; }

	// Row access, read straight through to the core — nothing is copied or kept
	// per row (the disassembly for a big binary is millions of rows; holding a
	// second stringified copy here was ~half the process's memory). Valid indices
	// are [0, rowCount()); reads are bounded by the worker's published count and
	// the reserve in decode() keeps the backing storage from moving, so this is
	// safe lock-free on the GUI thread. rowBytes/rowText build their string on the
	// fly, so call them only for the rows actually on screen.
	size_t rowCount() const;
	uint64_t rowVaddr(size_t i) const;
	std::string rowBytes(size_t i) const; // machine-code hex, trailing space trimmed
	std::string rowText(size_t i) const;  // formatted mnemonic + operands

	// Core's emulated register file (all zero until the debugger exists) and
	// simulated stack (empty until execution exists). Valid only when loaded().
	const Registers_x86_64& registers() const;
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

	// "Recompile" seam. `edits` pairs a row index (into rowCount()) with the new
	// instruction text typed into the pane. The assembler backend does not
	// exist yet, so this validates nothing and returns a human-readable stub
	// result; edits stay pending in the pane. When the assembler lands it will
	// be called from here, keeping the GUI unaware of the encoding details.
	std::string applyPatches(const std::vector<std::pair<size_t, std::string>>& edits);

	// Re-reads the worker's published instruction count so rowCount()/row*() expose
	// the newly-decoded rows. O(1) — it copies no rows; the panes read through on
	// demand. runDecode() resets the count and bumps decodeGeneration(), which is
	// how a re-open restarts the view from zero.
	void refresh();

	// Bumped once per decode start. The disassembly view watches it: a change means
	// the row content is entirely new, so it resets rather than diffing row counts.
	uint64_t decodeGeneration() const { return decodeGen_; }

private:
	void runDecode();     // launches Disassembler::decode() on decodeThread_
	void buildFallback(); // fills fallbackRows_ (small, capped) for a stub arch

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	std::string filePath_;
	std::string format_;
	std::string status_;
	std::shared_ptr<DecodeState> decodeState_;
	bool decodedForReal_ = false;
	uint64_t decodeGen_ = 0;

	// Real decode: the count the worker has published (rows are read through to the
	// core, never copied here). Stub arch: the small capped raw-bytes fallback,
	// which has no Instruction objects to read through.
	size_t shownRows_ = 0;
	std::vector<DisasmRow> fallbackRows_;

	// Declared LAST so it is destroyed (and thus joined) before decodeState_,
	// disassembler_ and space_ — the things the worker touches — are torn down.
	// The worker also holds shared_ptr copies of those, so it stays safe even if a
	// re-open swaps them out mid-decode.
	std::jthread decodeThread_;
};

} // namespace gui

#endif
