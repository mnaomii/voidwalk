#include <cstdint>
#include <string>
#pragma once




class Instruction {

protected:
	std::string machineCode;
	std::string instructionStr;
	bool hasChanged;

public:



	// One operand as the table describes it. addressingMode + size say how to decode it
	// from the bytes; value/value16 carry the concrete name for operands the opcode fixes
	// in silicon (AL, eAX, DX, the constant 1, a segment reg, a string pointer). value is
	// "" for operands that come from the bytes (E/G/M/S/I/J/O/A) - the decoder builds those.
	// value16 is the name a 0x66 prefix selects; duplicate value when the name does not move.
	struct TableOperand {
		uint8_t          addressingMode;
		uint8_t          size;
		std::string_view value;
		std::string_view value16;
	};

	struct OpcodeInfo {
		std::string_view text;
		// Mnemonic when a 0x66 prefix drops the operand size to 16 bits. Empty for the
		// opcodes whose name does not move with the operand size, which is almost all of
		// them - the decoder falls back to text. text always holds the name at the mode's
		// default operand size (32-bit for IA-32), which is what an unprefixed byte means.
		// This is the mnemonic itself flipping (CWDE/CBW, STOSD/STOSW), not an operand.
		std::string_view text16;
		bool hasRMByte;
		TableOperand op[3];
		int groupNo;
	};

	struct Operand {
		std::string text;
		uint64_t value;
		uint8_t addressingMode, size;
	};

	Instruction() : hasChanged(false), machineCode(""), instructionStr("") {};
	//void decode() {}
	virtual std::string& decodeLineString() = 0;
	virtual std::string& getMachineCode() = 0;
};