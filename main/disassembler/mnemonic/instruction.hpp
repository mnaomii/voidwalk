#include <cstdint>
#include <string>
#pragma once


class Instruction {



protected:
	std::string machineCode;
	std::string instructionStr;
	bool hasChanged;



public:

	struct TableOperand {
		uint8_t          addressingMode;
		uint8_t          size;
		bool forcedSize;
		std::string_view value64;
		std::string_view value32;
		std::string_view value16;
	};

	struct OpcodeInfo {

		enum class Default64 { None, d64, f64 };

		std::string_view text;
		std::string_view text16;
		bool hasRMByte;

		TableOperand op[3];
		int groupNo;
		bool isInvalid;

		std::string_view text64;
		Default64 def64;
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