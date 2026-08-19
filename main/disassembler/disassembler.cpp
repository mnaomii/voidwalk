#include "disassembler.hpp"
#include "../address-space/address_space.hpp"
#include "miscellaneous/sections/base/header.hpp"
#include "mnemonic/x86_64/x86_64-instr.hpp"
#include "mnemonic/x86_64/x86_64-mnemonic.hpp"


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

	uint64_t instructionBytes[15] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // originalDisp = 0; // instr cap of 15 bytes
	uint8_t tmp = 0x0;
	uint8_t cnt = 0;
	uint32_t width = 0, dispWidth = 0;   // bytes this immediate occupies in the stream



	bool checks[8] = { false, false, false, false, false, false, false , false}; 

	enum flags {
		hasPrefix, hasModRM, hasSIB, hasDisp, hasImm, has2Byte, hasOpsize, hasAddrSize
	};



	int endPrefix = 0, endOpcode = 0, immBegin = 0;
	uint64_t initAddress = address;

	try {
		tmp = static_cast<uint64_t>(contents.read_u8(address++));


		// --- PREFIXES
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


		// --- OPCODE
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


		uint64_t opcode = instructionBytes[endOpcode - 1];

		const Instruction::OpcodeInfo& outer = checks[has2Byte]
			? IA_32Mnemonic::twoByteTable()[opcode]
			: IA_32Mnemonic::opcodeTable()[opcode];

		Instruction::OpcodeInfo opcodeInfo = outer;



		// --- MOD R/M
		uint8_t mod = 0x0, reg_op = 0x0, rm = 0x0;
		if (outer.hasRMByte && cnt < 15) {
			checks[hasModRM] = true;
			instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++)); // getting ModR/M byte

			const uint8_t modrm = static_cast<uint8_t>(instructionBytes[cnt - 1]);
			mod = ((modrm & 0b11000000) >> 6);
			reg_op = ((modrm & 0b00111000) >> 3);
			rm = (modrm & 0b00000111);

			// Pass the whole ModRM byte: groups key on reg internally, x87 needs it all.
			opcodeInfo = checks[has2Byte]
				? IA_32Mnemonic::twoByteResolvedInfo(static_cast<uint32_t>(opcode), modrm)
				: IA_32Mnemonic::resolvedInfo(static_cast<uint32_t>(opcode), modrm);


			// --- SIB
			if (mod != 0b11 && rm == 0b100 && cnt < 15 && !checks[hasAddrSize]) {
				instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++)); // read sib;
				checks[hasSIB] = true;

			}


			// --- DISPLACEMENT
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
					if (!checks[hasAddrSize])
						instructionBytes[cnt++] = static_cast<uint64_t>(static_cast<int32_t>(contents.read_u32(address))); // disp32/16: already the full width
					else instructionBytes[cnt++] = static_cast<uint64_t>(static_cast<int16_t>(contents.read_u16(address)));
					dispWidth = 2;
					if (!checks[hasAddrSize])
						dispWidth += 2;				address += 4;
				}
				else checks[hasDisp] = false;
				break;
			case 0b00:
				if (cnt >= 15) checks[hasDisp] = false;
				else if (rm == 0b101 || (rm == 0b100 && (instructionBytes[endOpcode + 1] & 0b00000111) == 0b101)) {

					dispWidth = 2;
					if (!checks[hasAddrSize])
						dispWidth += 2;


					if (dispWidth == 2) {	// absolute address, no base register
						instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u16(address));
						address += 2;
					}
					else {
						instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u32(address));
						address += 4;
					}

					break;
				}

			default:
				checks[hasDisp] = false;
				break;
			}
		}


		uint64_t rawImmediates[] = { 0,0,0 };
		// --- IMMEDIATE(s)
		if (IA_32Mnemonic::hasImmediate(opcodeInfo) && cnt < 15) { // set immediate field
			checks[hasImm] = true;
			immBegin = cnt;

			uint8_t immSize = 0;
			uint8_t immAddr = 0;

			bool pick[] = { false, false, false };
#define cast(name) static_cast<uint8_t>(name)

			for (int i = 0; i < 3; ++i)
				switch (opcodeInfo.op[i].addressingMode) {
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

					immSize = opcodeInfo.op[i].size;
					immAddr = opcodeInfo.op[i].addressingMode;

					const bool isRelative = (immAddr == static_cast<uint8_t>(IA_32::ADDRESSING::J));

					switch (immSize) {
					case cast(IA_32::SIZE::b): case cast(IA_32::SIZE::bs): width = 1; break;   // bs occupies 1 byte too
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

					// 83 /r ib, 6B, 6A: the imm8 is sign-extended to the operand width (16-bit
					// under a 66h prefix, else 32-bit). Widen the byte just read; length stays 1,
					// so this is display-only and cannot desync the sweep.
					if (immSize == cast(IA_32::SIZE::bs))
						value = checks[hasOpsize] ? static_cast<uint16_t>(static_cast<int8_t>(static_cast<uint8_t>(value)))
							: static_cast<uint32_t>(static_cast<int8_t>(static_cast<uint8_t>(value)));

					address += width;

					// rel is counted from the END of the instruction, so resolve the target only once
					// every byte of it has been consumed - address - initAddress is now its full length.
					//originalDisp = value;
					instructionBytes[cnt] = isRelative
						? static_cast<uint64_t>(vaddr + (address - initAddress) + value)
						: static_cast<uint64_t>(value);
					rawImmediates[cnt - immBegin] = (isRelative) ? value : 0x0;
					++cnt;
				}
		}

		auto instruction = std::make_unique<IA_32>();
		instruction->decode(instructionBytes, checks, endPrefix, endOpcode, immBegin, width, dispWidth, rawImmediates);
		decodedInstructions.push_back(std::move(instruction));

		return address; // address where the next instr begins
#undef cast

	}
	catch (std::length_error& ) { return initAddress; }

}


void Disassembler::decodeCS(FILE* outputStream) {

	decode();

	size_t i = 0;
	for (const auto& instruction : decodedInstructions)
		fprintf(outputStream, "  | %08llx:  %-24s %s\n", instructionAddresses[i++], instruction->getMachineCode().c_str(), instruction->decodeLineString().c_str());

}


