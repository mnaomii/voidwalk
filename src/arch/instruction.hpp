#pragma once
#include "chunk_store.hpp"

#include <cstdint>
#include <memory>
#include <string>


namespace voidwalk {

// One decoded instruction, as the rest of the program sees it.
//
// Two rendered strings plus the control flow: what kind of branch this is and,
// when it is direct, where it goes. That is all the frontends need back (call
// targets for the symbol sidebar, jump direction for the NOTES column), so they
// never re-parse the text for it. No operand list or length survives decoding.
//
// Subclassed once per architecture. Instances are heap-allocated and owned by an
// InstructionStore; the decoder hands ownership over via push_back and never
// touches them again.
class Instruction {
public:
	// What the instruction does to control flow.
	enum class Flow : uint8_t { None, Call, Jump, CondJump, Return };

protected:
	Flow flow_ = Flow::None;
	uint64_t target_ = 0;   // absolute branch target; 0 when indirect or not a branch

	// Hex of the bytes this instruction occupies, in stream order, space-separated
	// and WITH a trailing space - e.g. "48 8b 05 12 34 00 00 ". Callers that render
	// it trim the tail (gui::Session::rowBytes).
	std::string machineCode;

	// The rendered line, as "<MNEMONIC> \t<operands>" - note the literal space
	// before the tab, and that an operand-less instruction still carries both.
	// "(bad)" for anything the decoder rejected, with no tab at all.
	// gui::formatDisasmText depends on exactly this shape.
	std::string instructionText;

public:
	// The rendered line. Non-const reference because the GUI formats from it in
	// place; callers must not mutate it.
	virtual std::string& decodeLineString() = 0;

	// The instruction's bytes as hex. Same aliasing caveat as above.
	virtual std::string& getMachineCode() = 0;

	Flow flow() const { return flow_; }
	uint64_t target() const { return target_; }

	virtual ~Instruction() = default;
};

// Where a sweep's decoded instructions accumulate. Chunked rather than contiguous
// because the decode worker appends to it while the UI reads it; see chunk_store.hpp.
using InstructionStore = ChunkStore<std::unique_ptr<Instruction>>;

} // namespace voidwalk
