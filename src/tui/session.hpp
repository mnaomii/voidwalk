#pragma once
#include "analysis_session.hpp"

#include <array>
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

// The FTXUI view-model: voidwalk::Session plus the string rows the panes render.
// Panes never touch AddressSpace or Disassembler. Anything the core cannot provide
// yet is filled with a visible placeholder here.
class Session : public voidwalk::Session {
public:
	Session() = default;

	// Adopts an already-loaded binary (the startup path, where main() has opened the
	// file to report errors before the UI exists) and starts decoding it.
	Session(std::shared_ptr<AddressSpace> space,
	        std::shared_ptr<Disassembler> disassembler,
	        std::string filePath);

	// Disassembly rows: one per decoded instruction, then any status / placeholder
	// lines. A row is built only when asked for - instructions are read through to
	// the core, never copied - so call disassemblyRow() for the rows on screen only.
	size_t disassemblyRowCount() const { return shownInstrs_ + extraLines_.size(); }
	std::string disassemblyRow(size_t i) const;

	// Emulated register rows, "name value" per line. Rebuilt by refresh().
	const std::vector<std::string>& registerRows() const { return regRows_; }

	// Vector register rows, "name value" per line, one list per group: MMX, SSE,
	// AVX, AVX-512. Rebuilt by refresh(); the lists themselves stay put.
	const std::array<std::vector<std::string>, 4>& vectorRegisterRows() const { return vecRows_; }

	// Simulated stack rows, top of stack first. Rebuilt by refresh().
	const std::vector<std::string>& stackRows() const { return stackRows_; }

	// Re-derives all pane feeds from core state. Call after open() and after any
	// future debugger step mutates registers/stack. O(1) for the disassembly: it
	// only re-reads the worker's published instruction count.
	void refresh();

private:
	// Drops the previous sweep's rows.
	void onDecodeStarted() override;

	size_t shownInstrs_ = 0;               // instructions exposed (<= the worker's published count)
	std::vector<std::string> extraLines_;  // status / placeholder lines after them
	std::vector<std::string> regRows_;
	std::array<std::vector<std::string>, 4> vecRows_;
	std::vector<std::string> stackRows_;
};

} // namespace tui
