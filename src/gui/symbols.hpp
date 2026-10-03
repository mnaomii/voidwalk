#pragma once
#include "gui/session.hpp"

#include <cstdint>
#include <stop_token>
#include <string>
#include <vector>

namespace gui {

// One entry in the symbol sidebar.
struct SymbolInfo {
	enum class Kind { Function, Import, String };

	Kind kind = Kind::Function;
	std::string name;   // "sub_401160", "MessageBoxA", "\"Access denied\""
	uint64_t addr = 0;  // virtual address (functions) / file offset (strings)
	std::string detail; // right-hand column: hex address, module, section
};

// Derives the sidebar's contents from what the core already knows — no new
// Session or Disassembler API. Three passes:
//
//   Functions  every direct call target in the decoded rows (Instruction::target()),
//              plus the binary's entry point, named by functionName().
//              These are real call graph facts, not guesses.
//   Strings    printable ASCII runs of >= kMinStringLen bytes in .rodata and
//              .data, quoted and truncated for display. `addr` is the file
//              offset, since a string is shown in the memory pane, not the code.
//   Imports    left empty: the loader does not expose an import table yet.
//              collectImports() is the single seam to fill in when it does —
//              the sidebar already renders the group and hides it while empty.
//
// The function pass is one linear walk over *every* decoded row, reading each
// row's flow and target. It takes a Snapshot, which pins the published row
// count. `stop` lets a superseded scan give up early — it is polled between rows,
// so cancellation is prompt even mid-binary.
std::vector<SymbolInfo> collectSymbols(const Snapshot& snapshot,
                                       const std::stop_token& stop = {});

// "entry" for the entry point, else "sub_<VADDR>". The one spelling of a function
// name, shared by the sidebar and the disassembly's NOTES column.
std::string functionName(uint64_t addr, uint64_t entry);

constexpr int kMinStringLen = 4;
constexpr int kMaxStringDisplay = 28;

} // namespace gui

