#include "pe_disassembler.h"
#include "../../address-space/address_space.h"
#include "parsers/pe_section_map.h"
#include "../mnemonic/IA-32/IA-32-instr.h"
#include "../mnemonic/AArch64/AArch64-instr.h"
#include "../mnemonic/AMD64/AMD64-instr.h"
#include "../mnemonic/ARM32/ARM32-instr.h"
#include <stdexcept>




PE_Disassembler::PE_Disassembler(AddressSpace& data) : Disassembler(data) {

	this->e_lfanew = contents.read_u32(0x3C);

	this->architecture = contents.read_u32(e_lfanew + 4);

	this->setHeadersOffsets();
}

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


void PE_Disassembler::decodeCS(FILE* outputStream) {

	decode();

	for (const auto& instruction : decodedInstructions)
		fprintf(outputStream, "  | \t %s \n", instruction->decodeLineString().c_str());

}

