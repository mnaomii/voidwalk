#include "elf_disassembler.h"
#include "parsers/elf_section_map.h"
#include <stdexcept>
#include "../mnemonic/instruction.h"
#include "../mnemonic/IA-32/IA-32-instr.h"
#include "../mnemonic/AArch64/AArch64-instr.h"
#include "../mnemonic/AMD64/AMD64-instr.h"
#include "../mnemonic/ARM32/ARM32-instr.h"


ELF_Disassembler::ELF_Disassembler(AddressSpace& data) : Disassembler(data) { // constructor
	this->architecture = this->contents.read_u16(0x12);


	this->setHeadersOffsets();
}

std::string ELF_Disassembler::getArchitecture() {
	switch (this->architecture) {
	case 0x03:
		return "x86";
	case 0x28:
		return "ARM32";
	case 0x3E:
		return "x86_64";
	case 0xB7:
		return "AArch64";
	default:
		return "Unknown";
	}
}

void ELF_Disassembler::setHeadersOffsets() {
	switch(this->architecture){
		case 0x03 : setHeaders32bit(this->baseSections, this->extraSections, this->contents); break;
		
		case 0x28: setHeaders32bit(this->baseSections, this->extraSections, this->contents); break;

		case 0x3E: setHeaders64bit(this->baseSections, this->extraSections, this->contents); break;

		case 0xB7: setHeaders64bit(this->baseSections, this->extraSections, this->contents); break;
		default:
			throw std::runtime_error("Unsupported 64-bit ELF architecture.");
	}
}
uint64_t ELF_Disassembler::decodeLine(uint64_t address) {
	return 0;
}

void ELF_Disassembler::decodeCS(FILE* outputStream) {

}

