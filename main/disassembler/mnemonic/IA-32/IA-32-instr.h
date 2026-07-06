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
	Operand op1, op2, op3;

	std::string instructionStr;


public:

static constexpr std::array<OpcodeInfo, 256> buildOpcodes() {

	std::array<OpcodeInfo, 256> n{};
#define P(name,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s) n[static_cast<uint16_t>(OPCODE::name)] = OpcodeInfo{text,hasRM, hasIMM, op1am,op2am,op3am, op1s,op2s,op3s}
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)

	P(ADD_EbGb, "ADD", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(ADD_EvGv, "ADD", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(ADD_GbEb, "ADD", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(ADD_GvEv, "ADD", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(ADD_ALIb, "ADD", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(ADD_eAXIv, "ADD", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(PUSH_ES, "PUSH", false, false, a(ES), s(None), a(None), s(None), a(None), s(None));
	P(POP_ES, "POP", false, false, a(ES), s(None), a(None), s(None), a(None), s(None));

	P(OR_EbGb, "OR", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(OR_EvGv, "OR", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(OR_GbEb, "OR", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(OR_GvEv, "OR", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(OR_ALIb, "OR", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(OR_eAXIv, "OR", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(PUSH_CS, "PUSH", false, false, a(CS), s(None), a(None), s(None), a(None), s(None));
	P(TWOBYTE, "2BYTE", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(ADC_EbGb, "ADC", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(ADC_EvGv, "ADC", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(ADC_GbEb, "ADC", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(ADC_GvEv, "ADC", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(ADC_ALIb, "ADC", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(ADC_eAXIv, "ADC", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(PUSH_SS, "PUSH", false, false, a(SS), s(None), a(None), s(None), a(None), s(None));
	P(POP_SS, "POP", false, false, a(SS), s(None), a(None), s(None), a(None), s(None));

	P(SBB_EbGb, "SBB", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(SBB_EvGv, "SBB", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(SBB_GbEb, "SBB", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(SBB_GvEv, "SBB", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(SBB_ALIb, "SBB", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(SBB_eAXIv, "SBB", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(PUSH_DS, "PUSH", false, false, a(DS), s(None), a(None), s(None), a(None), s(None));
	P(POP_DS, "POP", false, false, a(DS), s(None), a(None), s(None), a(None), s(None));

	P(AND_EbGb, "AND", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(AND_EvGv, "AND", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(AND_GbEb, "AND", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(AND_GvEv, "AND", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(AND_ALIb, "AND", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(AND_eAXIv, "AND", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(ES, "ES", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(DAA, "DAA", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(SUB_EbGb, "SUB", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(SUB_EvGv, "SUB", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(SUB_GbEb, "SUB", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(SUB_GvEv, "SUB", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(SUB_ALIb, "SUB", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(SUB_eAXIv, "SUB", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(CS, "CS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(DAS, "DAS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(XOR_EbGb, "XOR", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(XOR_EvGv, "XOR", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(XOR_GbEb, "XOR", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(XOR_GvEv, "XOR", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(XOR_ALIb, "XOR", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(XOR_eAXIv, "XOR", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(SS, "SS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(AAA, "AAA", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(CMP_EbGb, "CMP", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(CMP_EvGv, "CMP", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(CMP_GbEb, "CMP", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(CMP_GvEv, "CMP", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(CMP_ALIb, "CMP", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(CMP_eAXIv, "CMP", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));

	P(DS, "DS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(AAS, "AAS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(INC_eAX, "INC", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None));
	P(INC_eCX, "INC", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None));
	P(INC_eDX, "INC", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None));
	P(INC_eBX, "INC", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None));
	P(INC_eSP, "INC", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None));
	P(INC_eBP, "INC", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None));
	P(INC_eSI, "INC", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None));
	P(INC_eDI, "INC", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None));

	P(DEC_eAX, "DEC", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eCX, "DEC", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eDX, "DEC", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eBX, "DEC", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eSP, "DEC", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eBP, "DEC", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eSI, "DEC", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None));
	P(DEC_eDI, "DEC", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None));

	P(PUSH_eAX, "PUSH", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eCX, "PUSH", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eDX, "PUSH", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eBX, "PUSH", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eSP, "PUSH", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eBP, "PUSH", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eSI, "PUSH", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None));
	P(PUSH_eDI, "PUSH", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None));

	P(POP_eAX, "POP", false, false, a(eAX), s(v), a(None), s(None), a(None), s(None));
	P(POP_eCX, "POP", false, false, a(eCX), s(v), a(None), s(None), a(None), s(None));
	P(POP_eDX, "POP", false, false, a(eDX), s(v), a(None), s(None), a(None), s(None));
	P(POP_eBX, "POP", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None));
	P(POP_eSP, "POP", false, false, a(eSP), s(v), a(None), s(None), a(None), s(None));
	P(POP_eBP, "POP", false, false, a(eBP), s(v), a(None), s(None), a(None), s(None));
	P(POP_eSI, "POP", false, false, a(eSI), s(v), a(None), s(None), a(None), s(None));
	P(POP_eDI, "POP", false, false, a(eDI), s(v), a(None), s(None), a(None), s(None));

	P(PUSHA, "PUSHA", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(POPA, "POPA", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(BOUND_GvMa, "BOUND", true, false, a(G), s(v), a(M), s(a), a(None), s(None));
	P(ARPL_EwGw, "ARPL", true, false, a(E), s(w), a(G), s(w), a(None), s(None));
	P(FS, "FS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(GS, "GS", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(OPSIZE, "OPSIZE", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ADSIZE, "ADSIZE", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(PUSH_Iv, "PUSH", false, true, a(I), s(v), a(None), s(None), a(None), s(None));
	P(IMUL_GvEvIv, "IMUL", true, true, a(G), s(v), a(E), s(v), a(I), s(v));
	P(PUSH_Ib, "PUSH", false, true, a(I), s(b), a(None), s(None), a(None), s(None));
	P(IMUL_GvEvIb, "IMUL", true, true, a(G), s(v), a(E), s(v), a(I), s(b));

	P(INSB_YbDX, "INSB", false, false, a(Y), s(b), a(DX), s(None), a(None), s(None));
	P(INSW_YzDX, "INSW", false, false, a(Y), s(z), a(DX), s(None), a(None), s(None));
	P(OUTSB_DXXb, "OUTSB", false, false, a(DX), s(None), a(X), s(b), a(None), s(None));
	P(OUTSW_DXXv, "OUTSW", false, false, a(DX), s(None), a(X), s(v), a(None), s(None));

	P(JO, "JO", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNO, "JNO", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JB, "JB", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNB, "JNB", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JZ, "JZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNZ, "JNZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JBE, "JBE", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JA, "JA", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JS, "JS", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNS, "JNS", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JP, "JP", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNP, "JNP", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JL, "JL", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNL, "JNL", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JLE, "JLE", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JNLE, "JNLE", false, true, a(J), s(b), a(None), s(None), a(None), s(None));

	P(ADD_EbIb, "ADD", true, true, a(E), s(b), a(I), s(b), a(None), s(None));
	P(ADD_EvIv, "ADD", true, true, a(E), s(v), a(I), s(v), a(None), s(None));
	P(SUB_EbIb, "SUB", true, true, a(E), s(b), a(I), s(b), a(None), s(None));
	P(SUB_EvIb, "SUB", true, true, a(E), s(v), a(I), s(b), a(None), s(None));

	P(TEST_EbGb, "TEST", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(TEST_EvGv, "TEST", true, false, a(E), s(v), a(G), s(v), a(None), s(None));

	P(XCHG_EbGb, "XCHG", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(XCHG_EvGv, "XCHG", true, false, a(E), s(v), a(G), s(v), a(None), s(None));

	P(MOV_EbGb, "MOV", true, false, a(E), s(b), a(G), s(b), a(None), s(None));
	P(MOV_EvGv, "MOV", true, false, a(E), s(v), a(G), s(v), a(None), s(None));
	P(MOV_GbEb, "MOV", true, false, a(G), s(b), a(E), s(b), a(None), s(None));
	P(MOV_GvEv, "MOV", true, false, a(G), s(v), a(E), s(v), a(None), s(None));
	P(MOV_EwSw, "MOV", true, false, a(E), s(w), a(S), s(w), a(None), s(None));

	P(LEA_GvM, "LEA", true, false, a(G), s(v), a(M), s(None), a(None), s(None));

	P(MOV_SwEw, "MOV", true, false, a(S), s(w), a(E), s(w), a(None), s(None));

	P(POP_Ev, "POP", true, false, a(E), s(v), a(None), s(None), a(None), s(None));

	P(NOP, "NOP", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(XCHG_eAXeCX, "XCHG", false, false, a(eAX), s(v), a(eCX), s(v), a(None), s(None));
	P(XCHG_eAXeDX, "XCHG", false, false, a(eAX), s(v), a(eDX), s(v), a(None), s(None));
	P(XCHG_eAXeBX, "XCHG", false, false, a(eAX), s(v), a(eBX), s(v), a(None), s(None));
	P(XCHG_eAXeSP, "XCHG", false, false, a(eAX), s(v), a(eSP), s(v), a(None), s(None));
	P(XCHG_eAXeBP, "XCHG", false, false, a(eAX), s(v), a(eBP), s(v), a(None), s(None));
	P(XCHG_eAXeSI, "XCHG", false, false, a(eAX), s(v), a(eSI), s(v), a(None), s(None));
	P(XCHG_eAXeDI, "XCHG", false, false, a(eAX), s(v), a(eDI), s(v), a(None), s(None));

	P(CBW, "CBW", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(CWD, "CWD", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(CALL_Ap, "CALL", false, true, a(A), s(p), a(None), s(None), a(None), s(None));
	P(FWAIT, "FWAIT", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(PUSHF_Fv, "PUSHF", false, false, a(F), s(v), a(None), s(None), a(None), s(None));
	P(POPF_Fv, "POPF", false, false, a(F), s(v), a(None), s(None), a(None), s(None));
	P(SAHF, "SAHF", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(LAHF, "LAHF", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(MOV_ALOb, "MOV", false, true, a(AL), s(b), a(O), s(b), a(None), s(None));
	P(MOV_eAXOv, "MOV", false, true, a(eAX), s(v), a(O), s(v), a(None), s(None));
	P(MOV_ObAL, "MOV", false, true, a(O), s(b), a(AL), s(b), a(None), s(None));
	P(MOV_OveAX, "MOV", false, true, a(O), s(v), a(eAX), s(v), a(None), s(None));

	P(MOVSB_XbYb, "MOVSB", false, false, a(X), s(b), a(Y), s(b), a(None), s(None));
	P(MOVSW_XvYv, "MOVSW", false, false, a(X), s(v), a(Y), s(v), a(None), s(None));
	P(CMPSB_XbYb, "CMPSB", false, false, a(X), s(b), a(Y), s(b), a(None), s(None));
	P(CMPSW_XvYv, "CMPSW", false, false, a(X), s(v), a(Y), s(v), a(None), s(None));
	P(TEST_ALIb, "TEST", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(TEST_eAXIv, "TEST", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));
	P(STOSB_YbAL, "STOSB", false, false, a(Y), s(b), a(AL), s(b), a(None), s(None));
	P(STOSW_YveAX, "STOSW", false, false, a(Y), s(v), a(eAX), s(v), a(None), s(None));
	P(LODSB_ALXb, "LODSB", false, false, a(AL), s(b), a(X), s(b), a(None), s(None));
	P(LODSW_eAXXv, "LODSW", false, false, a(eAX), s(v), a(X), s(v), a(None), s(None));
	P(SCASB_ALYb, "SCASB", false, false, a(AL), s(b), a(Y), s(b), a(None), s(None));
	P(SCASW_eAXYv, "SCASW", false, false, a(eAX), s(v), a(Y), s(v), a(None), s(None));

	P(MOV_ALIb, "MOV", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(MOV_CLIb, "MOV", false, true, a(CL), s(b), a(I), s(b), a(None), s(None));
	P(MOV_DLIb, "MOV", false, true, a(DL), s(b), a(I), s(b), a(None), s(None));
	P(MOV_BLIb, "MOV", false, true, a(BL), s(b), a(I), s(b), a(None), s(None));
	P(MOV_AHIb, "MOV", false, true, a(AH), s(b), a(I), s(b), a(None), s(None));
	P(MOV_CHIb, "MOV", false, true, a(CH), s(b), a(I), s(b), a(None), s(None));
	P(MOV_DHIb, "MOV", false, true, a(DH), s(b), a(I), s(b), a(None), s(None));
	P(MOV_BHIb, "MOV", false, true, a(BH), s(b), a(I), s(b), a(None), s(None));

	P(MOV_eAXIv, "MOV", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eCXIv, "MOV", false, true, a(eCX), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eDXIv, "MOV", false, true, a(eDX), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eBXIv, "MOV", false, true, a(eBX), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eSPIv, "MOV", false, true, a(eSP), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eBPIv, "MOV", false, true, a(eBP), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eSIIv, "MOV", false, true, a(eSI), s(v), a(I), s(v), a(None), s(None));
	P(MOV_eDIIv, "MOV", false, true, a(eDI), s(v), a(I), s(v), a(None), s(None));

	P(GRP2_EbIb, "GRP2", true, true, a(E), s(b), a(I), s(b), a(None), s(None));
	P(GRP2_EvIb, "GRP2", true, true, a(E), s(v), a(I), s(b), a(None), s(None));
	P(RET_Iw, "RET", false, true, a(I), s(w), a(None), s(None), a(None), s(None));
	P(RET, "RET", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(LES_GvMp, "LES", true, false, a(G), s(v), a(M), s(p), a(None), s(None));
	P(LDS_GvMp, "LDS", true, false, a(G), s(v), a(M), s(p), a(None), s(None));
	P(MOV_EbIb, "MOV", true, true, a(E), s(b), a(I), s(b), a(None), s(None));
	P(MOV_EvIv, "MOV", true, true, a(E), s(v), a(I), s(v), a(None), s(None));
	P(ENTER_IwIb, "ENTER", false, true, a(I), s(w), a(I), s(b), a(None), s(None));
	P(LEAVE, "LEAVE", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(RETF_Iw, "RETF", false, true, a(I), s(w), a(None), s(None), a(None), s(None));
	P(RETF, "RETF", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(INT3, "INT3", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(INT_Ib, "INT", false, true, a(I), s(b), a(None), s(None), a(None), s(None));
	P(INTO, "INTO", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(IRET, "IRET", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(GRP2_Eb1, "GRP2", true, false, a(E), s(b), a(One), s(None), a(None), s(None));
	P(GRP2_Ev1, "GRP2", true, false, a(E), s(v), a(One), s(None), a(None), s(None));
	P(GRP2_EbCL, "GRP2", true, false, a(E), s(b), a(CL), s(None), a(None), s(None));
	P(GRP2_EvCL, "GRP2", true, false, a(E), s(v), a(CL), s(None), a(None), s(None));

	P(AAM_Ib, "AAM", false, true, a(I), s(b), a(None), s(None), a(None), s(None));
	P(AAD_Ib, "AAD", false, true, a(I), s(b), a(None), s(None), a(None), s(None));
	P(SALC, "SALC", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(XLAT, "XLAT", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(ESC0, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC1, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC2, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC3, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC4, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC5, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC6, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(ESC7, "ESC", true, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(LOOPNZ_Jb, "LOOPNZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(LOOPZ_Jb, "LOOPZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(LOOP_Jb, "LOOP", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(JCXZ_Jb, "JCXZ", false, true, a(J), s(b), a(None), s(None), a(None), s(None));
	P(IN_ALIb, "IN", false, true, a(AL), s(b), a(I), s(b), a(None), s(None));
	P(IN_eAXIb, "IN", false, true, a(eAX), s(v), a(I), s(b), a(None), s(None));
	P(OUT_IbAL, "OUT", false, true, a(I), s(b), a(AL), s(b), a(None), s(None));
	P(OUT_IbeAX, "OUT", false, true, a(I), s(b), a(eAX), s(v), a(None), s(None));

	P(CALL_Jv, "CALL", false, true, a(J), s(v), a(None), s(None), a(None), s(None));
	P(JMP_Jv, "JMP", false, true, a(J), s(v), a(None), s(None), a(None), s(None));
	P(JMP_Ap, "JMP", false, true, a(A), s(p), a(None), s(None), a(None), s(None));
	P(JMP_Jb, "JMP", false, true, a(J), s(b), a(None), s(None), a(None), s(None));

	P(IN_ALDX, "IN", false, false, a(AL), s(b), a(DX), s(None), a(None), s(None));
	P(IN_eAXDX, "IN", false, false, a(eAX), s(v), a(DX), s(None), a(None), s(None));
	P(OUT_DXAL, "OUT", false, false, a(DX), s(None), a(AL), s(b), a(None), s(None));
	P(OUT_DXeAX, "OUT", false, false, a(DX), s(None), a(eAX), s(v), a(None), s(None));

	P(LOCK, "LOCK", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(INT1, "INT1", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(REPNE, "REPNE", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(REP, "REP", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(HLT, "HLT", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(CMC, "CMC", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(GRP3_Eb, "GRP3", true, false, a(E), s(b), a(None), s(None), a(None), s(None));
	P(GRP3_Ev, "GRP3", true, false, a(E), s(v), a(None), s(None), a(None), s(None));

	P(CLC, "CLC", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(STC, "STC", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(CLI, "CLI", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(STI, "STI", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(CLD, "CLD", false, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(STD, "STD", false, false, a(None), s(None), a(None), s(None), a(None), s(None));

	P(GRP4, "GRP4", true, false, a(None), s(None), a(None), s(None), a(None), s(None));
	P(GRP5, "GRP5", true, false, a(None), s(None), a(None), s(None), a(None), s(None));



#undef s
#undef a
#undef P

	return n;
	
}

static const std::array<OpcodeInfo, 256>& opcodeTable() {
	static constexpr std::array<OpcodeInfo, 256> t = buildOpcodes();
	return t;
}

static std::string_view opcodeStrOf(uint8_t op)  { return opcodeTable()[op].text; }
static bool hasRMbyte(uint8_t op)                { return opcodeTable()[op].hasRMByte; }
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
			break;

	}
	return reg;
}

IA_32(uint32_t prefix, uint32_t opcode, uint8_t rmbyte, uint8_t sib, uint32_t displacement, uint32_t immediate) {

	if (isPrefix(prefix)) {
		instructionStr = prefixStrOf(prefix).data();
		instructionStr += " ";
	}

	instructionStr += opcodeStrOf(opcode).data();

	if (hasRMbyte(static_cast<uint8_t>(opcode))) {
		uint8_t mod = ((rmbyte & 0b11000000) >> 6)
		uint8_t reg = ((rmbyte & 0b00111000)) >> 3
		uint8_t rm = (rmbyte & 0b00000111);
	}

	if (hasImmediateByte(opcode)) {
		if (op2.value == 0) op2.value = immediate; else op3.value = immediate;
		instructionStr += " ";
		instructionStr += std::format("{:#x}", immediate);
	}


};

	inline std::string decodeLineString()  {
		return instructionStr;
	}

};