#include "elf-disassembler.h"
#include <stdexcept>

void ELF_Disassembler::setHeadersOffsets() {
	uint32_t e_shoff = this->contents[0x28];

}


ELF_Disassembler::ELF_Disassembler(vector<uint8_t> data): Disassembler(data) {
	if (this->contents[0x04] == 0x01) is32bit = true;
	switch (this->contents[0x10]) {
		case 0x3E:
			this->filetype = "x86-64"; break;

		case 0x28:
			this->filetype = "ARM"; break;
		
		default:
			throw runtime_error("Invalid architecture.");
	}

	this->setHeadersOffsets();
}