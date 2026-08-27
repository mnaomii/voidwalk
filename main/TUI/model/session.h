#ifndef TUI_SESSION_H
#define TUI_SESSION_H

#include "../../address-space/address_space.hpp"
#include "../../disassembler/disassembler.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace tui {

// Cross-thread decode state, heap-allocated behind a shared_ptr so the worker
// can reference it independently of the Session (which is moved into the UI, and
// std::atomic is not movable). `running` is the acquire/release flag the render
// loop polls; `note` is the decode's error message, written before `running` is
// cleared and read only once it is false. Mirrors gui::DecodeState.
struct DecodeState {
	std::atomic<bool> running{false};
	std::string note;
};

// View-model between the analysis core and the FTXUI panes.
// Panes never touch AddressSpace/Disassembler directly; they render the
// string rows this class derives. Anything the core cannot provide yet
// (instruction decode, debugger stepping) is filled with a visible
// placeholder here, so unimplemented features have exactly one home.
//
// Lifetime rule: disassembler_ holds an AddressSpace&, so space_ and
// disassembler_ must always be replaced together (see open()).
class Session {
public:
	Session() = default;
	Session(std::shared_ptr<AddressSpace> space,
	        std::shared_ptr<Disassembler> disassembler,
	        std::string filePath);

	// Loads a new binary at runtime (TUI "Open" action). On failure returns
	// false, keeps the previous binary and puts the reason in status().
	bool open(const std::string& path);

	bool loaded() const { return disassembler_ != nullptr; }
	const std::string& filePath() const { return filePath_; }
	const std::string& format() const { return format_; } // "ELF" / "PE" / ""
	std::string architecture() const;

	// status-bar message (also used by placeholder actions: "not implemented yet")
	const std::string& status() const { return status_; }
	void setStatus(std::string s) { status_ = std::move(s); }

	// True while decode() is still running on the worker thread. The UI loop polls
	// this to keep re-deriving the disassembly feed until the sweep finishes.
	bool isDecoding() const {
		return decodeState_ && decodeState_->running.load(std::memory_order_acquire);
	}

	// Pane feeds. Rebuilt by refresh(); stable between refreshes so FTXUI
	// renderers can hold references to the vectors.
	const std::vector<std::string>& disassemblyLines() const { return disasmLines_; }
	const std::vector<std::string>& registerRows() const { return regRows_; }
	const std::vector<std::string>& stackRows() const { return stackRows_; }

	// Raw bytes for the memory pane (clamped at EOF; empty if nothing loaded).
	std::vector<uint8_t> bytes(uint64_t offset, size_t count) const;
	size_t binarySize() const;
	uint64_t textOffset() const; // start of .text, 0 if unknown

	// Re-derives all pane feeds from core state. Call after open() and after
	// any future debugger step mutates registers/stack.
	void refresh();

private:
	// Launches Disassembler::decode() on decodeThread_ (one worker per loaded
	// binary). decodeLine() throws for architectures whose decoder isn't written
	// yet; the worker catches it into decodeState_->note rather than letting it
	// escape the thread, and the render loop reads that only once !isDecoding().
	void runDecode();

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	std::string filePath_;
	std::string format_;
	std::string status_;
	std::shared_ptr<DecodeState> decodeState_; // why the decode stopped early, + running flag

	std::vector<std::string> disasmLines_;
	std::vector<std::string> regRows_;
	std::vector<std::string> stackRows_;
	// How many *instruction* lines are already in disasmLines_. refresh() appends
	// only the newly-decoded ones instead of rebuilding the whole vector each poll
	// (any trailing "... decoding" status line sits past this count). Reset by
	// runDecode() when a new sweep starts.
	size_t builtInstrs_ = 0;

	// Declared LAST so it is joined before decodeState_/disassembler_/space_ are
	// destroyed. The worker holds shared_ptr copies of those, so it also stays safe
	// if a re-open swaps them out mid-decode.
	std::jthread decodeThread_;
};

} // namespace tui

#endif
