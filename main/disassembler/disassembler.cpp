#include "disassembler.hpp"
#include "../address-space/address_space.hpp"
#include "miscellaneous/sections/base/header.hpp"
#include "mnemonic/x86_64/x86_64-instr.hpp"
#include "mnemonic/x86_64/x86_64-mnemonic.hpp"

#include <iomanip>

void Disassembler::decode(std::stop_token stopToken) {
	decodedInstructions.clear();
	instructionAddresses.clear();
	readyCount.store(0, std::memory_order_relaxed);

	const uint64_t start = baseSections._text.getOffset();
	const uint64_t end = start + baseSections._text.getSize();

	// Worst case is one instruction per byte of .text. Reserving that many entries
	// up front means neither vector ever reallocates during the sweep, so a reader
	// thread (GUI/TUI) can safely hold a reference into them and index [0,
	// readyCount) while we keep appending: the buffer never moves out from under it.
	// This reserve is what makes the lock-free handoff below correct.
	const auto worst = static_cast<size_t>(baseSections._text.getSize());
	decodedInstructions.reserve(contents.size());
	instructionAddresses.reserve(contents.size());

	uint64_t ptr = start;
	uint64_t vaddr = baseSections._text.getVaddr();

	while (ptr < end) {
		if (stopToken.stop_requested()) break; // re-open / shutdown asked to bail

		const uint64_t lineVaddr = vaddr;
		uint64_t next = decodeLine(ptr, vaddr);

		while (instructionAddresses.size() < decodedInstructions.size())
			instructionAddresses.push_back(lineVaddr);

		// Both vectors are equal length and consistent for [0, size) here, so
		// publish. The release pairs with the acquire in readyInstructions(): a
		// reader that sees this count also sees every vector write behind it.
		readyCount.store(decodedInstructions.size(), std::memory_order_release);

		if (next <= ptr) break;

		emitDecodedLine();

		vaddr += next - ptr;
		ptr = next;

	}

	//decodedInstructions.shrink_to_fit();
	//instructionAddresses.shrink_to_fit();
}

uint64_t Disassembler::decodeLine_x86_64(uint64_t address, uint64_t vaddr, bool is64Bit) {

	uint64_t instructionBytes[15]{}; // originalDisp = 0; // instr cap of 15 bytes
	uint8_t tmp = 0x0;
	uint8_t cnt = 0;
	uint32_t immWidth[3]{}, dispWidth = 0;   // bytes this immediate/disp occupies in the stream

	uint64_t rawImmediates[3]{};


	uint64_t initAddress = address;


	bool rexBits[4]{};
	int positions[4]{};
	bool checks[11]{};



	enum flagsIdx {
		hasPrefix, hasModRM, hasSIB, hasDisp, hasImm, has2Byte, hasOpsize, hasAddrSize, hasREX, hasAdditionalEscape, isInvalid
	};

	enum rexBitsIdx {
		b, x, r, w
	};

	enum positionsIdx {
		endPrefix, endOpcode, immBegin, rexBegin
	};


	try {
		tmp = static_cast<uint64_t>(contents.read_u8(address++));


		// --- PREFIXES
		while (x86_64::isPrefix(tmp) && cnt < 15) {
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

		positions[endPrefix] = cnt; // past the last prefix


		// --- VEX will go here..

		// --- REX



		if (is64Bit && tmp >= 0x40 && tmp <= 0x4f && cnt < 15)
		{
				checks[hasREX] = true;

				instructionBytes[cnt++] = tmp; 

				tmp = static_cast<uint64_t>(contents.read_u8(address++));
				
				positions[rexBegin] = cnt - 1; // *exactly* the last REX position - where the REX reading should start

				auto aux = instructionBytes[positions[rexBegin]];
				int cnt_aux = 0;
				while (aux != 0b0100) { // filling the rex struct with its individual values
					rexBits[cnt_aux] = aux & 0b1;
					aux >>= 1;
					cnt_aux++;
				}
				if (rexBits[w]) checks[hasOpsize] = false;  // REX.W nullifies OpsizePFX if present
			
		}

		if (cnt < 15) {

			// --- OPCODE

			if (x86_64_Mnemonic::opcodeTable()[tmp].isInvalid && cnt < 15 && is64Bit) {
				checks[isInvalid] = true;
				instructionBytes[cnt++] = tmp;
			}
			else {
				instructionBytes[cnt++] = tmp;

				if (tmp == 0x0f) {
					checks[has2Byte] = true;
					tmp = contents.read_u8(address++);

					// 3rd operand byte - further escape opcodes : sse, avx, vex ISA's etc
				if(cnt<15)
					  instructionBytes[cnt++] = tmp;

					// 0F 38 / 0F 3A are escapes into the three-byte maps: the byte after
					// them is the opcode, not a ModRM. Eat it here, or the ModRM read
					// below swallows the opcode and every length from this point on is
					// wrong. The maps themselves are not built yet - threeByteRow()
					// supplies the shape so the sweep stays aligned regardless.
					if ((tmp == 0x38 || tmp == 0x3a) && cnt < 15) {
						checks[hasAdditionalEscape] = true;
						instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++));
					}

				}

			}

			positions[endOpcode] = cnt; // past the last opcode byte
		}

		if (cnt<15) { // continue instruction processing
			uint64_t opcode = instructionBytes[positions[endOpcode] - 1];



			// After a three-byte escape, `opcode` is a byte from the 0F 38 / 0F 3A map -
			// indexing twoByteTable() with it would read an unrelated row, so take the
			// stand-in shape instead. instructionBytes[endOpcode - 2] is the escape byte.
			const Instruction::OpcodeInfo& outer = checks[hasAdditionalEscape]
				? x86_64_Mnemonic::threeByteRow(instructionBytes[positions[endOpcode] - 2] == 0x3a)
				: ( checks[has2Byte]
					? x86_64_Mnemonic::twoByteTable()[opcode]
					: ( (is64Bit && opcode == 0x63) ? x86_64_Mnemonic::resolvedInfo(0x63, 0,true) : x86_64_Mnemonic::opcodeTable()[opcode]) );

			Instruction::OpcodeInfo opcodeInfo = outer;

			if (opcodeInfo.def64 == Instruction::OpcodeInfo::Default64::f64 && is64Bit) checks[hasOpsize] = false;

			// --- MOD R/M
			uint8_t mod = 0x0, reg_op = 0x0, rm = 0x0;
			if (outer.hasRMByte && cnt < 15 && !checks[isInvalid]) {
				checks[hasModRM] = true;
				instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u8(address++)); // getting ModR/M byte

				const uint8_t modrm = static_cast<uint8_t>(instructionBytes[cnt - 1]);
				mod = ((modrm & 0b11000000) >> 6);
				reg_op = ((modrm & 0b00111000) >> 3);
				rm = (modrm & 0b00000111);

				// Pass the whole ModRM byte: groups key on reg internally, x87 needs it all.
				// Three-byte rows have no group to resolve against - keep the shape.
				opcodeInfo = checks[hasAdditionalEscape]
					? outer
					: ( checks[has2Byte]
						? x86_64_Mnemonic::twoByteResolvedInfo(static_cast<uint32_t>(opcode), modrm)
						: x86_64_Mnemonic::resolvedInfo(static_cast<uint32_t>(opcode), modrm, is64Bit) );


				// --- SIB
				if (mod != 0b11 && rm == 0b100 && cnt < 15 && ((is64Bit)? true : !checks[hasAddrSize])) {
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
							if (!checks[hasAddrSize]) { // disp32/16: already the full width
								instructionBytes[cnt++] = static_cast<uint64_t>(static_cast<int32_t>(contents.read_u32(address)));
								address += 2;
							}
							else instructionBytes[cnt++] = static_cast<uint64_t>(static_cast<int16_t>(contents.read_u16(address)));

							dispWidth = 2;
							if (!checks[hasAddrSize])
								dispWidth += 2;
							address += 2;
						}
						else checks[hasDisp] = false;
						break;
					case 0b00:
						if (cnt >= 15) checks[hasDisp] = false;
						else if (rm == (checks[hasAddrSize] ? 0b110 : 0b101)
							|| (!checks[hasAddrSize] && rm == 0b100 && (instructionBytes[positions[endOpcode] + 1] & 0b00000111) == 0b101)) {

							dispWidth = 2;
							if (!checks[hasAddrSize])
								dispWidth += 2;


							if (dispWidth == 2) {	// absolute address, no base register
								instructionBytes[cnt++] = static_cast<uint64_t>(contents.read_u16(address));
								address += 2;
							}
							else if (dispWidth == 4) {
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


			int j = 0;
			// --- IMMEDIATE(s)
			if (x86_64_Mnemonic::hasImmediate(opcodeInfo) && cnt < 15 && !checks[isInvalid]) { // set immediate field
				checks[hasImm] = true;
				positions[immBegin] = cnt;

				uint8_t immSize = 0;
				uint8_t immAddr = 0;

				bool pick[] = { false, false, false };
#define cast(name) static_cast<uint8_t>(name)

				for (int i = 0; i < 3; ++i)
					switch (opcodeInfo.op[i].addressingMode) {
						case cast(x86_64::ADDRESSING::I):
						case cast(x86_64::ADDRESSING::J):
						case cast(x86_64::ADDRESSING::A):
						case cast(x86_64::ADDRESSING::O):
							pick[i] = true;
							break;

						default:
							break;
					}


				for (int i = 0; i < 3; ++i)
					if (pick[i]) {

						immSize = opcodeInfo.op[i].size;
						immAddr = opcodeInfo.op[i].addressingMode;

						const bool isRelative = (immAddr == static_cast<uint8_t>(x86_64::ADDRESSING::J));

						switch (immSize) {
							case cast(x86_64::SIZE::b): case cast(x86_64::SIZE::bs): immWidth[j] = 1; break;   // bs occupies 1 byte too
							case cast(x86_64::SIZE::w): immWidth[j] = 2; break;
							case cast(x86_64::SIZE::v):

								if (rexBits[w])
								{
									immWidth[j] = 8;
									if (checks[hasOpsize] && opcodeInfo.def64 == Instruction::OpcodeInfo::Default64::d64)
										immWidth[j] = 2;
								}
								else immWidth[j] = checks[hasOpsize] ? 2 : 4; break;

							case cast(x86_64::SIZE::z):
								immWidth[j] = checks[hasOpsize] ? 2 : 4; break;
							case cast(x86_64::SIZE::p): immWidth[j] = checks[hasOpsize] ? 4 : 6; break;
							default:
								fprintf(stderr, "Immediate x86_64::SIZE not implemented yet (opcode %llx)\n", opcode);
								break;
						}

						if (immAddr == cast(x86_64::ADDRESSING::O))
							immWidth[j] = is64Bit ? (checks[hasAddrSize] ? 4 : 8) : (checks[hasAddrSize] ? 2 : 4);

						int64_t value = 0;
						switch (immWidth[j]) {
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
							case 8:
								value = contents.read_u64(address);
						}

						// Sign extension of Imm8, based on the custom Size::bs

						if (immSize == cast(x86_64::SIZE::bs)) {
							value = static_cast<int8_t>(value);
							if (is64Bit && rexBits[w]) value = static_cast<uint64_t>(value);
							else if (checks[hasOpsize]) value = static_cast<uint16_t>(value);
							else value = static_cast<uint32_t>(value);
						}


						address += immWidth[j++];

						// rel is counted from the END of the instruction, so resolve the target only once
						// every byte of it has been consumed - address - initAddress is now its full length.
						//originalDisp = value;
						instructionBytes[cnt] = isRelative
							? static_cast<uint64_t>(vaddr + (address - initAddress) + value)
							: static_cast<uint64_t>(value);
						rawImmediates[cnt - positions[immBegin]] = (isRelative) ? value : 0x0;
						++cnt;
					}
			}
		}

		auto instruction = std::make_unique<x86_64>();
		instruction->decode(instructionBytes, checks, positions, rexBits, immWidth, dispWidth, rawImmediates, is64Bit);
		decodedInstructions.push_back(std::move(instruction));

		return address; // address where the next instr begins
#undef cast

	}
	catch (std::length_error& ) { return initAddress; }

}



// prints the most recenlty decoded line to the embedded streams
void Disassembler::emitDecodedLine() {

	for(auto stream : outputStreams)
		*stream << std::hex << std::setfill('0') << std::setw(8)
		<< instructionAddresses[instrDecodePos] << ":  "
		<< std::setfill(' ') << std::left << std::setw(24)
		<< decodedInstructions[instrDecodePos]->getMachineCode() << ' '
		<< decodedInstructions[instrDecodePos]->decodeLineString() << '\n';

	instrDecodePos++;
}


