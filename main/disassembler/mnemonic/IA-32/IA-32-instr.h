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

	uint32_t prefix, opcode, opcode2 , scale, index, base, displacement, immediate;
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
static uint8_t op1AddressingMode(uint32_t op)	  { return opcodeTable()[op].op1am; }
static uint8_t op2AddressingMode(uint32_t op) { return opcodeTable()[op].op2am; }
static uint8_t op3AddressingMode(uint32_t op) { return opcodeTable()[op].op3am; }
static uint8_t op1Size(uint32_t op) { return opcodeTable()[op].op1s; }
static uint8_t op2Size(uint32_t op) { return opcodeTable()[op].op2s; }
static uint8_t op3Size(uint32_t op) { return opcodeTable()[op].op3s; }


static std::string_view prefixStrOf(uint8_t op) { return prefixTable()[op]; }
static bool isPrefix(uint8_t op) { return !prefixTable()[op].empty(); }


IA_32():prefix(0), opcode(0), scale(0), base(0), index(0), displacement(0), immediate(0), opcode2(0) {};

inline void decode(uint64_t pfx, uint64_t opc, uint64_t rmbyte, uint64_t sib, uint64_t disp, uint64_t imm) {

	op1 = op2 = op3 = Instruction::Operand{ "", UINT_MAX,
		static_cast<uint8_t>(ADDRESSING::None), static_cast<uint8_t>(SIZE::None) };

	prefix = isPrefix(static_cast<uint8_t>(pfx)) ? static_cast<uint32_t>(pfx) : 0;
	opcode = static_cast<uint32_t>(opc);

	

	const Instruction::OpcodeInfo& info = opcodeTable()[opcode];

	op1.addressingMode = info.op1am; op1.size = info.op1s;
	op2.addressingMode = info.op2am; op2.size = info.op2s;
	op3.addressingMode = info.op3am; op3.size = info.op3s;

	uint8_t mod = 0, reg_op = 0, rm = 0;
	bool hasDisplacement = false;
	bool hasSIB = false;
	// isGroup(), not "groupNoOf() != -1": opcodes the table never filled in are value
	// initialised, so their groupNo is 0 - which is not -1, and would send every undefined
	// byte down the group path.
	const bool isGroupOpcode = IA_32Mnemonic::isGroup(opcode);

	if (info.hasRMByte) {
		mod    = static_cast<uint8_t>((rmbyte & 0b11000000) >> 6);
		reg_op = static_cast<uint8_t>((rmbyte & 0b00111000) >> 3);
		rm     = static_cast<uint8_t>(rmbyte & 0b00000111);

		if (isGroupOpcode)
			opcode2 = reg_op;


		hasSIB = (mod != 0b11 && rm == static_cast<uint8_t>(REGISTER::SP));         // rm 100 = SIB byte follows
		hasDisplacement = (mod == 0b01 || mod == 0b10)                              // disp8 / disp32
		               || (mod == 0b00 && rm == 0b101 && !hasSIB);                  // 00/101 = disp32, no base
	}

	// The one memory operand, if any: [base + index*scale + disp]. Whichever operand asks
	// for E or M takes this text; with mod 11 there is no memory and E is a plain register.
	std::string memory;
	if (info.hasRMByte && mod != 0b11) {

		if (hasSIB) {
			scale = 1u << ((sib & 0b11000000) >> 6);          // 00/01/10/11 -> *1/*2/*4/*8
			index = static_cast<uint32_t>((sib & 0b00111000) >> 3);
			base  = static_cast<uint32_t>(sib & 0b00000111);
		}
		if (hasDisplacement) displacement = static_cast<uint32_t>(disp);

		memory = "[";
		if (hasSIB) {
			memory += registerOf(base, prefix);
			if (index != static_cast<uint32_t>(REGISTER::SP))                       // index 100 = no index
				memory += " + " + registerOf(index, prefix) + "*" + std::to_string(static_cast<int>(scale));
		}
		else if (!(mod == 0b00 && rm == 0b101)) {                                    // that form has no base register
			memory += registerOf(rm, prefix);
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
			op->text = (mod == 0b11) ? registerOf(rm, prefix) : memory;
			break;

		case ADDRESSING::G:
			op->value = reg_op;
			op->text = registerOf(reg_op, prefix);
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
				: IA_32Mnemonic::implicitOperandOf(op->addressingMode, prefix);
			break;
		}
	}

	instructionStr.clear();
	if (prefix) {
		instructionStr += prefixStrOf(static_cast<uint8_t>(prefix));
		instructionStr += " ";
	}

	if (isGroupOpcode) {
		const std::string_view mnemonic = IA_32Mnemonic::groupEntryOf(opcode, reg_op).text;
		instructionStr += mnemonic.empty() ? "(bad)" : mnemonic;   // illegal /reg for this group
	}
	else instructionStr += opcodeStrOf(opcode);

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