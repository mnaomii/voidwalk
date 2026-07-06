#include "../../../address-space/address_space.h"
#include "../../ELF/elf_disassembler.h"
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





// in IA-32 -> default operand size : 32bits, change with 66f

class IA_32: public Instruction{

public:
	
private:

	uint16_t prefix, opcode, scale, index, base, displacement, immediate;
	Operand op1, op2;

	std::string instructionStr;


public:

static constexpr std::array<OpInfo, 256> buildOpcodes() {

static const std::array<OpInfo, 256>& opcodeTable() {
	static constexpr std::array<OpInfo, 256> t = buildOpcodes();
	return t;
}

static std::string_view opcodeStrOf(uint8_t op)  { return opcodeTable()[op].text; }
static bool hasRMbyte(uint8_t op)                { return opcodeTable()[op].hasRMbyte; }
static bool hasImmediateByte(uint8_t op)         { return opcodeTable()[op].hasImmediateByte; }




static constexpr std::array<std::string_view, 256> buildPrefixes() {
	std::array<std::string_view, 256> n{};

#define P(name, text) n[static_cast<uint16_t>(Prefix::name)] = text

	P(LOCK,"LOCK"); P(REPNE,"REPNE"); P(REP,"REP");                          
	P(CS,"CS:"); P(SS,"SS:"); P(DS,"DS:"); P(ES,"ES:"); P(FS,"FS:"); P(GS,"GS:");  
	P(OPSIZE,"OPSIZE");                                                      
	P(ADDRSIZE,"ADDRSIZE");                                                  

#undef P
	return n;
}

static const std::array<std::string_view, 256>& prefixTable() {
	static constexpr std::array<std::string_view, 256> prefix_str = buildPrefixes();
	return prefix_str;
}

static std::string_view prefixStrOf(uint8_t op) { return prefixTable()[op]; }
static bool isPrefix(uint8_t op) { return !prefixTable()[op].empty(); }




std::string registerOf(uint16_t r) {
	std::string reg = "E";
	if (prefix == 0x66) reg = "";
	switch (r) {
		case static_cast<int>(REGISTER::AX):
			reg += "AX";
			break;
		case static_cast<int>(REGISTER::CX):
			reg += "CX";
			break;
		case static_cast<int>(REGISTER::DX):
			reg += "DX";
			break;
		case static_cast<int>(REGISTER::BX):
			reg += "BX";
			break;
		case static_cast<int>(REGISTER::SP):
			reg += "SP";
			break;
		case static_cast<int>(REGISTER::BP):
			reg += "BP";
			break;
		case static_cast<int>(REGISTER::SI):
			reg += "SI";
			break;
		case static_cast<int>(REGISTER::DI):
			reg += "DI";
			break;
		default: 
			throw std::runtime_error("Malformed expression detected..");

	}
	return reg;
}

IA_32(uint32_t prefix, uint32_t opcode, uint8_t rmbyte, uint8_t sib, uint32_t displacement, uint32_t immediate) {
	if (isPrefix(prefix)) instructionStr = prefixStrOf(prefix);
	instructionStr += " " + std::string(opcodeStrOf(opcode));
	if (hasRMbyte(static_cast<uint8_t>(opcode))) {

	}

	if (hasImmediateByte(opcode)) {
		op2.value = immediate; op2.state = STATE::IS_IMMEDIATE;  instructionStr += " " + std::format("{:#x}", immediate);
	}


};

	inline std::string decodeLineString()  {
		return instructionStr;
	}

};