#include "../../../address-space/address_space.h"
#include "../instruction.h"
#include "IA-32-mnemonic.h"
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

class IA_32: public Instruction{


public:

	// aliases
	using ADDRESSING = IA_32Mnemonic::ADDRESSING;
	using SIZE = IA_32Mnemonic::SIZE;
	using REGISTER = IA_32Mnemonic::REGISTER;
	using OPCODE = IA_32Mnemonic::OPCODE;
	using Prefix = IA_32Mnemonic::Prefix;

static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable()	{ return IA_32Mnemonic::opcodeTable(); }
static const std::array<std::string_view, 256>& prefixTable()			{ return IA_32Mnemonic::prefixTable(); }
static bool hasRMbyte(uint32_t op)										{ return opcodeTable()[op].hasRMByte; }
static Instruction::OpcodeInfo resolvedInfo(uint64_t op, uint8_t reg)	{ return IA_32Mnemonic::resolvedInfo(static_cast<uint32_t>(op), reg); }
static std::string_view opcodeStrOf(uint64_t op, uint8_t reg)			{ return resolvedInfo(op, reg).text; }
static std::string_view opcodeStr16Of(uint64_t op, uint8_t reg) { return resolvedInfo(op, reg).text16; }
static const std::array<Instruction::OpcodeInfo, 256>& twoByteTable()	{ return IA_32Mnemonic::twoByteTable(); }
static bool hasRMbyte2(uint32_t op2)									{ return twoByteTable()[op2].hasRMByte; }
static Instruction::OpcodeInfo twoByteResolvedInfo(uint64_t op2, uint8_t reg)	{ return IA_32Mnemonic::twoByteResolvedInfo(static_cast<uint32_t>(op2), reg); }
static std::string_view twoByteStrOf(uint64_t op2, uint8_t reg)			{ return twoByteResolvedInfo(op2, reg).text; }
static std::string_view twoByteStr16Of(uint64_t op2, uint8_t reg) { return twoByteResolvedInfo(op2, reg).text16; }

static std::string_view prefixStrOf(uint8_t op)							{ if (isPrefix(op)) return prefixTable()[op];  else return ""; }
static bool isPrefix(uint8_t op)										{ return !prefixTable()[op].empty(); }


IA_32() {};

inline void decode( uint64_t (&instructionBytes)[15], const bool (&checks)[8], const int& prefixEnd, const int& opcodeEnd, const int& immBegin, const uint32_t& immWidth, const uint32_t& dispWidth, const uint64_t (&rawImmediates)[3]) {
	
	uint8_t mod = 0, reg_op = 0, rm = 0, scale = 0, index = 0, base = 0;
	enum flags { 
		hasPrefix, hasModRM, hasSIB, hasDisp, hasImm, has2Byte, hasOpsize, hasAddrSize
	};

	std::string segment = "", fmt = "";
	for (int i = 0; i < prefixEnd; ++i) {

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

	Instruction::OpcodeInfo opcode;
	if (checks[hasModRM]) {

		mod = static_cast<uint8_t>((instructionBytes[opcodeEnd] & 0b11000000) >> 6);
		reg_op = static_cast<uint8_t>((instructionBytes[opcodeEnd] & 0b00111000) >> 3);
		rm = static_cast<uint8_t>(instructionBytes[opcodeEnd] & 0b00000111);

	}
	if (checks[has2Byte]) opcode = twoByteResolvedInfo(instructionBytes[opcodeEnd-1], reg_op);
	else opcode = resolvedInfo(instructionBytes[opcodeEnd-1], reg_op);
		

	machineCode += (checks[has2Byte]) ? "0f " : "";
	machineCode += std::format("{:02x} ", instructionBytes[opcodeEnd - 1]);
 	machineCode += (checks[hasModRM]) ? std::format("{:02x} ", instructionBytes[opcodeEnd]) : ""; // append the  ModRM byte

	// add the opcode to the decode string

	instructionStr += (checks[hasOpsize] && !opcode.text16.empty()) ? opcode.text16 : opcode.text;
	instructionStr += " ";




	// The one memory operand, if any: [base + index*scale + disp]. Whichever operand asks
	// for E or M takes this text; with mod 11 there is no memory and E is a plain register.

	fmt = "";
	std::string memory; uint64_t displacement = 0x0;
	if (checks[hasModRM] && mod != 0b11) {

		if (checks[hasSIB]) {
			machineCode += std::format("{:02x} ", instructionBytes[opcodeEnd + 1]);

			scale = 1u << ((instructionBytes[opcodeEnd + 1] & 0b11000000) >> 6);          // 00/01/10/11 -> *1/*2/*4/*8
			index = static_cast<uint32_t>((instructionBytes[opcodeEnd + 1] & 0b00111000) >> 3);
			base  = static_cast<uint32_t>(instructionBytes[opcodeEnd + 1] & 0b00000111);
		}
		if (checks[hasDisp]) {
			displacement = checks[hasSIB] ? instructionBytes[opcodeEnd + 2] : instructionBytes[opcodeEnd + 1];
			uint64_t aux = displacement;
			for (int k = 0; k < dispWidth; ++k)
				fmt += std::format("{:02x} ", (displacement >> (8 * k)) & 0xff);

			displacement = aux;

			machineCode += fmt;
		}
		
		if(hasAddrSize)
		memory += "[";
		if (checks[hasSIB]) {
			if(! (base == 5 && mod == 0))
			memory += IA_32Mnemonic::registerOf(base, checks[hasAddrSize]);
			if (index != static_cast<uint32_t>(REGISTER::SP))                       // index 100 = no index
				memory +=  " + " + IA_32Mnemonic::registerOf(index, checks[hasAddrSize]) + "*" + std::to_string(static_cast<int>(scale));
		}
		else if (!(mod == 0b00 && rm == 0b101)) {                                    // that form has no base register
			memory += IA_32Mnemonic::registerOf(rm, checks[hasAddrSize]);
		}
		if (checks[hasDisp]) {
			// Nothing printed yet means there is no base register: the displacement is an
			// absolute address, not an offset, so it stays unsigned.
			if (memory.size() == 1) {
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
			immediate[j] =  instructionBytes[immBegin + j];

			const uint64_t hexValue = (m == ADDRESSING::J) ? rawImmediates[j] : immediate[j];

			for(int k=0; k<immWidth; ++k)
				fmt+=std::format("{:02x} ", (hexValue >> (8 * k)) & 0xff);
			machineCode += fmt;
			++j;
		}
		++counter;
	}

	j = 0;
	// Every operand is built from its addressing mode. The ones the opcode names in
	// silicon (AL, eAX, CL, DX, the constant 1, ...) come from the mode as well - unless
	// the mnemonic text already spells them out, which would print them twice.


	for (int i = 0; i < 3; ++i) {

		if (op[i].value != "") {
			if (checks[hasOpsize])
				operands[i] = op[i].value16;
			else operands[i] = op[i].value;
				continue;
		};

		switch (static_cast<ADDRESSING>(op[i].addressingMode)) {

		case ADDRESSING::E:
		case ADDRESSING::M:		// memory
			operands[i] = (mod == 0b11) ? IA_32Mnemonic::registerOf(rm, op[i].size,checks[hasOpsize]) : (segment + memory);
			if (mod != 0b11) segment = "";
			break;

		case ADDRESSING::G:
			operands[i] = IA_32Mnemonic::registerOf(reg_op, op[i].size, checks[hasOpsize]);
			break;

		case ADDRESSING::S:		// segment
			operands[i] = IA_32Mnemonic::segmentOf(reg_op);
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

		case ADDRESSING::Z:
			operands[i] = IA_32Mnemonic::registerOf(instructionBytes[opcodeEnd -1] & 0x07, op[i].size, checks[hasOpsize]);

		case ADDRESSING::None:
			break;

		default:
			// Implicit operand named by the table row (AL, EAX/AX, DX, 1, [ESI]...).
			operands[i] = std::string(
				checks[hasOpsize] && !op[i].value16.empty() ? op[i].value16 : op[i].value);
			break;
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