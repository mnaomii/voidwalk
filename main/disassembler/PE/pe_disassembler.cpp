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

size_t PE_Disassembler::decodeLine(uint64_t address) {

	// field4 is SIB byte
	uint32_t field1=0, field2=0, field3=0, field4=0, field5=0, field6 =0;
	size_t size = 0;

	switch (this->architecture) {
	case 0x14c: // x86

		field1 = contents.read_u8(address);
		size += 8;
		if (!IA_32::isPrefix(field1)) // invalid prefix, just read opcode
		{
			field2 = field1; 
			field1 = 0x00;
		}
		else { // must read opcode
			address += 8;
			size += 8;

			field2 = contents.read_u8(address);

			address += 8;
		}

		if (IA_32::hasRMbyte(field2)) {
			field3 = contents.read_u8(address);
			address += 8;

			uint8_t mod = ((field3 & 0b11000000) >> 6);
			uint8_t reg_op = ((field3 & 0b00111000) >> 3);
			uint8_t rm = (field3 & 0b00000111);

			if (rm == 0b100) {
				switch (mod) {
				case 0b01: { // mem + disp8
					field4 = contents.read_u8(address);
					address += 8;
					break;
				}

				case 0b10: { // mem + disp32
					field4 = contents.read_u32(address);
					address += 32;
					break;
				}
				default:
					field4 = 0x0;
				}

			}

			if (IA_32::hasImmediateByte(field2)) {
				uint32_t size;
				if(IA_32::opcodeStrOf(field2) == "IMUL")
					size = static_cast<uint32_t>(IA_32::opcodeTable()[field2].op3s);
				else size = static_cast<uint32_t>(IA_32::opcodeTable()[field2].op2s);


				// wip
			}

		}

		decodedInstructions.push_back(IA_32(field1, field2, field3, field4, field5, field6));
		return;

	case 0x8664:
		throw std::runtime_error("Not implemented yet.");

	case 0xAA64:
		throw std::runtime_error("Not implemented yet.");

	case 0x1c0:
		throw std::runtime_error("Not implemented yet.");

		//return ;

	default:
		throw std::runtime_error("Invalid architecture. Cannot parse.");

	}
}



std::string PE_Disassembler::decodeCS(FILE* outputStream) {
	
	
	uint64_t currentPtr = baseSections._text.getOffset();
	uint64_t endBoundary = baseSections._text.getSize() + currentPtr;

	while (currentPtr != endBoundary) {
		fprintf(outputStream, "%s", (decodedInstructions.end() - 1)->decodeLineString().c_str());
		// noBytes of instruction needed to continue;
	}

}

