#pragma once
#include "chunk_store.hpp"

#include <cstdint>
#include <memory>
#include <string>


namespace voidwalk {

// One decoded instruction, as the rest of the program sees it.
//
// Deliberately just two rendered strings. Nothing structured survives decoding -
// no operand list, no length, no branch target - because every consumer so far
// wants text. The cost is that anything wanting semantics back (call targets for
// the symbol sidebar, jump direction for the NOTES column) re-parses the string;
// see gui::collectSymbols. Widening this type is the fix if that ever stops being
// good enough.
//
// Subclassed once per architecture. Instances are heap-allocated and owned by an
// InstructionStore; the decoder hands ownership over via push_back and never
// touches them again.
class Instruction {



protected:
	// Hex of the bytes this instruction occupies, in stream order, space-separated
	// and WITH a trailing space - e.g. "48 8b 05 12 34 00 00 ". Callers that render
	// it trim the tail (gui::Session::rowBytes).
	std::string machineCode;

	// The rendered line, as "<MNEMONIC> \t<operands>" - note the literal space
	// before the tab, and that an operand-less instruction still carries both.
	// "(bad)" for anything the decoder rejected, with no tab at all.
	// gui::formatDisasmText depends on exactly this shape.
	std::string instructionStr;

	// Reserved for the patch/recompile path: set false at construction and never
	// read. Nothing marks an instruction dirty yet.
	bool hasChanged;



public:

	// One operand slot of an opcode-table row. Purely static table data - it
	// describes what the operand IS, never what a particular instruction decoded to.
	//
	// The two enum fields are stored as uint8_t rather than their enum types so this
	// header stays architecture-neutral; x86 casts them back to
	// x86_64_Mnemonic::ADDRESSING and ::SIZE.
	struct TableOperand {
		uint8_t          addressingMode; // ADDRESSING: E, G, I, J, M, Z, None, ...
		uint8_t          size;           // SIZE: b, v, z, bs, ... (None = inherit)
		bool forcedSize;                 // reserved; no table row sets it true today

		// Name of an implicit operand, per operand width - "RAX" / "EAX" / "AX".
		// All three are empty when the operand is not implicit (it comes from ModRM,
		// an immediate, etc.). value64 equals value32 except where a table row
		// overrides it (V64_0 in x86_64_tables.cpp, for XCHG's accumulator).
		std::string_view value64;
		std::string_view value32;
		std::string_view value16;
	};

	// One row of an opcode table: everything known about an opcode before any
	// instruction bytes are looked at.
	struct OpcodeInfo {

		// Long-mode default operand size.
		//   None  32-bit default, REX.W promotes to 64
		//   d64   64-bit default, but a 0x66 prefix drops it to 16 (PUSH/POP)
		//   f64   forced 64-bit, 0x66 ignored entirely (near branches)
		// Ignored by a 32-bit decode.
		enum class Default64 { None, d64, f64 };

		std::string_view text;    // default mnemonic
		std::string_view text16;  // mnemonic under 0x66; empty means use `text`
		bool hasRMByte;           // a ModRM byte follows the opcode

		// Up to three operands, in printed order. The first slot whose
		// addressingMode is ADDRESSING::None ends the list.
		TableOperand op[3];

		// > 0 when this opcode is a group escape whose real meaning comes from
		// ModRM.reg (or the whole ModRM byte, for x87); -1 when it is not a group.
		// Callers must resolve through resolvedInfo() rather than reading this row.
		int groupNo;

		// The opcode does not exist in 64-bit mode. The tables are shared with the
		// 32-bit decode, which ignores this.
		bool isInvalid;

		std::string_view text64;  // mnemonic under REX.W; empty means use `text`
		Default64 def64;
	};

	// Unused. Intended as a decoded (not table) operand for when Instruction grows
	// past two strings; nothing constructs one today.
	struct Operand {
		std::string text;
		uint64_t value;
		uint8_t addressingMode, size;
	};

	Instruction() : hasChanged(false), machineCode(""), instructionStr("") {};
	//void decode() {}

	// The rendered line. Non-const reference because the GUI formats from it in
	// place; callers must not mutate it.
	virtual std::string& decodeLineString() = 0;

	// The instruction's bytes as hex. Same aliasing caveat as above.
	virtual std::string& getMachineCode() = 0;

	virtual ~Instruction() = default;
};

// Where a sweep's decoded instructions accumulate. Chunked rather than contiguous
// because the decode worker appends to it while the UI reads it; see chunk_store.hpp.
using InstructionStore = ChunkStore<std::unique_ptr<Instruction>>;

} // namespace voidwalk
