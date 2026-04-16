#include "pe-disassembler.h"

void PE_Disassembler::setHeadersOffsets() {

}

std::string PE_Disassembler::getArchitecture() {
	return "WIP";
}

PE_Disassembler::PE_Disassembler(AddressSpace& data) : Disassembler(data) {
	this->setHeadersOffsets();
}

