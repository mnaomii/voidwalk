#include "../instruction.h"
#include <cstdint>
#include <array>
#include <string_view>
#include <stdexcept>
#pragma once


class IA_32Mnemonic{
public:

	static std::string registerOf(uint16_t r, bool is16bit) {
		std::string reg = "E";
		if (is16bit) reg = "";
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


	// ModRM.reg as a segment register (MOV Ew,Sw / MOV Sw,Ew). reg 6 and 7 name no
	// segment register, so the encoding is invalid - but a linear sweep of .text runs
	// over data and over opcodes we cannot decode yet, so invalid encodings are normal
	// and must not kill the run. Mark them the way objdump does instead of throwing.
	static std::string segmentOf(uint16_t r) {
		constexpr std::string_view names[8] = { "ES", "CS", "SS", "DS", "FS", "GS", "(bad)", "(bad)" };
		return std::string(names[r & 0x07]);
	}

	// Operands the opcode names in silicon rather than in its bytes: AL, eAX, CL, DX,
	// the constant 1, a segment register, the string source/destination. Returns ""
	// for addressing modes that encode their operand in the instruction (E, G, I, ...),
	// which the decoder resolves itself.
	static std::string implicitOperandOf(uint8_t am, bool is16bit) {
		const std::string e = (is16bit) ? "" : "E";

		switch (static_cast<ADDRESSING>(am)) {
		case ADDRESSING::AL:  return "AL";
		case ADDRESSING::CL:  return "CL";
		case ADDRESSING::DL:  return "DL";
		case ADDRESSING::BL:  return "BL";
		case ADDRESSING::AH:  return "AH";
		case ADDRESSING::CH:  return "CH";
		case ADDRESSING::DH:  return "DH";
		case ADDRESSING::BH:  return "BH";
		case ADDRESSING::DX:  return "DX";
		case ADDRESSING::One: return "1";

		case ADDRESSING::eAX: return e + "AX";
		case ADDRESSING::eCX: return e + "CX";
		case ADDRESSING::eDX: return e + "DX";
		case ADDRESSING::eBX: return e + "BX";
		case ADDRESSING::eSP: return e + "SP";
		case ADDRESSING::eBP: return e + "BP";
		case ADDRESSING::eSI: return e + "SI";
		case ADDRESSING::eDI: return e + "DI";

		case ADDRESSING::ES:  return "ES";
		case ADDRESSING::CS:  return "CS";
		case ADDRESSING::SS:  return "SS";
		case ADDRESSING::DS:  return "DS";

		case ADDRESSING::X:   return "[" + e + "SI]";   // string source, DS:eSI
		case ADDRESSING::Y:   return "[" + e + "DI]";   // string destination, ES:eDI
		case ADDRESSING::F:   return "";                // EFLAGS: PUSHF/POPF show no operand

		default: return "";
		}
	}


	static constexpr std::array<std::string_view, 256> buildPrefixes() {
		std::array<std::string_view, 256> n{};

#define P(name, text) n[static_cast<uint16_t>(Prefix::name)] = text

		P(LOCK, "LOCK"); P(REPNE, "REPNE"); P(REP, "REP");
		P(CS, "CS:"); P(SS, "SS:"); P(DS, "DS:"); P(ES, "ES:"); P(FS, "FS:"); P(GS, "GS:");
		P(OPSIZE, "OPSIZE");
		P(ADDRSIZE, "ADDRSIZE");

#undef P
		return n;
	}



	static const std::array<std::string_view, 256>& prefixTable() {
		static constexpr std::array<std::string_view, 256> prefix_str = buildPrefixes();
		return prefix_str;
	}

	// groupNo is the Intel opcode-extension group (Table A-6). -1 = no group:
	// the opcode byte alone names the instruction. >= 0 = the mnemonic lives in
	// ModRM.reg and OpcodeInfo::text ("GRP1", "GRP2", ...) is only a placeholder.
	// Unfilled table slots (undefined opcodes) get groupNo 0 from value-init, not
	// -1, so check the group only on opcodes the table actually names.
	// 0x8F (group 1A), 0xC6/0xC7 (group 11) are groups on paper, but only /0 is
	// legal, so they stay -1 and keep their real mnemonic (POP / MOV).
	static constexpr std::array<Instruction::OpcodeInfo, 256> buildOpcodes() {

		std::array<Instruction::OpcodeInfo, 256> n{};
		// PG: member of an opcode-extension group (group number last). P: everything else, no group.
#define PG(name,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s, textNamesOperands, group) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,hasRM, hasIMM, op1am,op2am,op3am, op1s,op2s,op3s, textNamesOperands, group}
#define P(name,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s, textNamesOperands) PG(name,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s, textNamesOperands, -1)
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)

		P(ADD_EbGb, "ADD", true, false, a(E), s(b), a(G), s(b), a(None), s(None), false);
		P(ADD_EvGv, "ADD", true, false, a(E), s(v), a(G), s(v), a(None), s(None), false);
		P(ADD_GbEb, "ADD", true, false, a(G), s(b), a(E), s(b), a(None), s(None), false);
		P(ADD_GvEv, "ADD", true, false, a(G), s(v), a(E), s(v), a(None), s(None), false);
		P(ADD_ALIb, "ADD AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
		P(ADD_eAXIv, "ADD eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);

		P(PUSH_ES, "PUSH", false, false, a(ES), s(None), a(None), s(None), a(None), s(None), false);
		P(POP_ES, "POP", false, false, a(ES), s(None), a(None), s(None), a(None), s(None), false);

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
		P(POP_eBX, "POP eBX", false, false, a(eBX), s(v), a(None), s(None), a(None), s(None), true);
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

		// Group 1 (0x80-0x83): ADD/OR/ADC/SBB/AND/SUB/XOR/CMP, selected by ModRM.reg
		PG(GRP1_EbIb, "GRP1", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false, 1);
		PG(GRP1_EvIz, "GRP1", true, true, a(E), s(v), a(I), s(v), a(None), s(None), false, 1);
		PG(GRP1_EbIb2, "GRP1", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false, 1);
		PG(GRP1_EvIb, "GRP1", true, true, a(E), s(v), a(I), s(b), a(None), s(None), false, 1);

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

		P(MOV_ALIb, "MOV AL", false, true, a(AL), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_CLIb, "MOV CL", false, true, a(CL), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_DLIb, "MOV DL", false, true, a(DL), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_BLIb, "MOV BL", false, true, a(BL), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_AHIb, "MOV AH", false, true, a(AH), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_CHIb, "MOV CH", false, true, a(CH), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_DHIb, "MOV DH", false, true, a(DH), s(b), a(I), s(b), a(None), s(None), true);
		P(MOV_BHIb, "MOV BH", false, true, a(BH), s(b), a(I), s(b), a(None), s(None), true);

		P(MOV_eAXIv, "MOV eAX", false, true, a(eAX), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eCXIv, "MOV eCX", false, true, a(eCX), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eDXIv, "MOV eDX", false, true, a(eDX), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eBXIv, "MOV eBX", false, true, a(eBX), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eSPIv, "MOV eSP", false, true, a(eSP), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eBPIv, "MOV eBP", false, true, a(eBP), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eSIIv, "MOV eSI", false, true, a(eSI), s(v), a(I), s(v), a(None), s(None), true);
		P(MOV_eDIIv, "MOV eDI", false, true, a(eDI), s(v), a(I), s(v), a(None), s(None), true);

		// Group 2 (0xC0/0xC1, 0xD0-0xD3): ROL/ROR/RCL/RCR/SHL/SHR/SAL/SAR, selected by ModRM.reg
		PG(GRP2_EbIb, "GRP2", true, true, a(E), s(b), a(I), s(b), a(None), s(None), false, 2);
		PG(GRP2_EvIb, "GRP2", true, true, a(E), s(v), a(I), s(b), a(None), s(None), false, 2);
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

		PG(GRP2_Eb1, "GRP2", true, false, a(E), s(b), a(One), s(None), a(None), s(None), false, 2);
		PG(GRP2_Ev1, "GRP2", true, false, a(E), s(v), a(One), s(None), a(None), s(None), false, 2);
		PG(GRP2_EbCL, "GRP2", true, false, a(E), s(b), a(CL), s(None), a(None), s(None), false, 2);
		PG(GRP2_EvCL, "GRP2", true, false, a(E), s(v), a(CL), s(None), a(None), s(None), false, 2);

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

		// Group 3 (0xF6/0xF7): TEST/NOT/NEG/MUL/IMUL/DIV/IDIV, selected by ModRM.reg
		PG(GRP3_Eb, "GRP3", true, false, a(E), s(b), a(None), s(None), a(None), s(None), false, 3);
		PG(GRP3_Ev, "GRP3", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 3);

		P(CLC, "CLC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
		P(STC, "STC", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
		P(CLI, "CLI", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
		P(STI, "STI", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
		P(CLD, "CLD", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);
		P(STD, "STD", false, false, a(None), s(None), a(None), s(None), a(None), s(None), false);

		// Group 4 (0xFE): INC/DEC Eb.  Group 5 (0xFF): INC/DEC/CALL/JMP/PUSH Ev.
		PG(GRP4, "GRP4", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false, 4);
		PG(GRP5, "GRP5", true, false, a(None), s(None), a(None), s(None), a(None), s(None), false, 5);



#undef s
#undef a
#undef P
#undef PG

		return n;

	}


	static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable() {
		static constexpr std::array<Instruction::OpcodeInfo, 256> t = buildOpcodes();
		return t;
	}

	// > 0 when the opcode is a group: the mnemonic must still be resolved from ModRM.reg.
	static int groupNoOf(uint32_t op) { return opcodeTable()[op].groupNo; }
	static bool isGroup(uint32_t op) { return opcodeTable()[op].groupNo > 0; }



#define G(reg,text, hasRM, hasIMM, op1am, op1s, op2am, op2s, op3am, op3s, textNamesOperands, group) n[reg] = Instruction::OpcodeInfo{text,hasRM, hasIMM, op1am,op2am,op3am, op1s,op2s,op3s, textNamesOperands, group}
#define GT(reg,text,group) G(reg, text, true, false, a(None), s(None), a(None), s(None), a(None), s(None), false, group)
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)

	// 0x80-0x83
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup1() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		GT(0, "ADD", 1);
		GT(1, "OR", 1);
		GT(2, "ADC", 1);
		GT(3, "SBB", 1);
		GT(4, "AND", 1);
		GT(5, "SUB", 1);
		GT(6, "XOR", 1);
		GT(7, "CMP", 1);
		return n;
	}

	// 0xC0/0xC1 (by Ib), 0xD0-0xD3 (by 1 / by CL). /6 SAL is an undocumented alias of /4 SHL.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup2() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		GT(0, "ROL", 2);
		GT(1, "ROR", 2);
		GT(2, "RCL", 2);
		GT(3, "RCR", 2);
		GT(4, "SHL", 2);
		GT(5, "SHR", 2);
		GT(6, "SAL", 2);
		GT(7, "SAR", 2);
		return n;
	}

	// 0xF6 (Eb) / 0xF7 (Ev). Only /0 and /1 take an immediate; /1 is an undocumented
	// alias of /0. /4../7 use AL/eAX implicitly - that is not encoded here.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup3() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "TEST", true, true, a(E), s(None), a(I), s(None), a(None), s(None), false, 3);
		G(1, "TEST", true, true, a(E), s(None), a(I), s(None), a(None), s(None), false, 3);
		G(2, "NOT", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		G(3, "NEG", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		G(4, "MUL", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		G(5, "IMUL", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		G(6, "DIV", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		G(7, "IDIV", true, false, a(E), s(None), a(None), s(None), a(None), s(None), false, 3);
		return n;
	}

	// 0xFE. /2../7 are illegal.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup4() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "INC", true, false, a(E), s(b), a(None), s(None), a(None), s(None), false, 4);
		G(1, "DEC", true, false, a(E), s(b), a(None), s(None), a(None), s(None), false, 4);
		return n;
	}

	// 0xFF. /3 and /5 are the far forms (m16:32); /7 is illegal.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup5() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "INC", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 5);
		G(1, "DEC", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 5);
		G(2, "CALL", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 5);
		G(3, "CALLF", true, false, a(M), s(p), a(None), s(None), a(None), s(None), false, 5);
		G(4, "JMP", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 5);
		G(5, "JMPF", true, false, a(M), s(p), a(None), s(None), a(None), s(None), false, 5);
		G(6, "PUSH", true, false, a(E), s(v), a(None), s(None), a(None), s(None), false, 5);
		return n;
	}

#undef s
#undef a
#undef GT
#undef G

	static const std::array<Instruction::OpcodeInfo, 8>& grp1Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildGroup1();
		return t;
	}
	static const std::array<Instruction::OpcodeInfo, 8>& grp2Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildGroup2();
		return t;
	}
	static const std::array<Instruction::OpcodeInfo, 8>& grp3Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildGroup3();
		return t;
	}
	static const std::array<Instruction::OpcodeInfo, 8>& grp4Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildGroup4();
		return t;
	}
	static const std::array<Instruction::OpcodeInfo, 8>& grp5Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildGroup5();
		return t;
	}

	// The group table an opcode extends. Throws if the opcode is not a group.
	static const std::array<Instruction::OpcodeInfo, 8>& groupTableOf(uint32_t op) {
		switch (groupNoOf(op)) {
		case 1: return grp1Table();
		case 2: return grp2Table();
		case 3: return grp3Table();
		case 4: return grp4Table();
		case 5: return grp5Table();
		default:
			throw std::runtime_error("Opcode is not an extension group..");
		}
	}

	// Resolve a group opcode + ModRM.reg to the instruction it actually names. An illegal
	// /reg (FF /7, FE /2..7) comes back with an empty text rather than throwing: a linear
	// sweep of .text runs over data, so invalid encodings are routine and must not kill it.
	static const Instruction::OpcodeInfo& groupEntryOf(uint32_t op, uint8_t reg) {
		return groupTableOf(op)[reg & 0x07];
	}

	// The one row every caller should read: the 256-entry row for a plain opcode, and for a group
	// opcode that row merged with the group entry ModRM.reg selects. Both the byte-eater and the
	// printer must go through this, or they disagree on how long the instruction is and the next
	// one decodes from the wrong offset.
	//
	// The two rows carry different halves of the truth, so neither can simply win:
	//   groups 1 and 2 (0x80-0x83, 0xC0-0xD3) spell their operands out in the outer row and use
	//     the group entry for the mnemonic alone - their entries name no operands at all;
	//   groups 4 and 5 (0xFE, 0xFF) leave the outer row empty and carry operands in the entry;
	//   group 3 (0xF6/0xF7) splits them - the outer row knows the operand width, and only the
	//     entry knows that /0 and /1 (TEST) take an immediate the outer row never mentions.
	// So: the entry wins wherever it names an operand, the outer row fills every blank, and a
	// size of None on the entry means "the width this opcode works at" - which is what the outer
	// row records in op1s. That is the only thing group 3's shared entries cannot know for
	// themselves, since they are reached through both 0xF6 (byte) and 0xF7 (word/dword).
	static Instruction::OpcodeInfo resolvedInfo(uint32_t op, uint8_t reg) {
		const Instruction::OpcodeInfo& outer = opcodeTable()[op];
		if (!isGroup(op)) return outer;

		const Instruction::OpcodeInfo& entry = groupEntryOf(op, reg);
		if (entry.text.empty()) return entry;   // illegal /reg (FF /7, FE /2..7): "(bad)", no operands

		constexpr uint8_t none = static_cast<uint8_t>(SIZE::None);
		constexpr uint8_t noMode = static_cast<uint8_t>(ADDRESSING::None);

		Instruction::OpcodeInfo info = outer;
		info.text = entry.text;
		info.hasImmediateByte = outer.hasImmediateByte || entry.hasImmediateByte;

		const uint8_t opSize = (outer.op1s != none) ? outer.op1s : entry.op1s;

		auto take = [&](uint8_t entryMode, uint8_t entrySize, uint8_t& mode, uint8_t& size) {
			if (entryMode == noMode) return;                     // entry says nothing: keep the outer row's
			mode = entryMode;
			size = (entrySize != none) ? entrySize : opSize;
		};
		take(entry.op1am, entry.op1s, info.op1am, info.op1s);
		take(entry.op2am, entry.op2s, info.op2am, info.op2s);
		take(entry.op3am, entry.op3s, info.op3am, info.op3s);

		return info;
	}

//   E=modrm r/m, G=modrm reg, I=imm, J=rel offset, O=moffs, S=seg reg, M=memory,
//   A=far ptr, Z=register in low 3 bits of opcode (+r), AL/eAX/DX/CL/One=implicit
enum class ADDRESSING : uint8_t {
	None, E, G, I, J, O, S, M, A, Z, AL, eAX, DX, CL, One,
	eCX, eDX, eBX, eSP, eBP, eSI, eDI,
	ES, CS, SS, DS,
	X, Y, F,
	DL, BL, AH, CH, DH, BH
};
enum class SIZE : uint8_t { None, b, w, v, z, p, a };   // b=8 w=16 v=16/32 z=imm16/32 p=far a=bound

enum class REGISTER : uint16_t {
	AX = 0x00,
	CX = 0x01,
	DX = 0x02,
	BX = 0x03,
	SP = 0x04,    // also AH
	BP = 0x05,   // also CH
	SI = 0x06,  // also DH
	DI = 0x07  // also BH
};

enum class Prefix : uint16_t {
	LOCK = 0xF0,
	REPNE = 0xF2,
	REP = 0xF3,

	// segment override

	CS = 0x2E,
	SS = 0x36,
	DS = 0x3E,
	ES = 0X26,
	FS = 0x64,
	GS = 0x65,

	HLT = 0xF4,
	CMC = 0xF5,

	INT1 = 0xF1,


	// operand-size prefix

	OPSIZE = 0x66,

	// branch hints
	NT = 0x2E,
	T = 0x3E,

	// address-size override prefix
	ADDRSIZE = 0x67


};

enum class OPCODE : uint32_t {
	ADD_EbGb = 0x00,
	ADD_EvGv = 0x01,
	ADD_GbEb = 0x02,
	ADD_GvEv = 0x03,
	ADD_ALIb = 0x04,
	ADD_eAXIv = 0x05,

	PUSH_ES = 0x06,
	POP_ES = 0x07,

	OR_EbGb = 0x08,
	OR_EvGv = 0x09,
	OR_GbEb = 0x0A,
	OR_GvEv = 0x0B,
	OR_ALIb = 0x0C,
	OR_eAXIv = 0x0D,

	PUSH_CS = 0x0E,
	TWOBYTE = 0x0F,

	ADC_EbGb = 0x10,
	ADC_EvGv = 0x11,
	ADC_GbEb = 0x12,
	ADC_GvEv = 0x13,
	ADC_ALIb = 0x14,
	ADC_eAXIv = 0x15,

	PUSH_SS = 0x16,
	POP_SS = 0x17,

	SBB_EbGb = 0x18,
	SBB_EvGv = 0x19,
	SBB_GbEb = 0x1A,
	SBB_GvEv = 0x1B,
	SBB_ALIb = 0x1C,
	SBB_eAXIv = 0x1D,

	PUSH_DS = 0x1E,
	POP_DS = 0x1F,

	AND_EbGb = 0x20,
	AND_EvGv = 0x21,
	AND_GbEb = 0x22,
	AND_GvEv = 0x23,
	AND_ALIb = 0x24,
	AND_eAXIv = 0x25,

	ES = 0x26,
	DAA = 0x27,

	SUB_EbGb = 0x28,
	SUB_EvGv = 0x29,
	SUB_GbEb = 0x2A,
	SUB_GvEv = 0x2B,
	SUB_ALIb = 0x2C,
	SUB_eAXIv = 0x2D,

	CS = 0x2E,
	DAS = 0x2F,


	XOR_EbGb = 0x30,
	XOR_EvGv = 0x31,
	XOR_GbEb = 0x32,
	XOR_GvEv = 0x33,
	XOR_ALIb = 0x34,
	XOR_eAXIv = 0x35,

	SS = 0x36,
	AAA = 0x37,

	CMP_EbGb = 0x38,
	CMP_EvGv = 0x39,
	CMP_GbEb = 0x3A,
	CMP_GvEv = 0x3B,
	CMP_ALIb = 0x3C,
	CMP_eAXIv = 0x3D,

	DS = 0x3E,
	AAS = 0x3F,

	INC_eAX = 0x40,
	INC_eCX = 0x41,
	INC_eDX = 0x42,
	INC_eBX = 0x43,
	INC_eSP = 0x44,
	INC_eBP = 0x45,
	INC_eSI = 0x46,
	INC_eDI = 0x47,

	DEC_eAX = 0x48,
	DEC_eCX = 0x49,
	DEC_eDX = 0x4A,
	DEC_eBX = 0x4B,
	DEC_eSP = 0x4C,
	DEC_eBP = 0x4D,
	DEC_eSI = 0x4E,
	DEC_eDI = 0x4F,


	PUSH_eAX = 0x50,
	PUSH_eCX = 0x51,
	PUSH_eDX = 0x52,
	PUSH_eBX = 0x53,
	PUSH_eSP = 0x54,
	PUSH_eBP = 0x55,
	PUSH_eSI = 0x56,
	PUSH_eDI = 0x57,


	POP_eAX = 0x58,
	POP_eCX = 0x59,
	POP_eDX = 0x5A,
	POP_eBX = 0x5B,
	POP_eSP = 0x5C,
	POP_eBP = 0x5D,
	POP_eSI = 0x5E,
	POP_eDI = 0x5F,

	PUSHA = 0x60,
	POPA = 0x61,

	BOUND_GvMa = 0x62,
	ARPL_EwGw = 0x63,
	FS = 0x64,
	GS = 0x65,
	OPSIZE = 0x66,
	ADSIZE = 0x67,

	PUSH_Iv = 0x68,
	IMUL_GvEvIv = 0x69,
	PUSH_Ib = 0x6A,
	IMUL_GvEvIb = 0x6B,

	INSB_YbDX = 0x6C,
	INSW_YzDX = 0x6D,
	OUTSB_DXXb = 0x6E,
	OUTSW_DXXv = 0x6F,

	// jumps

	JO = 0x70,
	JNO = 0x71,
	JB = 0x72,
	JNB = 0x73,
	JZ = 0x74,
	JNZ = 0x75,
	JBE = 0x76,
	JA = 0x77,
	JS = 0x78,
	JNS = 0x79,
	JP = 0x7A,
	JNP = 0x7B,
	JL = 0x7C,
	JNL = 0x7D,
	JLE = 0x7E,
	JNLE = 0x7F,


	// Group 1: ADD/OR/ADC/SBB/AND/SUB/XOR/CMP — mnemonic comes from ModRM.reg (/0../7)
	GRP1_EbIb  = 0x80,   // Eb, Ib
	GRP1_EvIz  = 0x81,   // Ev, Iz
	GRP1_EbIb2 = 0x82,   // Eb, Ib (undocumented alias of 0x80, invalid in 64-bit)
	GRP1_EvIb  = 0x83,   // Ev, Ib (imm8 sign-extended)

	TEST_EbGb = 0x84,
	TEST_EvGv = 0x85,

	XCHG_EbGb = 0x86,
	XCHG_EvGv = 0x87,

	MOV_EbGb = 0x88,
	MOV_EvGv = 0x89,
	MOV_GbEb = 0x8A,
	MOV_GvEv = 0x8B,
	MOV_EwSw = 0x8C,

	LEA_GvM = 0x8D,

	MOV_SwEw = 0x8E,

	POP_Ev = 0x8F,

	NOP = 0x90,
	XCHG_eAXeCX = 0x91,
	XCHG_eAXeDX = 0x92,
	XCHG_eAXeBX = 0x93,
	XCHG_eAXeSP = 0x94,
	XCHG_eAXeBP = 0x95,
	XCHG_eAXeSI = 0x96,
	XCHG_eAXeDI = 0x97,

	CBW = 0x98,
	CWD = 0x99,
	CALL_Ap = 0x9A,
	FWAIT = 0x9B,
	PUSHF_Fv = 0x9C,
	POPF_Fv = 0x9D,
	SAHF = 0x9E,
	LAHF = 0x9F,

	// MOV to/from accumulator (direct offset)
	MOV_ALOb = 0xA0,
	MOV_eAXOv = 0xA1,
	MOV_ObAL = 0xA2,
	MOV_OveAX = 0xA3,

	// string operations
	MOVSB_XbYb = 0xA4,
	MOVSW_XvYv = 0xA5,
	CMPSB_XbYb = 0xA6,
	CMPSW_XvYv = 0xA7,
	TEST_ALIb = 0xA8,
	TEST_eAXIv = 0xA9,
	STOSB_YbAL = 0xAA,
	STOSW_YveAX = 0xAB,
	LODSB_ALXb = 0xAC,
	LODSW_eAXXv = 0xAD,
	SCASB_ALYb = 0xAE,
	SCASW_eAXYv = 0xAF,

	// MOV immediate to byte register
	MOV_ALIb = 0xB0,
	MOV_CLIb = 0xB1,
	MOV_DLIb = 0xB2,
	MOV_BLIb = 0xB3,
	MOV_AHIb = 0xB4,
	MOV_CHIb = 0xB5,
	MOV_DHIb = 0xB6,
	MOV_BHIb = 0xB7,

	// MOV immediate to word/dword register
	MOV_eAXIv = 0xB8,
	MOV_eCXIv = 0xB9,
	MOV_eDXIv = 0xBA,
	MOV_eBXIv = 0xBB,
	MOV_eSPIv = 0xBC,
	MOV_eBPIv = 0xBD,
	MOV_eSIIv = 0xBE,
	MOV_eDIIv = 0xBF,

	// shift / rotate (Group 2) and other control flow
	GRP2_EbIb = 0xC0,
	GRP2_EvIb = 0xC1,
	RET_Iw = 0xC2,
	RET = 0xC3,
	LES_GvMp = 0xC4,
	LDS_GvMp = 0xC5,
	MOV_EbIb = 0xC6,
	MOV_EvIv = 0xC7,
	ENTER_IwIb = 0xC8,
	LEAVE = 0xC9,
	RETF_Iw = 0xCA,
	RETF = 0xCB,
	INT3 = 0xCC,
	INT_Ib = 0xCD,
	INTO = 0xCE,
	IRET = 0xCF,

	// shift / rotate (Group 2) – by 1 or CL
	GRP2_Eb1 = 0xD0,
	GRP2_Ev1 = 0xD1,
	GRP2_EbCL = 0xD2,
	GRP2_EvCL = 0xD3,

	AAM_Ib = 0xD4,
	AAD_Ib = 0xD5,
	SALC = 0xD6,
	XLAT = 0xD7,

	// x87 FPU escape opcodes
	ESC0 = 0xD8,
	ESC1 = 0xD9,
	ESC2 = 0xDA,
	ESC3 = 0xDB,
	ESC4 = 0xDC,
	ESC5 = 0xDD,
	ESC6 = 0xDE,
	ESC7 = 0xDF,

	// loop and I/O
	LOOPNZ_Jb = 0xE0,
	LOOPZ_Jb = 0xE1,
	LOOP_Jb = 0xE2,
	JeCXZ_Jb = 0xE3,
	//JCXZ_Jb = 0x67E3,   // prefix+opcode, not a table index: resolved at decode time from the 67 prefix
	IN_ALIb = 0xE4,
	IN_eAXIb = 0xE5,
	OUT_IbAL = 0xE6,
	OUT_IbeAX = 0xE7,

	// call / jump
	CALL_Jv = 0xE8,
	JMP_Jv = 0xE9,
	JMP_Ap = 0xEA,
	JMP_Jb = 0xEB,

	// I/O via DX
	IN_ALDX = 0xEC,
	IN_eAXDX = 0xED,
	OUT_DXAL = 0xEE,
	OUT_DXeAX = 0xEF,

	// prefixes / misc (also appear as opcode bytes)
	LOCK = 0xF0,
	INT1 = 0xF1,
	REPNE = 0xF2,
	REP = 0xF3,

	HLT = 0xF4,
	CMC = 0xF5,

	// unary arithmetic / l ogical (Group 3)
	GRP3_Eb = 0xF6,
	GRP3_Ev = 0xF7,

	// flag operations
	CLC = 0xF8,
	STC = 0xF9,
	CLI = 0xFA,
	STI = 0xFB,
	CLD = 0xFC,
	STD = 0xFD,

	// INC / DEC byte r/m (Group 4) and indirect call/jmp (Group 5)
	GRP4 = 0xFE,
	GRP5 = 0xFF,

};

};