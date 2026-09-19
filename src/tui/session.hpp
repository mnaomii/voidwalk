#pragma once
#include "analysis_session.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tui {

using voidwalk::AddressSpace;
using voidwalk::Disassembler;
using voidwalk::Header;
using voidwalk::Registers_x86_64;
using voidwalk::Sections;

// The FTXUI view-model: voidwalk::Session plus the preformatted string rows the
// panes render. Panes never touch AddressSpace or Disassembler. Anything the core
// cannot provide yet is filled with a visible placeholder here.
class Session : public voidwalk::Session {
public:
	Session() = default;

	// Adopts an already-loaded binary (the startup path, where main() has opened the
	// file to report errors before the UI exists) and starts decoding it.
	Session(std::shared_ptr<AddressSpace> space,
	        std::shared_ptr<Disassembler> disassembler,
	        std::string filePath);

	// Scrollable disassembly rows, one per decoded instruction plus a trailing
	// status line while a decode is in flight. Rebuilt by refresh().
	const std::vector<std::string>& disassemblyLines() const { return disasmLines_; }

	// Emulated register rows, "name value" per line. Rebuilt by refresh().
	const std::vector<std::string>& registerRows() const { return regRows_; }

	// Simulated stack rows, top of stack first. Rebuilt by refresh().
	const std::vector<std::string>& stackRows() const { return stackRows_; }

	// Re-derives all pane feeds from core state. Call after open() and after any
	// future debugger step mutates registers/stack. Appends only newly-decoded
	// instruction rows rather than rebuilding the whole vector each poll.
	void refresh();

private:
	// Drops the previous sweep's rows so refresh() appends from zero.
	void onDecodeStarted() override;

	std::vector<std::string> disasmLines_;
	std::vector<std::string> regRows_;
	std::vector<std::string> stackRows_;

	// How many *instruction* lines are already in disasmLines_; any trailing
	// "... decoding" status line sits past this count.
	size_t builtInstrs_ = 0;
};

} // namespace tui
