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

#define registerOf(field1,field2) IA_32Mnemonic::registerOf(field1,field2)



// FORMAT:

// prefix : 0-3 bytes
// opcode : 1-3 bytes
// Mod R/M : 0-1 bytes : Mod + Reg/Opcode + R/M (little endian)
// SIB : 0-1 bytes : Scale + Index + Base (little endian)
// Displacement : 0-4 bytes
// Immediate : 0-4 bytes





// in IA-32 -> default operand size : 32bits, change with 66f

class IA_32: public Instruction{


public:
	
private:

	Instruction::Prefix prefix;
	uint32_t  opcode, opcode2 , scale, index, base, displacement, immediate;
	Instruction::Operand op1, op2, op3;

	std::string instructionStr;


public:

	// aliases
	using ADDRESSING = IA_32Mnemonic::ADDRESSING;
	using SIZE = IA_32Mnemonic::SIZE;
	using REGISTER = IA_32Mnemonic::REGISTER;
	using OPCODE = IA_32Mnemonic::OPCODE;
	using Prefix = IA_32Mnemonic::Prefix;

	static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable() { return IA_32Mnemonic::opcodeTable(); }
	static const std::array<std::string_view, 256>& prefixTable() { return IA_32Mnemonic::prefixTable(); }


static std::string_view opcodeStrOf(uint32_t op)  { return opcodeTable()[op].text; }
static bool hasRMbyte(uint32_t op)                { return opcodeTable()[op].hasRMByte; }
static bool hasImmediateByte(uint32_t op)         { return opcodeTable()[op].hasImmediateByte; }

// Group-aware row: the only one safe to read operands and immediates from. Needs ModRM.reg,
// so it can only be asked for after the ModRM byte has been read.
static Instruction::OpcodeInfo resolvedInfo(uint32_t op, uint8_t reg) { return IA_32Mnemonic::resolvedInfo(op, reg); }
static uint8_t op1AddressingMode(uint32_t op)	  { return opcodeTable()[op].op1am; }
static uint8_t op2AddressingMode(uint32_t op) { return opcodeTable()[op].op2am; }
static uint8_t op3AddressingMode(uint32_t op) { return opcodeTable()[op].op3am; }
static uint8_t op1Size(uint32_t op) { return opcodeTable()[op].op1s; }
static uint8_t op2Size(uint32_t op) { return opcodeTable()[op].op2s; }
static uint8_t op3Size(uint32_t op) { return opcodeTable()[op].op3s; }


static std::string_view prefixStrOf(uint8_t op) { if (isPrefix(op)) return prefixTable()[op];  else return ""; }
static bool isPrefix(uint8_t op) { return !prefixTable()[op].empty(); }


IA_32():prefix(0), opcode(0), scale(0), base(0), index(0), displacement(0), immediate(0), opcode2(0) {};

inline void decode(Instruction::Prefix pfx, uint64_t opc, uint64_t rmbyte, uint64_t sib, uint64_t disp, uint64_t imm) {


	uint8_t mod = 0, reg_op = 0, rm = 0;
	bool hasDisplacement = false;
	bool hasSIB = false;
	bool is16bit = false;

	op1 = op2 = op3 = Instruction::Operand{ "", UINT_MAX,
		static_cast<uint8_t>(ADDRESSING::None), static_cast<uint8_t>(SIZE::None) };

	prefix = pfx;
	bool hasPrefix = (pfx.byte[0] != 0) ? true : false;
	for (int i = 0; i < 4 && hasPrefix && !is16bit; i++)
		if (prefix.byte[i] == 0x66)
			is16bit = true;


	opcode = static_cast<uint32_t>(opc);

	

	const Instruction::OpcodeInfo& outer = opcodeTable()[opcode];



	const bool isGroupOpcode = IA_32Mnemonic::isGroup(opcode);

	if (outer.hasRMByte) {
		mod    = static_cast<uint8_t>((rmbyte & 0b11000000) >> 6);
		reg_op = static_cast<uint8_t>((rmbyte & 0b00111000) >> 3);
		rm     = static_cast<uint8_t>(rmbyte & 0b00000111);

		if (isGroupOpcode)
			opcode2 = reg_op;


		hasSIB = (mod != 0b11 && rm == static_cast<uint8_t>(REGISTER::SP));         // rm 100 = SIB byte follows
		hasDisplacement = (mod == 0b01 || mod == 0b10)                              // disp8 / disp32
		               || (mod == 0b00 && rm == 0b101 && !hasSIB);                  // 00/101 = disp32, no base
	}

	// ModRM.reg is known now, so the placeholder row a group opcode sits behind ("GRP5", no
	// operands, no immediate) can be swapped for the instruction it actually names. Every
	// operand field below reads from this, never from the 256-entry row.
	const Instruction::OpcodeInfo info = IA_32Mnemonic::resolvedInfo(opcode, reg_op);

	op1.addressingMode = info.op1am; op1.size = info.op1s;
	op2.addressingMode = info.op2am; op2.size = info.op2s;
	op3.addressingMode = info.op3am; op3.size = info.op3s;

	// The one memory operand, if any: [base + index*scale + disp]. Whichever operand asks
	// for E or M takes this text; with mod 11 there is no memory and E is a plain register.
	std::string memory;
	if (outer.hasRMByte && mod != 0b11) {

		if (hasSIB) {
			scale = 1u << ((sib & 0b11000000) >> 6);          // 00/01/10/11 -> *1/*2/*4/*8
			index = static_cast<uint32_t>((sib & 0b00111000) >> 3);
			base  = static_cast<uint32_t>(sib & 0b00000111);
		}
		if (hasDisplacement) displacement = static_cast<uint32_t>(disp);

		memory = "[";
		if (hasSIB) {
			memory += registerOf(base, is16bit);
			if (index != static_cast<uint32_t>(REGISTER::SP))                       // index 100 = no index
				memory += " + " + registerOf(index, is16bit) + "*" + std::to_string(static_cast<int>(scale));
		}
		else if (!(mod == 0b00 && rm == 0b101)) {                                    // that form has no base register
			memory += registerOf(rm, is16bit);
		}
		if (hasDisplacement) {
			// Nothing printed yet means there is no base register: the displacement is an
			// absolute address, not an offset, so it stays unsigned. [0x40303c]
			if (memory.size() == 1) {
				memory += std::format("{:#x}", displacement);
			}
			else {
				const int32_t d = static_cast<int32_t>(displacement);   // offset from a register: signed
				memory += (d < 0)
					? " - " + std::format("{:#x}", -static_cast<int64_t>(d))
					: " + " + std::format("{:#x}", d);
			}
		}
		memory += "]";
	}

	if (info.hasImmediateByte) immediate = static_cast<uint32_t>(imm);

	// Every operand is built from its addressing mode. The ones the opcode names in
	// silicon (AL, eAX, CL, DX, the constant 1, ...) come from the mode as well - unless
	// the mnemonic text already spells them out, which would print them twice.
	for (Instruction::Operand* op : { &op1, &op2, &op3 }) {

		switch (static_cast<ADDRESSING>(op->addressingMode)) {

		case ADDRESSING::E:
		case ADDRESSING::M:
			op->value = rm;
			op->text = (mod == 0b11) ? registerOf(rm, is16bit) : memory;
			break;

		case ADDRESSING::G:
			op->value = reg_op;
			op->text = registerOf(reg_op, is16bit);
			break;

		case ADDRESSING::S:
			op->value = reg_op;
			op->text = IA_32Mnemonic::segmentOf(reg_op);
			break;

		case ADDRESSING::I:      // immediate
		case ADDRESSING::J:      // relative offset
		case ADDRESSING::O:      // moffs
		case ADDRESSING::A:      // far pointer
			op->value = immediate;
			op->text = std::format("{:#x}", immediate);
			break;

		default:
			op->text = info.textNamesOperands
				? ""
				: IA_32Mnemonic::implicitOperandOf(op->addressingMode, is16bit);
			break;
		}
	}

	instructionStr.clear();
	if (isPrefix) {
		instructionStr += std::string(prefixStrOf(static_cast<uint8_t>(prefix.byte[0]))) + std::string(prefixStrOf(static_cast<uint8_t>(prefix.byte[1]))) + std::string(prefixStrOf(static_cast<uint8_t>(prefix.byte[2]))) + std::string(prefixStrOf(static_cast<uint8_t>(prefix.byte[3])));
		instructionStr += " ";
	}

	// Empty text = an illegal /reg for this group (FF /7, FE /2..7), or an opcode the table
	// never named at all.
	instructionStr += info.text.empty() ? "(bad)" : info.text;

	std::string operands;
	for (const Instruction::Operand* op : { &op1, &op2, &op3 }) {
		if (op->addressingMode == static_cast<uint8_t>(ADDRESSING::None) || op->text.empty()) continue;
		if (!operands.empty()) operands += ", ";
		operands += op->text;
	}
	// "ADD AL" + ", 0x5"   vs   "ADD" + " EBX, EAX"
	if (!operands.empty())
		instructionStr += (info.textNamesOperands ? ", " : " ") + operands;
};

	inline std::string decodeLineString()  {
		return instructionStr;
	}

};