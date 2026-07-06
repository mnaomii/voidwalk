#include "pe_disassembler.h"
#include "parsers/pe_section_map.h"
#include <stdexcept>

void PE_Disassembler::setHeadersOffsets() {
	switch (this->architecture) {
	case 0x14c: // 32bit
		setHeaders32bit(this->baseSections, this->extraSections, this->contents, this->e_lfanew); break;

	case 0x8664: // 64bit
		setHeaders64bit(this->baseSections, this->extraSections, this->contents, this->e_lfanew); break;

	case 0xAA64: // AARCH64
		setHeaders64bit(this->baseSections, this->extraSections, this->contents, this->e_lfanew); break;

	case 0x1c0: // ARM32
		setHeaders32bit(this->baseSections, this->extraSections, this->contents, this->e_lfanew); break;
	}
}

std::string PE_Disassembler::getArchitecture() {
	switch (this->architecture) {
	case 0x14c:
		return "x86";

	case 0x8664:
		return "x86_64";

	case 0xAA64:
		return "AArch64";

	case 0x1c0:
		return "ARM32";
	default:
		return "Unknown";

	}
}

Instruction& PE_Disassembler::decodeLine(uint64_t address) {


	uint32_t field1, field2, field3, field4, field5;

	switch (this->architecture) {
	case 0x14c:

		break;

	case 0x8664:
		break;

	case 0xAA64:
		break;

	case 0x1c0:

		break;

	default:
		throw std::runtime_error("Invalid architecture. Cannot parse.");

	}
}

PE_Disassembler::PE_Disassembler(AddressSpace& data) : Disassembler(data) {
	
	this->e_lfanew = data.read_u32(0x3C);

	this->architecture = data.read_u32(e_lfanew + 4);

	this->setHeadersOffsets();
}

std::string PE_Disassembler::decodeCS(FILE* outputStream) {
	
	uint64_t currentPtr = baseSections._text.getOffset();
	uint64_t endBoundary = baseSections._text.getSize() + currentPtr;

	while()

}

