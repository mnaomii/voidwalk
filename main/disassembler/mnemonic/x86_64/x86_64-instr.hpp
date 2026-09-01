#include "../../../address-space/address_space.hpp"
#include "../instruction.hpp"
#include "x86_64-mnemonic.hpp"
#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <format>
#include <stdexcept>
#pragma once

// FORMAT:

// prefix : 0-3 bytes
// opcode : 1-3 bytes
// Mod R/M : 0-1 bytes : Mod + Reg/Opcode + R/M (little endian)
// SIB : 0-1 bytes : Scale + Index + Base (little endian)
// Displacement : 0-4 bytes
// Immediate : 0-4 bytes


#define PREFIX_MAX 255
#define PREFIX_UNINITIALIZED 0
#define OPCODE_MAX 255


// in IA-32 -> default operand size : 32bits, change with 66f / 67f

class x86_64: public Instruction{


public:

	// aliases
	using ADDRESSING = x86_64_Mnemonic::ADDRESSING;
	using SIZE = x86_64_Mnemonic::SIZE;
	using REGISTER = x86_64_Mnemonic::REGISTER;
	using OPCODE = x86_64_Mnemonic::OPCODE;
	using Prefix = x86_64_Mnemonic::Prefix;

static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable()	{ return x86_64_Mnemonic::opcodeTable(); }
static const std::array<std::string_view, 256>& prefixTable()			{ return x86_64_Mnemonic::prefixTable(); }
static bool hasRMbyte(uint32_t op)										{ return opcodeTable()[op].hasRMByte; }
static Instruction::OpcodeInfo resolvedInfo(uint64_t op, uint8_t modrm)	{ return x86_64_Mnemonic::resolvedInfo(static_cast<uint32_t>(op), modrm); }
static std::string_view opcodeStrOf(uint64_t op, uint8_t modrm)			{ return resolvedInfo(op, modrm).text; }
static std::string_view opcodeStr16Of(uint64_t op, uint8_t modrm) { return resolvedInfo(op, modrm).text16; }
static const std::array<Instruction::OpcodeInfo, 256>& twoByteTable()	{ return x86_64_Mnemonic::twoByteTable(); }
static bool hasRMbyte2(uint32_t op2)									{ return twoByteTable()[op2].hasRMByte; }
static Instruction::OpcodeInfo twoByteResolvedInfo(uint64_t op2, uint8_t modrm)	{ return x86_64_Mnemonic::twoByteResolvedInfo(static_cast<uint32_t>(op2), modrm); }
static std::string_view twoByteStrOf(uint64_t op2, uint8_t modrm)			{ return twoByteResolvedInfo(op2, modrm).text; }
static std::string_view twoByteStr16Of(uint64_t op2, uint8_t modrm) { return twoByteResolvedInfo(op2, modrm).text16; }

static std::string_view prefixStrOf(uint8_t op)							{ if (isPrefix(op)) return prefixTable()[op];  else return ""; }
static bool isPrefix(uint8_t op)										{ return !prefixTable()[op].empty(); }


x86_64() {};

inline void decode( uint64_t (&instructionBytes)[15], const bool (&checks)[11], const int (&positions)[4], const bool (&rexBits)[4], const uint32_t (&immWidth)[3], const uint32_t& dispWidth, const uint64_t(&rawImmediates)[3], bool is64Bit) {
	
	uint8_t mod = 0, reg_op = 0, rm = 0, scale = 0, index = 0, base = 0;
	enum flagsIdx { 
		hasPrefix, hasModRM, hasSIB, hasDisp, hasImm, has2Byte, hasOpsize, hasAddrSize, hasREX, hasAdditionalEscape, isInvalid
	};

	enum rexBitsIdx {
		b, x, r, w
	};

	enum positionsIdx {
		prefixEnd, opcodeEnd, immBegin, rexBegin
	};

	if (checks[isInvalid] || positions[opcodeEnd] == 0) {
		instructionStr = "(bad)";
		for (int i = 0; i < positions[opcodeEnd]; ++i)
			machineCode += std::format("{:02x}", instructionBytes[i]);
		return;
	}

	// CET landing pads: F3 0F 1E FA (ENDBR64) and F3 0F 1E FB (ENDBR32). Handled up
	// front for two reasons. The opcode table is keyed by the opcode byte alone, so
	// 0F 1E resolves to its base meaning - the reserved hint-NOP - and the F3 that
	// promotes it to ENDBR is a mandatory prefix, not a REP. And the prefix loop
	// below would print that F3 as "REP" before the opcode is ever looked at.
	if (checks[has2Byte] && checks[hasModRM]
	    && static_cast<uint8_t>(instructionBytes[positions[opcodeEnd] - 1]) == 0x1E) {

		const uint8_t landing = static_cast<uint8_t>(instructionBytes[positions[opcodeEnd]]);
		bool repPrefix = false;
		for (int p = 0; checks[hasPrefix] && p < positions[prefixEnd]; ++p)
			if (instructionBytes[p] == 0xF3) repPrefix = true;

		if (repPrefix && (landing == 0xFA || landing == 0xFB)) {
			instructionStr = (landing == 0xFA) ? "ENDBR64" : "ENDBR32";
			machineCode = std::format("f3 0f 1e {:02x} ", landing);
			return;
		}
	}

	std::string segment = "", fmt = "";
	int i = 0;
	for (i; i < positions[prefixEnd] && checks[hasPrefix] ; ++i) {

		switch (instructionBytes[i]) {
		case 0x2e:
			segment = "CS:";
			break;

		case 0x36:
			segment = "SS:";	
			break;

		case 0x3e:
			segment = "DS:";
			break;

		case 0x26:
			segment = "ES:";
			break;

		case 0x64:
			segment = "FS:";
			break;

		case 0x65:
			segment = "GS:";
			break;

		case 0x66:
		case 0x67:
			break;

		default :
			instructionStr += prefixStrOf(static_cast<uint8_t>(instructionBytes[i]));
			instructionStr += " ";
			break;
		}
		machineCode += std::format("{:02x} ", instructionBytes[i]);

	}

	// vex prefix processing will go here..

	// append all of the rex bytes read
	if (is64Bit && checks[hasREX]) machineCode += std::format("{:02x} ", instructionBytes[positions[rexBegin]]);

	Instruction::OpcodeInfo opcode;
	if (checks[hasModRM]) {

		mod = static_cast<uint8_t>((instructionBytes[positions[opcodeEnd]] & 0b11000000) >> 6);

		reg_op = static_cast<uint8_t>((instructionBytes[positions[opcodeEnd]] & 0b00111000) >> 3);
		if (is64Bit && rexBits[r]) reg_op |= 8;   // REX.R -> high bit of ModRM.reg

		rm = static_cast<uint8_t>(instructionBytes[positions[opcodeEnd]] & 0b00000111);
		if (is64Bit && rexBits[b] && !checks[hasSIB]) rm |= 8;   // REX.B -> high bit of ModRM.rm (reg form)

	}
	// Hand the resolver the whole ModRM byte (reg for groups, full byte for x87);
	// when there is no ModRM the value is unused (the opcode is neither a group nor x87).
	const uint8_t modrm = checks[hasModRM] ? static_cast<uint8_t>(instructionBytes[positions[opcodeEnd]]) : 0;
	// A three-byte escape leaves the opcode in the 0F 38 / 0F 3A map, which the 0F
	// table does not cover - take the same stand-in the byte-eater used, so the name
	// and the length agree.
	if (checks[hasAdditionalEscape])
		opcode = x86_64_Mnemonic::threeByteRow(instructionBytes[positions[opcodeEnd] - 2] == 0x3a);
	else if (checks[has2Byte]) opcode = twoByteResolvedInfo(instructionBytes[positions[opcodeEnd] -1], modrm);
	else opcode = resolvedInfo(instructionBytes[positions[opcodeEnd] -1], modrm);
		

	machineCode += (checks[has2Byte]) ? "0f " : "";
	if (checks[hasAdditionalEscape])
		machineCode += std::format("{:02x} ", instructionBytes[positions[opcodeEnd] - 2]);
	machineCode += std::format("{:02x} ", instructionBytes[positions[opcodeEnd] - 1]);
 	machineCode += (checks[hasModRM]) ? std::format("{:02x} ", instructionBytes[positions[opcodeEnd]]) : ""; // append the  ModRM byte

	// add the opcode to the decode string
	if (is64Bit && rexBits[w]) instructionStr += (rexBits[w] && !opcode.text64.empty()) ? opcode.text64 : opcode.text;
	else instructionStr += (checks[hasOpsize] && !opcode.text16.empty()) ? opcode.text16 : opcode.text;
	instructionStr += " ";




	// The one memory operand, if any: [base + index*scale + disp]. Whichever operand asks
	// for E or M takes this text; with mod 11 there is no memory and E is a plain register.

	fmt = "";
	std::string memory; uint64_t displacement = 0x0;
	if (checks[hasModRM] && mod != 0b11) {

		if (checks[hasSIB]) {
			machineCode += std::format("{:02x} ", instructionBytes[positions[opcodeEnd] + 1]);

			scale = 1u << ((instructionBytes[positions[opcodeEnd] + 1] & 0b11000000) >> 6);          // 00/01/10/11 -> *1/*2/*4/*8

			index = static_cast<uint32_t>((instructionBytes[positions[opcodeEnd] + 1] & 0b00111000) >> 3);
			if (is64Bit && rexBits[x]) index |= 8;   // REX.X -> high bit of SIB.index

			base  = static_cast<uint32_t>(instructionBytes[positions[opcodeEnd] + 1] & 0b00000111);
			if (is64Bit && rexBits[b]) base |= 8;   // REX.B -> high bit of SIB.base

		}
		if (checks[hasDisp]) {
			displacement = checks[hasSIB] ? instructionBytes[positions[opcodeEnd] + 2] : instructionBytes[positions[opcodeEnd] + 1];
			uint64_t aux = displacement;
			if ((mod & 7) == 0 && (rm & 7) == 5 && is64Bit) aux = static_cast<uint64_t>(static_cast<int32_t>(aux));
			for (int k = 0; k < dispWidth; ++k)
				fmt += std::format("{:02x} ", (displacement >> (8 * k)) & 0xff);

			displacement = aux;

			machineCode += fmt;
		}
		
		memory += "[";
		if (checks[hasSIB]) {
			if(! ((base & 7) == 5 && (mod & 7) == 0))
			memory += x86_64_Mnemonic::registerOf(base, checks[hasAddrSize], checks[hasREX], is64Bit);
			if (index != static_cast<uint32_t>(REGISTER::SP)) {                      // index 100 = no index
				memory += (!((base & 7) == 5 && (mod & 7) == 0)) ? " + " : "";
				memory += x86_64_Mnemonic::registerOf(index, checks[hasAddrSize], checks[hasREX], is64Bit) + "*" + std::to_string(static_cast<int>(scale));
			}
		}
		else if (!((mod & 7) == 0b00 && (rm & 7) == (checks[hasAddrSize] ? 0b110 :  0b101))) {                                    // that form has no base register
			memory += checks[hasAddrSize] ? x86_64_Mnemonic::registerOf16(rm, true):  x86_64_Mnemonic::registerOf(rm, checks[hasAddrSize], checks[hasREX], is64Bit);
		}
		if (checks[hasDisp]) {
			// Nothing printed yet means there is no base register: the displacement is an
			// absolute address, not an offset, so it stays unsigned.
			if (memory.size() == 1) {
				memory += ( (mod & 7) == 0 && (rm & 7) == 5 && is64Bit) ? "RIP + " : "";
				memory += std::format("{:#x}", displacement);
			}
			else {
				const int64_t d = static_cast<int64_t>(displacement);   // offset from a register: signed
				memory += (d < 0)
					? " - " + std::format("{:#x}", -static_cast<int64_t>(d))
					: " + " + std::format("{:#x}", d);
			}
		}
		memory += "]";
	}


	std::string operands[3];
	Instruction::TableOperand op[3] = { opcode.op[0], opcode.op[1], opcode.op[2] };

	uint64_t immediate[3] = { 0,0,0 };
	int j = 0;
	int counter = 0;
	while ( checks[hasImm] && j < 3 && counter < 3) {
		fmt = "";
		const auto m = static_cast<ADDRESSING>(op[counter].addressingMode);

		if (m == ADDRESSING::I || m == ADDRESSING::J || m == ADDRESSING::A || m == ADDRESSING::O) {
			immediate[j] =  instructionBytes[ positions[immBegin] + j];

			const uint64_t hexValue = (m == ADDRESSING::J) ? rawImmediates[j] : immediate[j];

			for(int k=0; k<immWidth[j]; ++k)
				fmt+=std::format("{:02x} ", (hexValue >> (8 * k)) & 0xff);
			machineCode += fmt;
			++j;
		}
		++counter;
	}

	j = 0;

	for ( i = 0; i < 3; ++i) {

		switch (static_cast<ADDRESSING>(op[i].addressingMode)) {

		case ADDRESSING::E:
		case ADDRESSING::M:		// memory
			operands[i] = ((mod & 7) == 0b11) ? x86_64_Mnemonic::registerOf(rm, op[i].size,checks[hasOpsize], checks[hasREX], rexBits[w]) : (segment + memory);
			if (mod != 0b11) segment = "";
			break;

		case ADDRESSING::G:
			operands[i] = x86_64_Mnemonic::registerOf(reg_op, op[i].size, checks[hasOpsize], checks[hasREX], rexBits[w]);
			break;

		case ADDRESSING::S:		// segment
			operands[i] = x86_64_Mnemonic::segmentOf(reg_op);
			break;

		case ADDRESSING::J:      // relative offset
		case ADDRESSING::I:      // immediate
			operands[i] = std::format("{:#x}", immediate[j++]);
			break;

		case ADDRESSING::O:      // moffs
			operands[i] = segment + std::format("[{:#x}]", immediate[j++]);
			break;

		case ADDRESSING::A:      // far pointer
			operands[i] = std::format("{:#x}:{:#x}", immediate[j] >> 32, immediate[j] & 0xffffffff);
			++j;
			break;

		case ADDRESSING::Z: {
			const uint8_t opLow = instructionBytes[positions[opcodeEnd] - 1] & 0x07;
			const uint8_t reg = opLow | (rexBits[b] << 3);      // REX.B extends to R8..R15
			// 64-bit width only when REX.W, or a d64 default (PUSH/POP) not overridden by 0x66.
			// registerOf then applies: REX.W(64) > 0x66(16) > 32.
			const bool wide64 = is64Bit
				&& (rexBits[w] || (opcode.def64 == Instruction::OpcodeInfo::Default64::d64 && !checks[hasOpsize]));
			operands[i] = x86_64_Mnemonic::registerOf(reg, op[i].size, checks[hasOpsize], checks[hasREX], wide64);
			break;
		}
		case ADDRESSING::None:
			break;

		default: {
			// Implicit operand named by the table row (AL, EAX/AX/RAX, DX, 1, [ESI]...).
			// value64 differs from value32 only for XCHG eAX,r's accumulator (op0, reg 0);
			// pick it under REX.W (or a d64 default). 0x66 still wins for the 16-bit name.
			const bool wide64 = is64Bit && !op[i].value64.empty()
				&& (opcode.def64 == Instruction::OpcodeInfo::Default64::d64 || rexBits[w]);
			operands[i] = std::string(
				checks[hasOpsize] && !op[i].value16.empty() ? op[i].value16
				: (wide64 ? op[i].value64 : op[i].value32));
			break;
		}
		}
	}


	if (instructionStr == " ") {
		instructionStr = "(bad)"; return;
	}

	
	instructionStr += "\t";
	for (int i = 1; i < 3; ++i) {
		if (op[i].addressingMode == static_cast<uint8_t>(ADDRESSING::None)) continue;
		operands[0] += ", " + operands[i];
	}

	instructionStr += operands[0];

};

	inline std::string& decodeLineString()  {
		return instructionStr;
	}

	inline std::string& getMachineCode() {

		return machineCode;
	}

};