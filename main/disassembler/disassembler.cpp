#include "disassembler.h"
#include "../address-space/address_space.h"
#include "miscellaneous/sections/base/header.h"
#include "mnemonic/IA-32/IA-32-instr.h"
#include "mnemonic/IA-32/IA-32-mnemonic.h"


void Disassembler::decode() {
	decodedInstructions.clear();
	instructionAddresses.clear();

	const uint64_t start = baseSections._text.getOffset();
	const uint64_t end = start + baseSections._text.getSize();

	uint64_t ptr = start;
	uint64_t vaddr = baseSections._text.getVaddr();

	while (ptr < end) {
		const uint64_t lineVaddr = vaddr;
		uint64_t next = decodeLine(ptr, vaddr);

		while (instructionAddresses.size() < decodedInstructions.size())
			instructionAddresses.push_back(lineVaddr);


		if (next <= ptr) break;

		vaddr += next - ptr;
		ptr = next;
	}
}

uint64_t Disassembler::decodeLine_IA_32(uint64_t address, uint64_t vaddr) {

	uint64_t instructionBytes[15]; // instr cap of 15 bytes
	uint8_t tmp = 0x0;
	uint8_t cnt = 0;
	uint32_t width = 0, dispWidth = 0;   // bytes this immediate occupies in the stream



	bool checks[8] = { false, false, false, false, false, false, false , false}; 

	enum flags {
		hasPrefix, hasModRM, hasSIB, hasDisp, hasImm, has2Byte, hasOpsize, hasAddrSize
	};



	int endPrefix = 0, endOpcode = 0, immBegin = 0;
	uint64_t initAddress = address;

	while (cnt < 15)
		instructionBytes[cnt++] = 0x0;
	cnt = 0;

	tmp = static_cast<uint64_t>(contents.read_u8(address++));


	while (IA_32::isPrefix(tmp) && cnt < 15) {
		checks[hasPrefix] = true;
		switch (tmp) {
		case 0x66:
			checks[hasOpsize] = true; break;

		case 0x67:
			checks[hasAddrSize] = true; break;
		default:
			break;
		}

		instructionBytes[cnt++] = tmp;
		tmp = static_cast<uint64_t>(contents.read_u8(address++));

	}

	if (cnt < 15) {
		endPrefix = cnt; // past the last prefix

		if (tmp == 0x0f) checks[has2Byte] = true;

		instructionBytes[cnt++] = tmp;
		tmp = static_cast<uint64_t>(contents.read_u8(address));

		if (checks[has2Byte] && cnt < 15) {
			instructionBytes[cnt++] = tmp; address++; // reading the second byte
		}
		endOpcode = cnt; // past the last opcode byte
	}

	uint16_t opcode = static_cast<uint16_t>(instructionBytes[endPrefix]);

	uint8_t mod = 0x0, reg_op = 0x0, rm = 0x0;
	if (IA_32::hasRMbyte(static_cast<uint32_t>(opcode)) && cnt < 15){
		checks[hasModRM] = true;
		instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++)); // getting ModR/M byte

		mod = ((instructionBytes[cnt-1] & 0b11000000) >> 6);
		reg_op = ((instructionBytes[cnt-1] & 0b00111000) >> 3);
		rm = (instructionBytes[cnt-1] & 0b00000111);

		if (mod != 0b11 && rm == 0b100 && cnt < 15) {
			instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++)); // read sib;
			checks[hasSIB] = true;

		}

		checks[hasDisp] = true;
		switch (mod) {
		case 0b01:
			if (cnt < 15) {
				instructionBytes[cnt++] = static_cast<uint64_t>(static_cast<int8_t>(contents.read_u8(address++)));
				dispWidth = 1;
			}
			else checks[hasDisp] = false;
			break;
		case 0b10:
			if (cnt < 15) {
				instructionBytes[cnt++] = contents.read_u32(address); // disp32: already the full width
				dispWidth = 2;
				if (!checks[hasAddrSize])
					dispWidth += 2;				address += 4;
			}
			else checks[hasDisp] = false;
			break;
		case 0b00:
			if (cnt >= 15) checks[hasDisp] = false;
			else if (rm == 0b101 || (rm == 0b100 && (instructionBytes[endOpcode+1] & 0b00000111) == 0b101)) {
					instructionBytes[cnt++] = contents.read_u32(address);          // absolute address, no base register
					
					dispWidth = 2;
					if(!checks[hasAddrSize])
						dispWidth += 2;

					address += 4;
				break;
				}
		
		default:
			checks[hasDisp] = false;
			break;
		}
	}

	const Instruction::OpcodeInfo info = IA_32::resolvedInfo(static_cast<uint32_t>(opcode), reg_op);

	if (IA_32Mnemonic::hasImmediate(info) && cnt<15) { // set immediate field
		checks[hasImm] = true;
		immBegin = cnt;

		uint8_t immSize = 0;
		uint8_t immAddr = 0;

		bool pick[] = { false, false, false };
#define cast(name) static_cast<uint8_t>(name)

		for (int i = 0; i < 3; ++i)
			switch (info.op[i].addressingMode) {
			case cast(IA_32::ADDRESSING::I):
			case cast(IA_32::ADDRESSING::J):
			case cast(IA_32::ADDRESSING::A):
			case cast(IA_32::ADDRESSING::O):
				pick[i] = true;
				break;

			default:
				break;
			}


		for (int i = 0; i < 3; ++i)
			if (pick[i]) {

				immSize = info.op[i].size;
				immAddr = info.op[i].addressingMode;

				const bool isRelative = (immAddr == static_cast<uint8_t>(IA_32::ADDRESSING::J));

				switch (immSize) {
				case cast(IA_32::SIZE::b): width = 1; break;
				case cast(IA_32::SIZE::w): width = 2; break;
				case cast(IA_32::SIZE::v):
				case cast(IA_32::SIZE::z): width = checks[hasOpsize] ? 2 : 4; break;
				case cast(IA_32::SIZE::p): width = checks[hasOpsize] ? 4 : 6; break;
				default:
					fprintf(stderr, "Immediate IA_32::SIZE not implemented yet (opcode %#x)\n", opcode);
					break;
				}

				if (immAddr == cast(IA_32::ADDRESSING::O))
					width = checks[hasAddrSize] ? 2 : 4;


				int64_t value = 0;
				switch (width) {
				case 1:
					value = isRelative ? static_cast<int8_t>(contents.read_u8(address))
						: static_cast<uint64_t>(contents.read_u8(address));
					break;
				case 2:
					value = isRelative ? static_cast<int16_t>(contents.read_u16(address))
						: static_cast<uint64_t>(contents.read_u16(address));
					break;
				case 4:
					value = isRelative ? static_cast<int32_t>(contents.read_u32(address))
						: static_cast<uint64_t>(contents.read_u32(address));
					break;
				case 6: {
					uint32_t off = contents.read_u32(address);
					uint16_t seg = contents.read_u16(address + 4);
					value = (static_cast<uint64_t>(seg) << 32) | off;
					break;
				}
				}
				address += width;

				// rel is counted from the END of the instruction, so resolve the target only once
				// every byte of it has been consumed - address - initAddress is now its full length.
				instructionBytes[cnt++] = isRelative
					? static_cast<uint64_t>(vaddr + (address - initAddress) + value)
					: static_cast<uint64_t>(value);
			}
	}

#undef cast

	auto instruction = std::make_unique<IA_32>();
	instruction->decode(instructionBytes , checks, endPrefix, endOpcode, immBegin, width, dispWidth);
	decodedInstructions.push_back(std::move(instruction));

	return address; // address where next instruction begins

}


void Disassembler::decodeCS(FILE* outputStream) {

	decode();

	size_t i = 0;
	for (const auto& instruction : decodedInstructions)
		fprintf(outputStream, "  | %08llx:  %-24s %s\n", instructionAddresses[i++], instruction->getMachineCode().c_str(), instruction->decodeLineString().c_str());

}


