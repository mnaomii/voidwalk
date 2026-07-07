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

uint64_t PE_Disassembler::decodeLine(uint64_t address) {

	// field4 is SIB byte
	uint32_t field1=0, field2=0, field3=0, field4=0, field5=0, field6 =0;

	switch (this->architecture) {
	case 0x14c: // x86

		field1 = contents.read_u8(address);
		if (!IA_32::isPrefix(field1)) // invalid prefix, just read opcode
		{
			field2 = field1;
			field1 = 0x00;
		}
		else { // must read opcode
			++address;

			field2 = contents.read_u8(address);
		}
		++address;

		field3 = 0x0;
		uint8_t mod = 0x0, reg_op = 0x0, rm = 0x0;
		if (IA_32::hasRMbyte(field2)) {
			field3 = contents.read_u8(address++); // getting ModR/M byte

			mod = ((field3 & 0b11000000) >> 6);
			reg_op = ((field3 & 0b00111000) >> 3);
			rm = (field3 & 0b00000111);

			if (mod != 0b11 && rm == 0b100)
				field4 = contents.read_u8(address++); // read sib;

			switch (mod) {
			case 0b01:
				field5 = contents.read_u8(address++);
				break;
			case 0b10:
				field5 = contents.read_u32(address);
				address += 4;
				break;
			case 0b00:
				if (rm == 0b101 || (rm == 100 && (field4 & 0b00000111) == 0b101)) {
					field5 = contents.read_u32(address);
					address += 4;
				}
				else field5 = 0x0;
				break;
			default:
				field5 = 0x0;
				break;
			}
		}


		if (IA_32::hasImmediateByte(field2)) { // set immediate field

#define cast(name) static_cast<uint8_t>(name)

				auto immSize = (IA_32::opcodeStrOf(field2) == "IMUL") ? IA_32::op3Size(field2) : IA_32::op2Size(field2);
				if(field1 == 0x66) // 16bit
					switch (immSize) {
					case cast(SIZE::b):
						field6 = contents.read_u8(address++);
						break;
					case cast(SIZE::w):
					case cast(SIZE::v):
					case cast(SIZE::z):
						field6 = contents.read_u16(address);
						address += 2;
						break;

					default:
						fprintf(stderr, "Not implemented yet");
						break;
					}
				else { // 32bit
					switch (immSize) {
					case cast(SIZE::b):
						field6 = contents.read_u8(address++);
						break;
					case cast(SIZE::w):
						field6 = contents.read_u16(address);
						address += 2;
						break;

					case cast(SIZE::v):
					case cast(SIZE::z):
						field6 = contents.read_u32(address);
						address += 4;
						break;

					default:
						fprintf(stderr, "Not implemented yet");
						break;
					}
				}
			}
			else field6 = 0x0;

#undef cast
		

		decodedInstructions.push_back(std::make_unique<IA_32>(field1,field2,field3,field4,field5,field6));
		return address; // address where next instruction begins

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



void PE_Disassembler::decodeCS(FILE* outputStream) {
	
	
	uint64_t currentPtr = baseSections._text.getOffset();
	uint64_t endBoundary = baseSections._text.getSize() + currentPtr;
	size_t index;

	while (currentPtr != endBoundary) {

		currentPtr += decodeLine(currentPtr);
		fprintf(outputStream, "%s", decodedInstructions[index++]->decodeLineString().c_str());
		
	}

}

