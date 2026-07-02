#include <cstdint>
#include <string>

class Instruction {

public:
	Instruction() {};
	virtual std::string decodeLineString() = 0;
};