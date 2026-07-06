#include <cstdint>
#include <string>


struct OpcodeInfo {
	std::string_view text;
	bool hasRMByte;
	bool hasImmediateByte;
	uint8_t op1am, op2am, op3am;
	uint8_t op1s, op2s, op3s;
};

struct Operand {
	uint64_t value;
	uint8_t addressingMode, size;
};

class Instruction {

public:
	Instruction() {};
	virtual std::string decodeLineString() = 0;
};