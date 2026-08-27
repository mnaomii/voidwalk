#ifndef GUI_SYMBOLS_H
#define GUI_SYMBOLS_H

#include "gui_session.h"

#include <cstdint>
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
// Cheap enough to call from refresh() (one linear pass over the rows plus one
// over the two data sections); the sidebar caches the result either way.
std::vector<SymbolInfo> collectSymbols(const Session& session);

constexpr int kMinStringLen = 4;
constexpr int kMaxStringDisplay = 28;

} // namespace gui

#endif
