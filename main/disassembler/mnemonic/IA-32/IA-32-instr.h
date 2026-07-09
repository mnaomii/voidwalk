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

#define  NULL_OPERAND(name)  Instruction::Operand name{"",UINT_MAX, static_cast<uint8_t>(ADDRESSING::None) ,static_cast<uint8_t>(SIZE::None)} 


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

	uint32_t prefix, opcode, scale, index, base, displacement, immediate;
	Instruction::Operand op1, op2, op3;

	std::string instructionStr;


public:

static constexpr std::array<Instruction::OpcodeInfo, 256> buildOpcodes() {

	std::array<OpcodeInfo, 256> n{};
#define P(name,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s, impliesOperand) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,hasRM, hasIMM, op1am,op2am,op3am, op1s,op2s,op3s, impliesOperand}
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)

	P(ADD_EbGb, "ADD", true, false, a(E), s(b), a(G), s(b), a(None), s(None),false);
	P(ADD_EvGv, "ADD", true, false, a(E), s(v), a(G), s(v), a(None), s(None),false);
	P(ADD_GbEb, "ADD", true, false, a(G), s(b), a(E), s(b), a(None), s(None),false);
	P(ADD_GvEv, "ADD", true, false, a(G), s(v), a(E), s(v), a(None), s(None),false);
	P(ADD_ALIb, "ADD AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None),true);
	P(ADD_eAXIv, "ADD eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None),true);

	P(PUSH_ES, "PUSH", false, false, a(ES), s(None), a(None), s(None), a(None), s(None),false);
	P(POP_ES, "POP", false, false, a(ES), s(None), a(None), s(None), a(None), s(None),false);

	P(OR_EbGb, "OR", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(OR_EvGv, "OR", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(OR_GbEb, "OR", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(OR_GvEv, "OR", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(OR_ALIb, "OR AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(OR_eAXIv, "OR eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(PUSH_CS, "PUSH", false, false, a(CS), s(None), a(None), s(None), a(None), s(None), false);
	P(TWOBYTE, "2BYTE", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(ADC_EbGb, "ADC", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(ADC_EvGv, "ADC", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(ADC_GbEb, "ADC", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(ADC_GvEv, "ADC", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(ADC_ALIb, "ADC AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(ADC_eAXIv, "ADC eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(PUSH_SS, "PUSH", false, false, a(SS), s(None), a(None), s(None), a(None), s(None), false);
	P(POP_SS, "POP", false, false, a(SS), s(None), a(None), s(None), a(None), s(None), false);

	P(SBB_EbGb, "SBB", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(SBB_EvGv, "SBB", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(SBB_GbEb, "SBB", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(SBB_GvEv, "SBB", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(SBB_ALIb, "SBB AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(SBB_eAXIv, "SBB eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(PUSH_DS, "PUSH", false, false, a(DS), s(None), a(None), s(None), a(None), s(None), false);
	P(POP_DS, "POP", false, false, a(DS), s(None), a(None), s(None), a(None), s(None), false);

	P(AND_EbGb, "AND", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(AND_EvGv, "AND", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(AND_GbEb, "AND", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(AND_GvEv, "AND", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(AND_ALIb, "AND AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(AND_eAXIv, "AND eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(ES, "ES", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(DAA, "DAA", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(SUB_EbGb, "SUB", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(SUB_EvGv, "SUB", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(SUB_GbEb, "SUB", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(SUB_GvEv, "SUB", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(SUB_ALIb, "SUB AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(SUB_eAXIv, "SUB eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(CS, "CS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(DAS, "DAS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(XOR_EbGb, "XOR", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(XOR_EvGv, "XOR", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(XOR_GbEb, "XOR", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(XOR_GvEv, "XOR", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(XOR_ALIb, "XOR AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(XOR_eAXIv, "XOR eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(SS, "SS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(AAA, "AAA", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(CMP_EbGb, "CMP", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(CMP_EvGv, "CMP", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(CMP_GbEb, "CMP", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(CMP_GvEv, "CMP", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(CMP_ALIb, "CMP AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(CMP_eAXIv, "CMP eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

	P(DS, "DS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(AAS, "AAS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(INC_eAX, "INC eAX", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eCX, "INC eCX", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eDX, "INC eDX", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eBX, "INC eBX", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eSP, "INC eSP", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eBP, "INC eBP", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eSI, "INC eSI", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None), true);
	P(INC_eDI, "INC eDI", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None), true);

	P(DEC_eAX, "DEC eAX", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eCX, "DEC eCX", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eDX, "DEC eDX", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eBX, "DEC eBX", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eSP, "DEC eSP", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eBP, "DEC eBP", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eSI, "DEC eSI", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None), true);
	P(DEC_eDI, "DEC eDI", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None), true);

	P(PUSH_eAX, "PUSH eAX", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eCX, "PUSH eCX", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eDX, "PUSH eDX", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eBX, "PUSH eBX", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eSP, "PUSH eSP", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eBP, "PUSH eBP", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eSI, "PUSH eSI", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None), true);
	P(PUSH_eDI, "PUSH eDI", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None), true);

	P(POP_eAX, "POP eAX", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eCX, "POP eCX", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eDX, "POP eDX", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eBX, "POP eBX", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None), false);
	P(POP_eSP, "POP eSP", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eBP, "POP eBP", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eSI, "POP eSI", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None), true);
	P(POP_eDI, "POP eDI", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None), true);

	P(PUSHA, "PUSHA", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(POPA, "POPA", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(BOUND_GvMa, "BOUND", true, false, a(G), s(v), a(M), s(a), a(None), s(None), false);
	P(ARPL_EwGw, "ARPL", true, false, a(E), s(w), a(G), s(w), a(None), s(None), false);
	P(FS, "FS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(GS, "GS", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(OPSIZE, "OPSIZE", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ADSIZE, "ADSIZE", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(PUSH_Iv, "PUSH", false, true, a(I), s(v), a(None), s(None), a(None), s(None), false);
	P(IMUL_GvEvIv, "IMUL", true, true, a(G), s(v), a(E), s(v), a(I), s(v), false);
	P(PUSH_Ib, "PUSH", false, true, a(I), s(b), a(None), s(None), a(None), s(None), false);
	P(IMUL_GvEvIb, "IMUL", true, true, a(G), s(v), a(E), s(v), a(I), s(b), false);

	P(INSB_YbDX, "INSB", false, false, a(Y), s(b), a(DX), s(None), a(None), s(None), false);
	P(INSW_YzDX, "INSW", false, false, a(Y), s(z), a(DX), s(None), a(None), s(None), false);
	P(OUTSB_DXXb, "OUTSB DX", false, false, a(DX), s(None), a(X), s(b), a(None), s(None), true);
	P(OUTSW_DXXv, "OUTSW DX", false, false, a(DX), s(None), a(X), s(v), a(None), s(None), true);

	P(JO, "JO", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNO, "JNO", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JB, "JB", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNB, "JNB", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JZ, "JZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNZ, "JNZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JBE, "JBE", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JA, "JA", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JS, "JS", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNS, "JNS", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JP, "JP", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNP, "JNP", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JL, "JL", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNL, "JNL", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JLE, "JLE", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JNLE, "JNLE", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);

	// Group 1 (0x80-0x83): "GRP1" is a placeholder; real mnemonic = grp1StrOf(ModRM.reg)
	P(GRP1_EbIb,  "GRP1", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false);
	P(GRP1_EvIz,  "GRP1", true, true, a(E), s(v), a(I), s(v), a(None), s(None), false);
	P(GRP1_EbIb2, "GRP1", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false);
	P(GRP1_EvIb,  "GRP1", true, true, a(E), s(v), a(I), s(b), a(None), s(None), false);

	P(TEST_EbGb, "TEST", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(TEST_EvGv, "TEST", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);

	P(XCHG_EbGb, "XCHG", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(XCHG_EvGv, "XCHG", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);

	P(MOV_EbGb, "MOV", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
	P(MOV_EvGv, "MOV", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
	P(MOV_GbEb, "MOV", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
	P(MOV_GvEv, "MOV", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
	P(MOV_EwSw, "MOV", true, false, a(E), s(w), a(S), s(w), a(None), s(None), false);

	P(LEA_GvM, "LEA", true, false, a(G), s(v), a(M), s(None), a(None), s(None), false);

	P(MOV_SwEw, "MOV", true, false, a(S), s(w), a(E), s(w), a(None), s(None), false);

	P(POP_Ev, "POP", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false);

	P(NOP, "NOP", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(XCHG_eAXeCX, "XCHG eAX, eCX", false, false, a(eAX), s(v), a(eCX), s(v), a(None), s(None), true);
	P(XCHG_eAXeDX, "XCHG eAX, eDX", false, false, a(eAX), s(v), a(eDX), s(v), a(None), s(None), true);
	P(XCHG_eAXeBX, "XCHG eAX, eBX", false, false, a(eAX), s(v), a(eBX), s(v), a(None), s(None), true);
	P(XCHG_eAXeSP, "XCHG eAX, eSP", false, false, a(eAX), s(v), a(eSP), s(v), a(None), s(None), true);
	P(XCHG_eAXeBP, "XCHG eAX, eBP", false, false, a(eAX), s(v), a(eBP), s(v), a(None), s(None), true);
	P(XCHG_eAXeSI, "XCHG eAX, eSI", false, false, a(eAX), s(v), a(eSI), s(v), a(None), s(None), true);
	P(XCHG_eAXeDI, "XCHG eAX, eDI", false, false, a(eAX), s(v), a(eDI), s(v), a(None), s(None), true);

	P(CBW, "CBW", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(CWD, "CWD", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(CALL_Ap, "CALL", false, true, a(A), s(p), a(None), s(None), a(None), s(None), false);
	P(FWAIT, "FWAIT", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(PUSHF_Fv, "PUSHF", false, false, a(F), s(v), a(None), s(None), a(None), s(None), false);
	P(POPF_Fv, "POPF", false, false, a(F), s(v), a(None), s(None), a(None), s(None), false);
	P(SAHF, "SAHF", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(LAHF, "LAHF", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(MOV_ALOb, "MOV AL", false, true, a(AL), s(b), a(O), s(b), a(None), s(None), true);
	P(MOV_eAXOv, "MOV eAX", false, true, a(eAX), s(v), a(O), s(v), a(None), s(None), true);
	P(MOV_ObAL, "MOV", false, true, a(O), s(b), a(AL), s(b), a(None), s(None), false);
	P(MOV_OveAX, "MOV", false, true, a(O), s(v), a(eAX), s(v), a(None), s(None), false);

	P(MOVSB_XbYb, "MOVSB", false, false, a(X), s(b), a(Y), s(b), a(None), s(None), false);
	P(MOVSW_XvYv, "MOVSW", false, false, a(X), s(v), a(Y), s(v), a(None), s(None), false);
	P(CMPSB_XbYb, "CMPSB", false, false, a(X), s(b), a(Y), s(b), a(None), s(None), false);
	P(CMPSW_XvYv, "CMPSW", false, false, a(X), s(v), a(Y), s(v), a(None), s(None), false);
	P(TEST_ALIb, "TEST", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), false);
	P(TEST_eAXIv, "TEST", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), false);
	P(STOSB_YbAL, "STOSB", false, false, a(Y), s(b), a(AL), s(b), a(None), s(None), false);
	P(STOSW_YveAX, "STOSW", false, false, a(Y), s(v), a(eAX), s(v), a(None), s(None), false);
	P(LODSB_ALXb, "LODSB", false, false, a(AL), s(b), a(X), s(b), a(None), s(None), false);
	P(LODSW_eAXXv, "LODSW", false, false, a(eAX), s(v), a(X), s(v), a(None), s(None), false);
	P(SCASB_ALYb, "SCASB", false, false, a(AL), s(b), a(Y), s(b), a(None), s(None), false);
	P(SCASW_eAXYv, "SCASW", false, false, a(eAX), s(v), a(Y), s(v), a(None), s(None), false);

	P(MOV_ALIb, "MOV AL,", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_CLIb, "MOV CL,", false, true, a(CL), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_DLIb, "MOV DL,", false, true, a(DL), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_BLIb, "MOV BL,", false, true, a(BL), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_AHIb, "MOV AH,", false, true, a(AH), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_CHIb, "MOV CH,", false, true, a(CH), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_DHIb, "MOV DH,", false, true, a(DH), s(b), a(I), s(b), a(None), s(None), true);
	P(MOV_BHIb, "MOV BH,", false, true, a(BH), s(b), a(I), s(b), a(None), s(None), true);

	P(MOV_eAXIv, "MOV eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eCXIv, "MOV eCX", false, true, a(eCX), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eDXIv, "MOV eDX", false, true, a(eDX), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eBXIv, "MOV eBX", false, true, a(eBX), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eSPIv, "MOV eSP", false, true, a(eSP), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eBPIv, "MOV eBP", false, true, a(eBP), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eSIIv, "MOV eSI", false, true, a(eSI), s(v), a(I), s(v), a(None), s(None), true);
	P(MOV_eDIIv, "MOV eDI", false, true, a(eDI), s(v), a(I), s(v), a(None), s(None), true);

	P(GRP2_EbIb, "GRP2", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false);
	P(GRP2_EvIb, "GRP2", true, true, a(E), s(v), a(I), s(b), a(None), s(None), false);
	P(RET_Iw, "RET", false, true, a(I), s(w), a(None), s(None), a(None), s(None), false);
	P(RET, "RET", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(LES_GvMp, "LES", true, false, a(G), s(v), a(M), s(p), a(None), s(None), false);
	P(LDS_GvMp, "LDS", true, false, a(G), s(v), a(M), s(p), a(None), s(None), false);
	P(MOV_EbIb, "MOV", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false);
	P(MOV_EvIv, "MOV", true, true, a(E), s(v), a(I), s(v), a(None), s(None), false);
	P(ENTER_IwIb, "ENTER", false, true, a(I), s(w), a(I), s(b), a(None), s(None), false);
	P(LEAVE, "LEAVE", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(RETF_Iw, "RETF", false, true, a(I), s(w), a(None), s(None), a(None), s(None), false);
	P(RETF, "RETF", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(INT3, "INT3", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(INT_Ib, "INT", false, true, a(I), s(b), a(None), s(None), a(None), s(None), false);
	P(INTO, "INTO", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(IRET, "IRET", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(GRP2_Eb1, "GRP2", true, false, a(E), s(b), a(One), s(None), a(None), s(None), false);
	P(GRP2_Ev1, "GRP2", true, false, a(E), s(v), a(One), s(None), a(None), s(None), false);
	P(GRP2_EbCL, "GRP2", true, false, a(E), s(b), a(CL), s(None), a(None), s(None), false);
	P(GRP2_EvCL, "GRP2", true, false, a(E), s(v), a(CL), s(None), a(None), s(None), false);

	P(AAM_Ib, "AAM", false, true, a(I), s(b), a(None), s(None), a(None), s(None), false);
	P(AAD_Ib, "AAD", false, true, a(I), s(b), a(None), s(None), a(None), s(None), false);
	P(SALC, "SALC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(XLAT, "XLAT", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(ESC0, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC1, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC2, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC3, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC4, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC5, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC6, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(ESC7, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(LOOPNZ_Jb, "LOOPNZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(LOOPZ_Jb, "LOOPZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(LOOP_Jb, "LOOP", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);
	P(JeCXZ_Jb, "JECXZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);

	P(IN_ALIb, "IN AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
	P(IN_eAXIb, "IN eAX", false, true, a(eAX), s(v), a(I), s(b), a(None), s(None), true);
	P(OUT_IbAL, "OUT", false, true, a(I), s(b), a(AL), s(b), a(None), s(None), false);
	P(OUT_IbeAX, "OUT", false, true, a(I), s(b), a(eAX), s(v), a(None), s(None), false);

	P(CALL_Jv, "CALL", false, true, a(J), s(v), a(None), s(None), a(None), s(None), false);
	P(JMP_Jv, "JMP", false, true, a(J), s(v), a(None), s(None), a(None), s(None), false);
	P(JMP_Ap, "JMP", false, true, a(A), s(p), a(None), s(None), a(None), s(None), false);
	P(JMP_Jb, "JMP", false, true, a(J), s(b), a(None), s(None), a(None), s(None), false);

	P(IN_ALDX, "IN AL, DX", false, false, a(AL), s(b), a(DX), s(None), a(None), s(None), true);
	P(IN_eAXDX, "IN eAX, DX", false, false, a(eAX), s(v), a(DX), s(None), a(None), s(None), true);
	P(OUT_DXAL, "OUT DX, AL", false, false, a(DX), s(None), a(AL), s(b), a(None), s(None), true);
	P(OUT_DXeAX, "OUT DX, eAX", false, false, a(DX), s(None), a(eAX), s(v), a(None), s(None), true);

	P(LOCK, "LOCK", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(INT1, "INT1", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(REPNE, "REPNE", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(REP, "REP", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(HLT, "HLT", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(CMC, "CMC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(GRP3_Eb, "GRP3", true, false, a(E), s(b), a(None), s(None), a(None), s(None), false);
	P(GRP3_Ev, "GRP3", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false);

	P(CLC, "CLC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(STC, "STC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(CLI, "CLI", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(STI, "STI", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(CLD, "CLD", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(STD, "STD", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

	P(GRP4, "GRP4", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
	P(GRP5, "GRP5", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false);



#undef s
#undef a
#undef P

	return n;
	
}

static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable() {
	static constexpr std::array<Instruction::OpcodeInfo, 256> t = buildOpcodes();
	return t;
}

static std::string_view opcodeStrOf(uint32_t op)  { return opcodeTable()[op].text; }
static bool hasRMbyte(uint32_t op)                { return opcodeTable()[op].hasRMByte; }
static bool hasImmediateByte(uint32_t op)         { return opcodeTable()[op].hasImmediateByte; }
static uint8_t op1AddressingMode(uint32_t op)	  { return opcodeTable()[op].op1am; }
static uint8_t op2AddressingMode(uint32_t op) { return opcodeTable()[op].op2am; }
static uint8_t op3AddressingMode(uint32_t op) { return opcodeTable()[op].op3am; }
static uint8_t op1Size(uint32_t op) { return opcodeTable()[op].op1s; }
static uint8_t op2Size(uint32_t op) { return opcodeTable()[op].op2s; }
static uint8_t op3Size(uint32_t op) { return opcodeTable()[op].op3s; }




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
			break;

	}
	return reg;
}

IA_32() {};

IA_32(uint32_t pfx, uint32_t opc, uint32_t rmbyte, uint32_t sib, uint32_t disp, uint32_t imm): prefix(0), opcode(0), scale(0), base(0), index(0), displacement(0), immediate(0) {

	NULL_OPERAND(op1); NULL_OPERAND(op2); NULL_OPERAND(op3);

	bool hasDisplacement = false;
	bool hasSIB = false;
	bool op1IsMemory = false;
	bool op2IsMemory = false;
	bool skipOp2 = false;




	if (isPrefix(prefix)) {
		prefix = pfx;
		instructionStr = prefixStrOf(prefix).data();
		instructionStr += " ";
	}

	opcode = opc;

	if (prefix == 0x67)
		opcode = ((prefix << 8) | opcode);
	instructionStr += opcodeStrOf(opcode).data();


	op1.size = op1Size(opcode);
	op1.addressingMode = op1AddressingMode(opcode);

	op2.size = op2Size(opcode);
	op2.addressingMode = op2AddressingMode(opcode);

	op3.size = op3Size(opcode);
	op3.addressingMode = op3AddressingMode(opcode);


	if (hasRMbyte(static_cast<uint8_t>(opcode))) {
		uint8_t mod = ((rmbyte & 0b11000000) >> 6);
		uint8_t reg_op = ((rmbyte & 0b00111000) >> 3);
		uint8_t rm = (rmbyte & 0b00000111);



		if (mod == 0b01 || mod == 0b10)
			hasDisplacement = true;

		if ((mod == 0 || hasDisplacement) && rm == static_cast<uint8_t>(REGISTER::SP))// rm == ESP == 0b100
			hasSIB = true;

		// operand 2 can be in reg_op : either register or opcode extension
		if (reg_op > 0x07) {
			// should be an opcode extension
			instructionStr += std::string(opcodeStrOf(reg_op)) + " ";
			skipOp2 = true;
		}




		if (op1.addressingMode == static_cast<uint8_t>(ADDRESSING::G) || op1.addressingMode == static_cast<uint8_t>(ADDRESSING::E)) {
			try {
				op1.text = registerOf(rm) ;
				if (op1.addressingMode == static_cast<uint8_t>(ADDRESSING::E)) {
					op1IsMemory = true;
					op1.text = "[ " + op1.text + " ] ";
				}
				//else op1.text += ", ";

				op1.value = rm;
			}
			catch (std::runtime_error& e) { throw; }
		}


		if (!skipOp2) { // is an operand

			if (op1IsMemory && op2.addressingMode == static_cast<uint8_t>(ADDRESSING::G) ||
				op2.addressingMode != static_cast<uint8_t>(ADDRESSING::E) && !op1IsMemory) {

				if (!op1IsMemory) op2IsMemory = true;

				try {
					op2.text = registerOf(reg_op);
					if (op2IsMemory)
						op2.text = "[" + op2.text + "] ";
					else op2.text += " ";
					op2.value = reg_op;
				}
				catch (std::runtime_error& e) { throw; }
			}

		}


	}

	if (hasImmediateByte(opcode)) {
		immediate = imm;
		if ( op1.addressingMode == static_cast<uint8_t>(ADDRESSING::I) || op1.addressingMode == static_cast<uint8_t>(ADDRESSING::J)) {
			op1.value = immediate;
			op1.addressingMode = op1AddressingMode(opcode);
			op1.size = op1Size(opcode);
			op1.text = std::format("{:#x}", immediate);

		}
		else if ( op2.addressingMode == static_cast<uint8_t>(ADDRESSING::I) || op2.addressingMode == static_cast<uint8_t>(ADDRESSING::J)) {
			op2.value = immediate;
			op2.addressingMode = op2AddressingMode(opcode);
			op2.size = op2Size(opcode);
			op2.text = std::format("{:#x}", immediate);

		}
		else {
			op3.value = immediate;
			op3.addressingMode = op3AddressingMode(opcode);
			op3.size = op3Size(opcode);
			op3.text = std::format("{:#x}", immediate);

		}
	}

	if (hasSIB) { // either op1 or both op1 & op2 were filled in during analysis of RMbyte
		scale = ((sib & 0b11000000) >> 6);
		switch (scale) {

		case 0b00:
			scale = 1;
			break;
		case 0b11:
			scale = 8;
			break;

		default:
			scale *= 2;
			break;

		}
		index = ((sib & 0b00111000) >> 3);
		base = (sib & 0b00000111);


		if (op1IsMemory) {
			try {
				op1.text = " [" + registerOf(base) + " + " + registerOf(index) + "*" + std::to_string(static_cast<int>(scale));
			}
			catch (std::runtime_error& e) { throw; }
		}
		else { // op2IsMemory = true, 3rd cannot be a memory address.
			op2.text = " [" + registerOf(base) + " + " + registerOf(index) + "*" + std::to_string(static_cast<int>(scale));

		}

		if (hasDisplacement) {
			displacement = disp;
			instructionStr += " + " + std::format("{:#x}", displacement);
		}
		instructionStr += "] ";

	}
	else {
		if (opcodeTable()[opcode].impliesOperands == true && op1AddressingMode(opcode) != static_cast<uint8_t>(ADDRESSING::None) && op1.text != "")
			instructionStr += ", ";
		else
			instructionStr += " ";
		if (op1.addressingMode != static_cast<uint8_t>(ADDRESSING::None))
			instructionStr += op1.text;
		if (op2.addressingMode != static_cast<uint8_t>(ADDRESSING::None))
			instructionStr += ", " + op2.text;
		if (op3.addressingMode != static_cast<uint8_t>(ADDRESSING::None))
			instructionStr += ", " + op3.text;
	}


};

	inline std::string decodeLineString()  {
		return instructionStr;
	}

};