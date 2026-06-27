#include "ui.h"


UI::UI(std::unique_ptr<Disassembler> d): disassembler(std::move(d)) {}

std::string UI::formatRow() {
	return "";
}
void UI::start() {

}