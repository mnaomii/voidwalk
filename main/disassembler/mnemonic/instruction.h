#include <cstdint>
#include <string>
#pragma once




class Instruction {

public:

	struct OpcodeInfo {
		std::string_view text;
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

	Instruction() {};
	void decode() {}
	virtual std::string decodeLineString() = 0;
};