#pragma once
#include "analysis_session.hpp"
#include "address_space.hpp"
#include "disassembler/disassembler.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gui {

// The analysis core lives in namespace voidwalk (see src/address_space.hpp).
// Import only the names this layer actually names, rather than pulling the whole
// namespace in - the point of scoping the core was to stop it leaking.
using voidwalk::AddressSpace;
using voidwalk::Disassembler;
using voidwalk::Header;
using voidwalk::Instruction;
using voidwalk::Registers_x86_64;
using voidwalk::Sections;


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

// A stable, read-only view of the rows one decode produced — the only thing in
// this layer that is safe to touch off the UI thread.
//
// It holds shared_ptr copies of the disassembler and its address space, so an
// Open on the UI thread cannot pull them out from under a worker, and it pins
// the row count at the moment it was taken, so it can never read past what the
// decode had published. Rows are still read through to the core per call (no
// second stringified copy of the disassembly), so it is cheap to take and cheap
// to hold. Session::snapshot() is the only way to make a valid one.
//
// Only real decoded rows are exposed: for a stub-architecture fallback the
// snapshot is empty, since there are no instructions to walk.
//
// Reading it while the decode worker is still appending rests on exactly the
// same invariant the UI thread already relies on: decode() reserves worst case
// up front, so the backing vectors never reallocate, and rows_ never exceeds the
// worker's published count. If that reserve is ever traded for growth (see G1 in
// AUDIT.md), both readers need a different handoff, not just this one.
class Snapshot {
public:
	Snapshot() = default;

	bool valid() const { return disassembler_ != nullptr; }

	size_t rowCount() const { return rows_; }
	uint64_t rowVaddr(size_t i) const;
	std::string rowText(size_t i) const; // formatted mnemonic + operands

	uint64_t textVaddr() const;
	std::vector<SectionInfo> sections() const;
	std::vector<uint8_t> bytes(uint64_t offset, size_t count) const;

private:
	friend class Session;

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	size_t rows_ = 0;
};

// View-model between the analysis core and the Qt panes — same seam as
// tui::Session, but with structured rows instead of preformatted strings so
// the item models can put address/bytes/mnemonic in separate columns.
// Panes and models never touch AddressSpace/Disassembler directly.
//
// Lifetime rule (inherited from the core): disassembler_ holds an
// AddressSpace&, so space_ and disassembler_ are always replaced together.
class Session : public voidwalk::Session {
public:
	Session() = default;




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

	// A snapshot of the rows published so far, safe to hand to a worker thread —
	// see Snapshot. Empty (but valid) while the arch decoder is a stub.
	Snapshot snapshot() const;

private:
	// Drops the previous sweep's view state and bumps the generation counter.
	void onDecodeStarted() override;

	// Fills fallbackRows_ (small, capped) for an architecture whose decoder is a stub.
	void buildFallback();

	bool decodedForReal_ = false;
	uint64_t decodeGen_ = 0;

	// Real decode: the count the worker has published (rows are read through to the
	// core, never copied here). Stub arch: the small capped raw-bytes fallback,
	// which has no Instruction objects to read through.
	size_t shownRows_ = 0;
	std::vector<DisasmRow> fallbackRows_;

};

} // namespace gui

