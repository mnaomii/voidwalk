#include "arch/x86_64/x86_64_decoder.hpp"

#include "arch/x86_64/x86_64_instruction.hpp"
#include "arch/x86_64/x86_64_mnemonic.hpp"

namespace voidwalk {

// Decodes one x86/x86-64 instruction: prefixes, REX or VEX, opcode (including the 0F
// and 0F 38 / 0F 3A escapes), ModRM, SIB, displacement and immediates, resolves it into
// one x86_64::Fields and appends the rendered Instruction to `decodedInstructions`.
//
// Returns the offset just past the instruction, or `fileOffset` unchanged if the
// instruction ran past end-of-file. Never reads more than 15 bytes: one that needs
// more is a "(bad)" row of the 15 it had.
uint64_t X86Decoder::decodeLine(AddressSpace& contents,
                                uint64_t fileOffset,
                                uint64_t vaddr,
                                InstructionStore& decodedInstructions) {
	using M = x86_64_Mnemonic;
	using ADDRESSING = M::ADDRESSING;
	using SIZE = M::SIZE;
	using Default64 = M::OpcodeInfo::Default64;
	struct TooLong {};

	x86_64::Fields fields;
	M::Simd simd;
	fields.is64Bit = is64Bit_;

	// Every byte goes through here, so fields.bytes always holds exactly what was consumed.
	auto nextByte = [&]() -> uint8_t {
		if (fields.length == 15) throw TooLong{};
		fields.bytes[fields.length] = contents.read_u8(fileOffset + fields.length);
		return fields.bytes[fields.length++];
	};
	auto readLittleEndian = [&](unsigned byteCount) {   // little-endian, byteCount bytes
		uint64_t value = 0;
		for (unsigned i = 0; i < byteCount; ++i) value |= static_cast<uint64_t>(nextByte()) << (8 * i);
		return value;
	};
	auto signExtend = [](uint64_t value, unsigned byteCount) { const unsigned shift = 64 - 8 * byteCount; return static_cast<int64_t>(value << shift) >> shift; };
	auto mask = [](unsigned bits) { return (bits == 64) ? ~0ull : (1ull << bits) - 1; };
	auto emit = [&] {
		decodedInstructions.push_back(std::make_unique<x86_64>(fields,simd));
		return fileOffset + fields.length;   // address where the next instr begins
	};

	try {
		// --- PREFIXES. A REX counts only as the last byte before the opcode: the CPU
		// ignores one followed by a legacy prefix, and the last of several wins.
		bool hasOpSizePrefix = false, hasAddrSizePrefix = false;
		uint8_t currentByte = nextByte();
		for (;; currentByte = nextByte()) {
			if (is64Bit_ && (currentByte & 0xF0) == 0x40) { fields.rexPrefix = currentByte; continue; }
			if (!M::isPrefix(currentByte)) break;
			fields.rexPrefix = 0;
			switch (currentByte) {
			case 0x66: hasOpSizePrefix = true; break;
			case 0x67: hasAddrSizePrefix = true; break;
			case 0xF0: fields.hasLock = true; break;
			case 0xF2: case 0xF3: fields.repPrefix = currentByte; break;
			default: fields.segmentPrefix = currentByte; break;
			}
		}

		// --- OPCODE
		fields.opcode = currentByte;
		if (currentByte == 0x0F) {
			simd.map = 1;
			fields.opcode = nextByte();
			// 0F 38 / 0F 3A are escapes into the three-byte maps: the byte after them is
			// the opcode, not a ModRM.
			if (fields.opcode == 0x38 || fields.opcode == 0x3A) {
				simd.map = (fields.opcode == 0x38) ? 2 : 3;
				fields.opcode = nextByte();
			}
			// In these maps a 66 / F2 / F3 may be the opcode's mandatory prefix rather than
			// a modifier (F2/F3 win over 66): the column the row lookup tries.
			simd.pp = (fields.repPrefix == 0xF2) ? 3 : (fields.repPrefix == 0xF3) ? 2 : hasOpSizePrefix ? 1 : 0;
		}
		// C4 / C5 open a 3- / 2-byte VEX prefix: always in long mode, and in legacy mode
		// when the next byte cannot be LES / LDS's ModRM (those take memory, mod != 11).
		else if ((currentByte == 0xC4 || currentByte == 0xC5) && (is64Bit_ || contents.read_u8(fileOffset + fields.length) >= 0xC0)) {
			const bool threeByteVex = currentByte == 0xC4;
			const uint8_t vex1 = nextByte();
			const uint8_t vex2 = threeByteVex ? nextByte() : vex1;   // C5's one byte is laid out like C4's second
			simd.vex = currentByte;
			simd.map = threeByteVex ? (vex1 & 0x1F) : 1;
			simd.pp = vex2 & 0x03;
			simd.W = threeByteVex && (vex2 & 0x80);
			simd.L = (vex2 >> 2) & 1;
			simd.vvvv = (~vex2 >> 3) & (is64Bit_ ? 15 : 7);
			// R, X, B are stored inverted. Folded into a REX they extend ModRM and SIB as usual.
			if (is64Bit_) fields.rexPrefix = 0x40 | simd.W << 3 | ((~vex1 >> 5) & (threeByteVex ? 7 : 4));
			if (simd.map < 1 || simd.map > 3) {
				fields.isInvalid = true;
				return emit();
			}
			fields.opcode = nextByte();
		}
		else if (is64Bit_ && M::opcodeTable()[currentByte].isInvalid) {
			fields.isInvalid = true;
			return emit();
		}

		// --- MOD R/M, then the real row it selects.
		const bool hasModRM = simd.vex ? !(simd.map == 1 && fields.opcode == 0x77)   // all but VZEROUPPER / VZEROALL
		                    : (simd.map == 0) ? M::opcodeTable()[fields.opcode].hasRMByte
		                    : (simd.map == 1) ? M::twoByteTable()[fields.opcode].hasRMByte
		                    : true;   // every 0F 38 / 0F 3A opcode
		uint8_t modRM = 0;
		if (hasModRM) {
			modRM = nextByte();
			fields.hasModRM = true;
			fields.mod = modRM >> 6;
			fields.reg = ((modRM >> 3) & 7) | (fields.rexPrefix & 4) << 1;   // REX.R
			fields.rm  = (modRM & 7) | (fields.rexPrefix & 1) << 3;          // REX.B
		}
		// Pass the whole ModRM byte: groups key on reg internally, x87 needs it all.
		fields.info = simd.vex ? M::vexResolvedInfo(fields.opcode, modRM, simd)
		       : (simd.map == 0) ? M::resolvedInfo(fields.opcode, modRM, is64Bit_)
		       : (simd.map == 1) ? M::twoByteResolvedInfo(fields.opcode, modRM, simd)
		       : M::threeByteResolvedInfo(fields.opcode, simd);

		// --- WIDTHS. A mandatory prefix is part of the opcode: it neither prints nor resizes.
		// VEX.pp always is one; a legacy column only when the row took it. Untaken, it is no column.
		simd.mandatory = simd.vex ? simd.pp != 0 : fields.info.mandatoryPrefix != 0;
		if (!simd.mandatory) simd.pp = 0;
		else if (simd.pp == 1) hasOpSizePrefix = false;
		else fields.repPrefix = 0;
		const bool rexW = fields.rexPrefix & 8;
		const Default64 default64 = is64Bit_ ? fields.info.default64 : Default64::None;
		fields.opSize = (rexW || default64 == Default64::f64) ? 64 : hasOpSizePrefix ? 16 : (default64 == Default64::d64) ? 64 : 32;
		// Long mode's 67 selects 32-bit addressing; only legacy mode drops to 16.
		fields.addrSize = is64Bit_ ? (hasAddrSizePrefix ? 32 : 64) : (hasAddrSizePrefix ? 16 : 32);

		// --- SIB / DISPLACEMENT
		if (fields.hasModRM && fields.mod != 3) {
			const uint8_t rmField = modRM & 7;
			unsigned displacementSize = (fields.mod == 1) ? 1 : 0;
			if (fields.addrSize == 16) {
				if (fields.mod == 2 || (fields.mod == 0 && rmField == 6)) displacementSize = 2;
			}
			else {
				if (rmField == 4) {
					const uint8_t sib = nextByte();
					fields.hasSIB = true;
					fields.scale = 1 << (sib >> 6);                        // 00/01/10/11 -> *1/*2/*4/*8
					fields.index = ((sib >> 3) & 7) | (fields.rexPrefix & 2) << 2;    // REX.X
					fields.base  = (sib & 7) | (fields.rexPrefix & 1) << 3;           // REX.B
				}
				if (fields.mod == 2 || (fields.mod == 0 && (rmField == 5 || (fields.hasSIB && (fields.base & 7) == 5)))) displacementSize = 4;
			}
			if (displacementSize) {
				fields.hasDisplacement = true;
				fields.displacement = signExtend(readLittleEndian(displacementSize), displacementSize);
			}
		}

		// 3DNow! (0F 0F /r ib): the trailing byte is no immediate but the real opcode.
		if (!simd.vex && simd.map == 1 && fields.opcode == 0x0F) fields.info = M::threeDNowTable()[nextByte()];

		// --- IMMEDIATE(s), in operand order. L is a register number in an imm8's top bits.
		int immIndex = 0;
		for (const auto& operand : fields.info.operands) {
			const ADDRESSING mode = operand.addressingMode;
			if ((mode != ADDRESSING::I && mode != ADDRESSING::J && mode != ADDRESSING::A && mode != ADDRESSING::O && mode != ADDRESSING::L) || immIndex == 2)
				continue;

			unsigned byteCount;
			switch (operand.size) {
			case SIZE::b: case SIZE::bs: byteCount = 1; break;   // bs occupies 1 byte too
			case SIZE::w: byteCount = 2; break;
			case SIZE::d: byteCount = 4; break;
			case SIZE::v: byteCount = rexW ? 8 : (fields.opSize == 16) ? 2 : 4; break;
			default:      byteCount = (fields.opSize == 16) ? 2 : 4; break;   // z, and p's offset
			}
			if (mode == ADDRESSING::O) byteCount = fields.addrSize / 8;

			uint64_t value = readLittleEndian(byteCount);
			if (mode == ADDRESSING::A)
				value |= readLittleEndian(2) << 32;   // ptr16:16 / ptr16:32: the segment follows the offset
			else if (mode == ADDRESSING::J) {
				// rel is counted from the END of the instruction - it is always the last
				// field, so fields.length is now the full length. IP wraps at the operand width.
				value = vaddr + fields.length + signExtend(value, byteCount);
				if (!is64Bit_) value &= mask(fields.opSize == 16 ? 16 : 32);
			}
			else if (mode == ADDRESSING::I && (operand.size == SIZE::bs || (operand.size == SIZE::z && fields.opSize == 64)))
				value = static_cast<uint64_t>(signExtend(value, byteCount)) & mask(fields.opSize);   // the CPU sign-extends these
			fields.immediates[immIndex++] = value;
		}

		return emit();
	}
	catch (const std::length_error&) { return fileOffset; }
	catch (const TooLong&) {
		fields.isInvalid = true;
		return emit();
	}
}

} // namespace voidwalk
