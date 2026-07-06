#include <cstdint>
#include <string>

enum class STATE : int {
	IS_IMMEDIATE = 2,
	IS_MEMORY = 1,
	IS_REGISTER = 0,
	IS_NULL = -1
};

struct OpcodeInfo {
	std::string_view text;
	bool hasRMByte;
	bool hasImmediateByte;

};

struct Operand {
	uint32_t value;
	STATE state;

};

class Instruction {

public:
	Instruction() {};
	virtual std::string decodeLineString() = 0;
};