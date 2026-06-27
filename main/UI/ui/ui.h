#ifndef UI_H
#define UI_H
#include "../../disassembler/disassembler.h"
#include <string>
#include <memory>

class UI {
private:
	std::unique_ptr<Disassembler> disassembler;

	std::string formatRow();
public:
	UI(std::unique_ptr<Disassembler> d);
	void start();
};
#endif