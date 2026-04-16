#include "pe-disassembler.h"

void PE_Disassembler::setHeadersOffsets() {

}

PE_Disassembler::PE_Disassembler(AddressSpace& data) : Disassembler(data) {
	this->setHeadersOffsets();
}

