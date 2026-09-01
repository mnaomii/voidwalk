#ifndef GUI_SYMBOLS_H
#define GUI_SYMBOLS_H

#include "gui_session.h"

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
//   Functions  every address that appears as a `call` target in the decoded
//              rows (Session::row*()), plus .text's entry, named sub_<vaddr>.
//              These are real call graph facts, not guesses.
//   Strings    printable ASCII runs of >= kMinStringLen bytes in .rodata and
//              .data, quoted and truncated for display.
//   Imports    left empty: the loader does not expose an import table yet.
//              collectImports() is the single seam to fill in when it does —
//              the sidebar already renders the group and hides it while empty.
//
// NOT cheap: the function pass is one linear walk over *every* decoded row, each
// of which formats a string on the way through. On a large binary that is
// hundreds of thousands of allocations, which is why it takes a Snapshot (safe to
// read off the UI thread) rather than a live Session, and why SymbolsPane runs it
// on a worker. `stop` lets a superseded scan give up early — it is polled between
// rows, so cancellation is prompt even mid-binary.
std::vector<SymbolInfo> collectSymbols(const Snapshot& snapshot,
                                       const std::stop_token& stop = {});

constexpr int kMinStringLen = 4;
constexpr int kMaxStringDisplay = 28;

} // namespace gui

#endif
