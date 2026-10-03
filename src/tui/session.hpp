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
using voidwalk::Instruction;
using voidwalk::Registers_x86_64;
using voidwalk::Sections;

// Same shape as gui::SectionInfo, so the memory pane offers the same jump list.
struct SectionInfo {
	std::string name;
	uint64_t offset = 0;
	uint64_t vaddr = 0;
	uint64_t size = 0;
};

// Same shape and spelling as gui::SymbolInfo (Imports stays empty until the
// loader parses an import table - the sidebar hides an empty group).
struct SymbolInfo {
	enum class Kind { Function, Import, String };
	Kind kind = Kind::Function;
	std::string name;
	uint64_t addr = 0;     // vaddr (functions) / file offset (strings)
	std::string detail;    // right-hand column
};

// The FTXUI view-model. It now exposes structured rows - address, bytes, text,
// flow, target - exactly like gui::Session, instead of one preformatted string per
// row, so the TUI can draw the same five columns the Qt table does. Rows are still
// read through to the core per call; nothing is copied.
class Session : public voidwalk::Session {
public:
	Session() = default;
	Session(std::shared_ptr<AddressSpace> space,
	        std::shared_ptr<Disassembler> disassembler,
	        std::string filePath);

	size_t rowCount() const;
	uint64_t rowVaddr(size_t i) const;
	std::string rowBytes(size_t i) const;   // "f3 0f 1e fa", trailing space trimmed
	std::string rowText(size_t i) const;    // "mov    rax, rbx" - mnemonic padded to 7
	Instruction::Flow rowFlow(size_t i) const;
	uint64_t rowTarget(size_t i) const;
	// The NOTES column: callee name for a direct call, "backward" for a backward jump.
	std::string rowNote(size_t i) const;

	// Last row whose vaddr <= `vaddr`, searched per .text run (gui navigateTo()).
	size_t rowIndexFor(uint64_t vaddr) const;

	// One line shown above the rows when they are not (yet) real instructions:
	// the stub-arch notice, a decode-stopped note. Empty when there is nothing to say.
	const std::string& banner() const { return banner_; }

	const Registers_x86_64& registers() const;
	std::vector<SectionInfo> sections() const;

	// Functions + strings, collected once per decode after the worker finishes.
	const std::vector<SymbolInfo>& symbols() const { return symbols_; }
	bool scanningSymbols() const { return loaded() && symbolsGen_ != decodeGeneration(); }

	void refresh();

	// "entry" for the entry point, else "sub_<VADDR>" - gui::functionName's spelling.
	static std::string functionName(uint64_t addr, uint64_t entry);

private:
	struct FallbackRow {
		uint64_t vaddr = 0;
		std::string bytes;
		std::string text;
	};

	void onDecodeStarted() override;
	void buildFallback();
	void collectSymbols();

	bool real_ = false;
	size_t shown_ = 0;
	std::vector<FallbackRow> fallback_;
	std::string banner_;
	std::vector<SymbolInfo> symbols_;
	uint64_t symbolsGen_ = ~0ull;
};

} // namespace tui
