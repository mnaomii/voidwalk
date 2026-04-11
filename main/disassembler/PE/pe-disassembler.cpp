#include "pe-disassembler.h"

void PE_Disassembler::setHeadersOffsets() {

}

PE_Disassembler::PE_Disassembler(vector<uint8_t> data) : Disassembler(data) {
	this->setHeadersOffsets();
}

