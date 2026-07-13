#include "pe_disassembler.h"
#include "../../address-space/address_space.h"
#include "parsers/pe_section_map.h"
#include "../mnemonic/IA-32/IA-32-instr.h"
#include "../mnemonic/AArch64/AArch64-instr.h"
#include "../mnemonic/AMD64/AMD64-instr.h"
#include "../mnemonic/ARM32/ARM32-instr.h"
#include <stdexcept>

// the enums moved into IA_32Mnemonic (re-exported by IA_32); keep the old unqualified names below working
using ADDRESSING = IA_32::ADDRESSING;
using SIZE = IA_32::SIZE;


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

uint64_t PE_Disassembler::decodeLine(uint64_t address, uint64_t vaddr) {

	// field4 is SIB byte
	uint32_t field1=0, field2=0, field3=0, field4=0, field5=0, field6 =0;
	uint32_t initAddress = address;
	

	switch (this->architecture) {
	case 0x14c: {// x86 

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

			// A displacement is an offset from a register, so it is signed: disp8 0xFC is -4,
			// not +252. Widen it here - once it sits in an unsigned field the sign is gone
			// for good, and no cast downstream can bring it back.
			switch (mod) {
			case 0b01:
				field5 = static_cast<uint32_t>(static_cast<int8_t>(contents.read_u8(address++)));
				break;
			case 0b10:
				field5 = contents.read_u32(address);              // disp32: already the full width
				address += 4;
				break;
			case 0b00:
				if (rm == 0b101 || (rm == 0b100 && (field4 & 0b00000111) == 0b101)) {
					field5 = contents.read_u32(address);          // absolute address, no base register
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

			uint8_t immSize;
			uint8_t immAddr;
			field6 = 0x0;

			if (IA_32::opcodeTable()[field2].op1am == static_cast<uint8_t>(ADDRESSING::I) || IA_32::opcodeTable()[field2].op1am == static_cast<uint8_t>(ADDRESSING::J)) {
				immSize = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op1s);
				immAddr = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op1am);
			}
			else if (IA_32::opcodeTable()[field2].op2am == static_cast<uint8_t>(ADDRESSING::I) || IA_32::opcodeTable()[field2].op2am == static_cast<uint8_t>(ADDRESSING::J)) {
				immSize = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op2s);
				immAddr = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op2am);
			}
			else {
				immSize = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op3s);
				immAddr = static_cast<uint8_t>(IA_32::opcodeTable()[field2].op3am);

			}

			const bool sixteenBit = (field1 == 0x66);
			const bool isRelative = (immAddr == static_cast<uint8_t>(ADDRESSING::J));

			uint32_t width = 0;   // bytes this immediate occupies in the stream
			switch (immSize) {
			case cast(SIZE::b): width = 1; break;
			case cast(SIZE::w): width = 2; break;
			case cast(SIZE::v):
			case cast(SIZE::z): width = sixteenBit ? 2 : 4; break;
			default:
				fprintf(stderr, "Immediate size not implemented yet (opcode %#x)\n", field2);
				break;
			}

			// A relative offset is signed - rel8 0xF0 is -16, a jump backwards. Every other
			// immediate (a value, a port, an absolute address) is just bits, so read it plain.
			int64_t value = 0;
			switch (width) {
			case 1:
				value = isRelative ? static_cast<int8_t>(contents.read_u8(address))
				                   : static_cast<int64_t>(contents.read_u8(address));
				break;
			case 2:
				value = isRelative ? static_cast<int16_t>(contents.read_u16(address))
				                   : static_cast<int64_t>(contents.read_u16(address));
				break;
			case 4:
				value = isRelative ? static_cast<int32_t>(contents.read_u32(address))
				                   : static_cast<int64_t>(contents.read_u32(address));
				break;
			}
			address += width;

			// rel is counted from the END of the instruction, so resolve the target only once
			// every byte of it has been consumed - address - initAddress is now its full length.
			field6 = isRelative
				? static_cast<uint32_t>(vaddr + (address - initAddress) + value)
				: static_cast<uint32_t>(value);
		}
		else field6 = 0x0;

#undef cast

		auto instruction = std::make_unique<IA_32>();
		instruction->decode(field1, field2, field3, field4, field5, field6);
		decodedInstructions.push_back(std::move(instruction));

		return address; // address where next instruction begins
	}
	case 0x8664: {
		throw std::runtime_error("Not implemented yet.");
	}
	case 0xAA64: {
		throw std::runtime_error("Not implemented yet.");
	}
	case 0x1c0: {
		throw std::runtime_error("Not implemented yet.");

		//return ;
	}
	default: {
		throw std::runtime_error("Invalid architecture. Cannot parse.");
	}
	}

}



void PE_Disassembler::decodeCS(FILE* outputStream) {

	decode();

	for (const auto& instruction : decodedInstructions)
		fprintf(outputStream, "  | \t %s \n", instruction->decodeLineString().c_str());

}

