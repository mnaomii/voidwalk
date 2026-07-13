#ifndef TUI_SESSION_H
#define TUI_SESSION_H

#include "../../address-space/address_space.h"
#include "../../disassembler/disassembler.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tui {

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
	// Runs Disassembler::decode() once per loaded binary. decodeLine() throws for
	// architectures whose decoder isn't written yet, so the reason is captured in
	// decodeNote_ rather than escaping into the render loop.
	void runDecode();

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	std::string filePath_;
	std::string format_;
	std::string status_;
	std::string decodeNote_; // why the decode pass stopped early, if it did

	std::vector<std::string> disasmLines_;
	std::vector<std::string> regRows_;
	std::vector<std::string> stackRows_;
};

} // namespace tui

#endif
