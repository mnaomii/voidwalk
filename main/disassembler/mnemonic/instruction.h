#include <cstdint>
#include <string>
#pragma once




class Instruction {

protected:
	std::string machineCode;
	bool hasChanged;

public:

	struct Prefix {
		uint64_t byte[4];// up to 4 bytes supported for prefixes
	};

	struct OpcodeInfo {
		std::string_view text;
		// Mnemonic when a 0x66 prefix drops the operand size to 16 bits. Empty for the
		// opcodes whose name does not move with the operand size, which is almost all of
		// them - the decoder falls back to text. text always holds the name at the mode's
		// default operand size (32-bit for IA-32), which is what an unprefixed byte means.
		std::string_view text16;
		bool hasRMByte;
		bool hasImmediateByte;
		uint8_t op1am, op2am, op3am;
		uint8_t op1s, op2s, op3s;
		// The mnemonic text already spells its operands out ("ADD AL", "XCHG eAX, eCX"),
		// so the decoder must not render them a second time from the addressing modes.
		bool textNamesOperands;
		int groupNo;
	};

	struct Operand {
		std::string text;
		uint64_t value;
		uint8_t addressingMode, size;
	};

	Instruction() : hasChanged(false), machineCode("") {};
	//void decode() {}
	virtual std::string& decodeLineString() = 0;
	virtual std::string& getMachineCode() = 0;
};