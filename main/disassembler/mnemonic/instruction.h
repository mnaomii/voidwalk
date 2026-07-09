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
		bool impliesOperands;
	};

	struct Operand {
		std::string text;
		uint64_t value;
		uint8_t addressingMode, size;
	};

	Instruction() {};
	virtual std::string decodeLineString() = 0;
};