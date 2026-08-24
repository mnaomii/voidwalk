#include "../instruction.hpp"
#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <stdexcept>
#pragma once


class x86_64_Mnemonic{
public:

	// Reg number -> name at a given operand size. size b picks the 8-bit set (AL..BH),
	// where reg 4..7 alias to AH/CH/DH/BH instead of SP/BP/SI/DI; size w is always 16-bit;
	// size v/z is 32-bit unless a 0x66 prefix (opsize16) drops it to 16-bit. Callers hand
	// E/G registers the operand size and the memory base/index the address size, so the two
	// no longer share one flag.

	static std::string registerOf16(uint16_t r, bool hasAddrSize) {

		static constexpr std::string_view addr16[] = {"BX + SI","BX + DI","BP + SI","BP + DI","SI","DI","BP","BX"}; 
		return hasAddrSize ? std::string(addr16[r]) : "";
	}

	static std::string registerOf(const uint16_t& r, const uint8_t size, const bool& is16bit, const bool& is64bit = false) {
		static constexpr std::string_view r8_32 [] = { "AL","CL","DL","BL",
														"AH","CH","DH","BH" };

		static constexpr std::string_view r8_64[] = {  "AL","CL","DL","BL" ,
													   "SPL", "BPL", "SIL", "DIL",
													   "R8B", "R9B", "R10B", "R11B", "R12B", "R13B", "R14B", "R15B" };

		static constexpr std::string_view r16[] = { "AX","CX","DX","BX","SP","BP","SI","DI" ,
													"R8W", "R9W", "R10W", "R11W", "R12W", "R13W", "R14W", "R15W" };

		static constexpr std::string_view r32[] = { "EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI",
											"R8D", "R9D", "R10D", "R11D", "R12D", "R13D", "R14D", "R15D" };

		static constexpr std::string_view r64[] = { "RAX","RCX","RDX","RBX","RSP","RBP","RSI","RDI",
											"R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

		if (r > 15)
			throw std::runtime_error("Malformed expression detected..");
		switch (static_cast<SIZE>(size)) {
		case SIZE::b: return (is64bit) ? std::string(r8_64[r]) : std::string(r8_32[r]);
		case SIZE::w: return std::string(r16[r]);
		default:      return is64bit ? std::string(r64[r])                                 // v/z: REX.W(64) >
		                   : (is16bit ? std::string(r16[r]) : std::string(r32[r]));        //      0x66(16) > 32
		}
	}

	// Legacy two-arg form: a bare is16bit flag means "default operand size, 16 or 32".
	// Kept so existing callers keep working; it cannot reach the 8-bit set (pass a size for that).
	static std::string registerOf(uint16_t r, bool is16bit, bool is64bit = false) {
		return registerOf(r, static_cast<uint8_t>(SIZE::v), is16bit, is64bit);
	}


	// ModRM.reg as a segment register (MOV Ew,Sw / MOV Sw,Ew). reg 6 and 7 name no
	// segment register, so the encoding is invalid - but a linear sweep of .text runs
	// over data and over opcodes we cannot decode yet, so invalid encodings are normal
	// and must not kill the run. Mark them the way objdump does instead of throwing.
	static std::string segmentOf(uint16_t r) {
		constexpr std::string_view names[8] = { "ES", "CS", "SS", "DS", "FS", "GS", "(bad)", "(bad)" };
		return std::string(names[r & 0x07]);
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

	static constexpr std::array<Instruction::OpcodeInfo, 256> buildOpcodes() {

		std::array<Instruction::OpcodeInfo, 256> n{};

		// A row is: mnemonic (32-bit name), then op1/op2/op3, then the group number.
		//   P    - plain opcode, no group.
		//   PG   - plain opcode that is an extension group (group number last).
		//   P16  - the mnemonic itself flips with the operand size (32-bit name, then 16-bit name).
		//   PG16 - the general form both build on.
		// Each operand is OP(addressingMode, size, value, value16), or NOP_ for an absent one.
		// value/value16 carry a silicon-named operand ("AL", "EAX"/"AX"); "" means "decode it
		// from the bytes". For an operand whose name does not change with a 0x66 prefix, value16
		// duplicates value.
#define PG16(name,text,text16, hasRM, op1, op2, op3, group) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,text16, hasRM, op1, op2, op3, group}
#define PG(name,text, hasRM, op1, op2, op3, group) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define P16(name,text,text16, hasRM, op1, op2, op3) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,text16, hasRM, op1, op2, op3, -1}
#define P(name,text, hasRM, op1, op2, op3) n[static_cast<uint32_t>(OPCODE::name)] = Instruction::OpcodeInfo{text,"", hasRM, op1, op2, op3, -1}
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define OP(mode,sz,val,val16) Instruction::TableOperand{ a(mode), s(sz), false, val, val, val16 }
#define NOP_ OP(None,None,"","")

		P(ADD_EbGb, "ADD", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(ADD_EvGv, "ADD", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(ADD_GbEb, "ADD", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(ADD_GvEv, "ADD", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(ADD_ALIb, "ADD", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(ADD_eAXIv, "ADD", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(PUSH_ES, "PUSH", false, OP(ES,None,"ES","ES"), NOP_, NOP_);
		P(POP_ES, "POP", false, OP(ES,None,"ES","ES"), NOP_, NOP_);

		P(OR_EbGb, "OR", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(OR_EvGv, "OR", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(OR_GbEb, "OR", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(OR_GvEv, "OR", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(OR_ALIb, "OR", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(OR_eAXIv, "OR", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(PUSH_CS, "PUSH", false, OP(CS,None,"CS","CS"), NOP_, NOP_);
		P(TWOBYTE, "2BYTE", false, NOP_, NOP_, NOP_);

		P(ADC_EbGb, "ADC", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(ADC_EvGv, "ADC", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(ADC_GbEb, "ADC", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(ADC_GvEv, "ADC", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(ADC_ALIb, "ADC", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(ADC_eAXIv, "ADC", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(PUSH_SS, "PUSH", false, OP(SS,None,"SS","SS"), NOP_, NOP_);
		P(POP_SS, "POP", false, OP(SS,None,"SS","SS"), NOP_, NOP_);

		P(SBB_EbGb, "SBB", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(SBB_EvGv, "SBB", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(SBB_GbEb, "SBB", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(SBB_GvEv, "SBB", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(SBB_ALIb, "SBB", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(SBB_eAXIv, "SBB", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(PUSH_DS, "PUSH", false, OP(DS,None,"DS","DS"), NOP_, NOP_);
		P(POP_DS, "POP", false, OP(DS,None,"DS","DS"), NOP_, NOP_);

		P(AND_EbGb, "AND", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(AND_EvGv, "AND", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(AND_GbEb, "AND", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(AND_GvEv, "AND", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(AND_ALIb, "AND", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(AND_eAXIv, "AND", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(ES, "ES", false, NOP_, NOP_, NOP_);
		P(DAA, "DAA", false, NOP_, NOP_, NOP_);

		P(SUB_EbGb, "SUB", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(SUB_EvGv, "SUB", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(SUB_GbEb, "SUB", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(SUB_GvEv, "SUB", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(SUB_ALIb, "SUB", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(SUB_eAXIv, "SUB", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(CS, "CS", false, NOP_, NOP_, NOP_);
		P(DAS, "DAS", false, NOP_, NOP_, NOP_);

		P(XOR_EbGb, "XOR", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(XOR_EvGv, "XOR", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(XOR_GbEb, "XOR", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(XOR_GvEv, "XOR", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(XOR_ALIb, "XOR", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(XOR_eAXIv, "XOR", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(SS, "SS", false, NOP_, NOP_, NOP_);
		P(AAA, "AAA", false, NOP_, NOP_, NOP_);

		P(CMP_EbGb, "CMP", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(CMP_EvGv, "CMP", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(CMP_GbEb, "CMP", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(CMP_GvEv, "CMP", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(CMP_ALIb, "CMP", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(CMP_eAXIv, "CMP", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);

		P(DS, "DS", false, NOP_, NOP_, NOP_);
		P(AAS, "AAS", false, NOP_, NOP_, NOP_);

		P(INC_eAX, "INC", false, OP(eAX,v,"EAX","AX"), NOP_, NOP_);
		P(INC_eCX, "INC", false, OP(eCX,v,"ECX","CX"), NOP_, NOP_);
		P(INC_eDX, "INC", false, OP(eDX,v,"EDX","DX"), NOP_, NOP_);
		P(INC_eBX, "INC", false, OP(eBX,v,"EBX","BX"), NOP_, NOP_);
		P(INC_eSP, "INC", false, OP(eSP,v,"ESP","SP"), NOP_, NOP_);
		P(INC_eBP, "INC", false, OP(eBP,v,"EBP","BP"), NOP_, NOP_);
		P(INC_eSI, "INC", false, OP(eSI,v,"ESI","SI"), NOP_, NOP_);
		P(INC_eDI, "INC", false, OP(eDI,v,"EDI","DI"), NOP_, NOP_);

		P(DEC_eAX, "DEC", false, OP(eAX,v,"EAX","AX"), NOP_, NOP_);
		P(DEC_eCX, "DEC", false, OP(eCX,v,"ECX","CX"), NOP_, NOP_);
		P(DEC_eDX, "DEC", false, OP(eDX,v,"EDX","DX"), NOP_, NOP_);
		P(DEC_eBX, "DEC", false, OP(eBX,v,"EBX","BX"), NOP_, NOP_);
		P(DEC_eSP, "DEC", false, OP(eSP,v,"ESP","SP"), NOP_, NOP_);
		P(DEC_eBP, "DEC", false, OP(eBP,v,"EBP","BP"), NOP_, NOP_);
		P(DEC_eSI, "DEC", false, OP(eSI,v,"ESI","SI"), NOP_, NOP_);
		P(DEC_eDI, "DEC", false, OP(eDI,v,"EDI","DI"), NOP_, NOP_);

		// 50-5F PUSH/POP r: the register is the opcode's low 3 bits (+r). Z decodes it from
		// the byte (and folds in REX.B for R8-R15); no baked names. PUSH/POP are d64.
		P(PUSH_eAX, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eCX, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eDX, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eBX, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eSP, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eBP, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eSI, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);
		P(PUSH_eDI, "PUSH", false, OP(Z,v,"",""), NOP_, NOP_);

		P(POP_eAX, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eCX, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eDX, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eBX, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eSP, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eBP, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eSI, "POP", false, OP(Z,v,"",""), NOP_, NOP_);
		P(POP_eDI, "POP", false, OP(Z,v,"",""), NOP_, NOP_);

		P16(PUSHA, "PUSHAD", "PUSHA", false, NOP_, NOP_, NOP_);
		P16(POPA, "POPAD", "POPA", false, NOP_, NOP_, NOP_);

		P(BOUND_GvMa, "BOUND", true, OP(G,v,"",""), OP(M,a,"",""), NOP_);
		P(ARPL_EwGw, "ARPL", true, OP(E,w,"",""), OP(G,w,"",""), NOP_);
		P(FS, "FS", false, NOP_, NOP_, NOP_);
		P(GS, "GS", false, NOP_, NOP_, NOP_);
		P(OPSIZE, "OPSIZE", false, NOP_, NOP_, NOP_);
		P(ADSIZE, "ADSIZE", false, NOP_, NOP_, NOP_);

		P(PUSH_Iv, "PUSH", false, OP(I,z,"",""), NOP_, NOP_);
		P(IMUL_GvEvIv, "IMUL", true, OP(G,v,"",""), OP(E,v,"",""), OP(I,z,"",""));
		P(PUSH_Ib, "PUSH", false, OP(I,bs,"",""), NOP_, NOP_);            // imm8 sign-extended to opsize
		P(IMUL_GvEvIb, "IMUL", true, OP(G,v,"",""), OP(E,v,"",""), OP(I,bs,"",""));   // imm8 sign-extended

		P(INSB_YbDX, "INSB", false, OP(Y,b,"[EDI]","[EDI]"), OP(DX,None,"DX","DX"), NOP_);
		P16(INSW_YzDX, "INSD", "INSW", false, OP(Y,z,"[EDI]","[EDI]"), OP(DX,None,"DX","DX"), NOP_);
		P(OUTSB_DXXb, "OUTSB", false, OP(DX,None,"DX","DX"), OP(X,b,"[ESI]","[ESI]"), NOP_);
		P16(OUTSW_DXXv, "OUTSD", "OUTSW", false, OP(DX,None,"DX","DX"), OP(X,v,"[ESI]","[ESI]"), NOP_);

		P(JO, "JO", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNO, "JNO", false, OP(J,b,"",""), NOP_, NOP_);
		P(JB, "JB", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNB, "JNB", false, OP(J,b,"",""), NOP_, NOP_);
		P(JZ, "JZ", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNZ, "JNZ", false, OP(J,b,"",""), NOP_, NOP_);
		P(JBE, "JBE", false, OP(J,b,"",""), NOP_, NOP_);
		P(JA, "JA", false, OP(J,b,"",""), NOP_, NOP_);
		P(JS, "JS", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNS, "JNS", false, OP(J,b,"",""), NOP_, NOP_);
		P(JP, "JP", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNP, "JNP", false, OP(J,b,"",""), NOP_, NOP_);
		P(JL, "JL", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNL, "JNL", false, OP(J,b,"",""), NOP_, NOP_);
		P(JLE, "JLE", false, OP(J,b,"",""), NOP_, NOP_);
		P(JNLE, "JNLE", false, OP(J,b,"",""), NOP_, NOP_);

		// Group 1 (0x80-0x83): ADD/OR/ADC/SBB/AND/SUB/XOR/CMP, selected by ModRM.reg
		PG(GRP1_EbIb, "GRP1", true, OP(E,b,"",""), OP(I,b,"",""), NOP_, 1);
		PG(GRP1_EvIz, "GRP1", true, OP(E,v,"",""), OP(I,z,"",""), NOP_, 1);
		PG(GRP1_EbIb2, "GRP1", true, OP(E,b,"",""), OP(I,b,"",""), NOP_, 1);
		PG(GRP1_EvIb, "GRP1", true, OP(E,v,"",""), OP(I,bs,"",""), NOP_, 1);   // 83 /r ib: imm8 sign-extended to Ev width

		P(TEST_EbGb, "TEST", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(TEST_EvGv, "TEST", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);

		P(XCHG_EbGb, "XCHG", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(XCHG_EvGv, "XCHG", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);

		P(MOV_EbGb, "MOV", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		P(MOV_EvGv, "MOV", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		P(MOV_GbEb, "MOV", true, OP(G,b,"",""), OP(E,b,"",""), NOP_);
		P(MOV_GvEv, "MOV", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		P(MOV_EwSw, "MOV", true, OP(E,w,"",""), OP(S,w,"",""), NOP_);

		P(LEA_GvM, "LEA", true, OP(G,v,"",""), OP(M,None,"",""), NOP_);

		P(MOV_SwEw, "MOV", true, OP(S,w,"",""), OP(E,w,"",""), NOP_);

		P(POP_Ev, "POP", true, OP(E,v,"",""), NOP_, NOP_);

		P(NOP, "NOP", false, NOP_, NOP_, NOP_);
		// 90-97 XCHG eAX,r: op0 is the accumulator (reg 0, named in silicon - keeps value*),
		// op1 is the +r register decoded from the opcode via Z. Not d64, so 64-bit only under REX.W.
		P(XCHG_eAXeCX, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeDX, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeBX, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeSP, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeBP, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeSI, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);
		P(XCHG_eAXeDI, "XCHG", false, OP(eAX,v,"EAX","AX"), OP(Z,v,"",""), NOP_);

		// 0x98/0x99 name their operands in silicon and take no operand slots, so the operand
		// size can only show up in the mnemonic: CWDE sign-extends AX into EAX, CBW AL into AX;
		// CDQ sign-extends EAX into EDX:EAX, CWD AX into DX:AX.
		P16(CBW, "CWDE", "CBW", false, NOP_, NOP_, NOP_);
		P16(CWD, "CDQ", "CWD", false, NOP_, NOP_, NOP_);
		P(CALL_Ap, "CALL", false, OP(A,p,"",""), NOP_, NOP_);
		P(FWAIT, "FWAIT", false, NOP_, NOP_, NOP_);
		P16(PUSHF_Fv, "PUSHFD", "PUSHF", false, OP(F,v,"",""), NOP_, NOP_);
		P16(POPF_Fv, "POPFD", "POPF", false, OP(F,v,"",""), NOP_, NOP_);
		P(SAHF, "SAHF", false, NOP_, NOP_, NOP_);
		P(LAHF, "LAHF", false, NOP_, NOP_, NOP_);

		// MOV to/from accumulator, direct memory offset (moffs). The offset is a memory
		// reference, not an immediate - value is "" so the decoder builds [addr].
		P(MOV_ALOb, "MOV", false, OP(AL,b,"AL","AL"), OP(O,b,"",""), NOP_);
		P(MOV_eAXOv, "MOV", false, OP(eAX,v,"EAX","AX"), OP(O,v,"",""), NOP_);
		P(MOV_ObAL, "MOV", false, OP(O,b,"",""), OP(AL,b,"AL","AL"), NOP_);
		P(MOV_OveAX, "MOV", false, OP(O,v,"",""), OP(eAX,v,"EAX","AX"), NOP_);

		P(MOVSB_XbYb, "MOVSB", false, OP(X,b,"[ESI]","[ESI]"), OP(Y,b,"[EDI]","[EDI]"), NOP_);
		// Intel names the dword string ops MOVSD/CMPSD, which collide with the SSE2 scalar-double
		// MOVSD/CMPSD (F2 0F 10, F2 0F C2). Same spelling, unrelated instructions - the operands
		// tell them apart. AT&T sidesteps the clash by spelling these movsl/cmpsl instead.
		P16(MOVSW_XvYv, "MOVSD", "MOVSW", false, OP(X,v,"[ESI]","[ESI]"), OP(Y,v,"[EDI]","[EDI]"), NOP_);
		P(CMPSB_XbYb, "CMPSB", false, OP(X,b,"[ESI]","[ESI]"), OP(Y,b,"[EDI]","[EDI]"), NOP_);
		P16(CMPSW_XvYv, "CMPSD", "CMPSW", false, OP(X,v,"[ESI]","[ESI]"), OP(Y,v,"[EDI]","[EDI]"), NOP_);
		P(TEST_ALIb, "TEST", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(TEST_eAXIv, "TEST", false, OP(eAX,v,"EAX","AX"), OP(I,z,"",""), NOP_);
		P(STOSB_YbAL, "STOSB", false, OP(Y,b,"[EDI]","[EDI]"), OP(AL,b,"AL","AL"), NOP_);
		P16(STOSW_YveAX, "STOSD", "STOSW", false, OP(Y,v,"[EDI]","[EDI]"), OP(eAX,v,"EAX","AX"), NOP_);
		P(LODSB_ALXb, "LODSB", false, OP(AL,b,"AL","AL"), OP(X,b,"[ESI]","[ESI]"), NOP_);
		P16(LODSW_eAXXv, "LODSD", "LODSW", false, OP(eAX,v,"EAX","AX"), OP(X,v,"[ESI]","[ESI]"), NOP_);
		P(SCASB_ALYb, "SCASB", false, OP(AL,b,"AL","AL"), OP(Y,b,"[EDI]","[EDI]"), NOP_);
		P16(SCASW_eAXYv, "SCASD", "SCASW", false, OP(eAX,v,"EAX","AX"), OP(Y,v,"[EDI]","[EDI]"), NOP_);

		P(MOV_ALIb, "MOV", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(MOV_CLIb, "MOV", false, OP(CL,b,"CL","CL"), OP(I,b,"",""), NOP_);
		P(MOV_DLIb, "MOV", false, OP(DL,b,"DL","DL"), OP(I,b,"",""), NOP_);
		P(MOV_BLIb, "MOV", false, OP(BL,b,"BL","BL"), OP(I,b,"",""), NOP_);
		P(MOV_AHIb, "MOV", false, OP(AH,b,"AH","AH"), OP(I,b,"",""), NOP_);
		P(MOV_CHIb, "MOV", false, OP(CH,b,"CH","CH"), OP(I,b,"",""), NOP_);
		P(MOV_DHIb, "MOV", false, OP(DH,b,"DH","DH"), OP(I,b,"",""), NOP_);
		P(MOV_BHIb, "MOV", false, OP(BH,b,"BH","BH"), OP(I,b,"",""), NOP_);

		// B8-BF MOV r,imm: op0 is the +r register (Z); imm is v (imm64 under REX.W -> movabs).
		P(MOV_eAXIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eCXIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eDXIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eBXIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eSPIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eBPIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eSIIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);
		P(MOV_eDIIv, "MOV", false, OP(Z,v,"",""), OP(I,v,"",""), NOP_);

		// Group 2 (0xC0/0xC1, 0xD0-0xD3): ROL/ROR/RCL/RCR/SHL/SHR/SAL/SAR, selected by ModRM.reg
		PG(GRP2_EbIb, "GRP2", true, OP(E,b,"",""), OP(I,b,"",""), NOP_, 2);
		PG(GRP2_EvIb, "GRP2", true, OP(E,v,"",""), OP(I,b,"",""), NOP_, 2);
		P(RET_Iw, "RET", false, OP(I,w,"",""), NOP_, NOP_);
		P(RET, "RET", false, NOP_, NOP_, NOP_);
		P(LES_GvMp, "LES", true, OP(G,v,"",""), OP(M,p,"",""), NOP_);
		P(LDS_GvMp, "LDS", true, OP(G,v,"",""), OP(M,p,"",""), NOP_);
		P(MOV_EbIb, "MOV", true, OP(E,b,"",""), OP(I,b,"",""), NOP_);
		P(MOV_EvIv, "MOV", true, OP(E,v,"",""), OP(I,z,"",""), NOP_);
		P(ENTER_IwIb, "ENTER", false, OP(I,w,"",""), OP(I,b,"",""), NOP_);
		P(LEAVE, "LEAVE", false, NOP_, NOP_, NOP_);
		P(RETF_Iw, "RETF", false, OP(I,w,"",""), NOP_, NOP_);
		P(RETF, "RETF", false, NOP_, NOP_, NOP_);
		P(INT3, "INT3", false, NOP_, NOP_, NOP_);
		P(INT_Ib, "INT", false, OP(I,b,"",""), NOP_, NOP_);
		P(INTO, "INTO", false, NOP_, NOP_, NOP_);
		P16(IRET, "IRETD", "IRET", false, NOP_, NOP_, NOP_);

		PG(GRP2_Eb1, "GRP2", true, OP(E,b,"",""), OP(One,None,"1","1"), NOP_, 2);
		PG(GRP2_Ev1, "GRP2", true, OP(E,v,"",""), OP(One,None,"1","1"), NOP_, 2);
		PG(GRP2_EbCL, "GRP2", true, OP(E,b,"",""), OP(CL,None,"CL","CL"), NOP_, 2);
		PG(GRP2_EvCL, "GRP2", true, OP(E,v,"",""), OP(CL,None,"CL","CL"), NOP_, 2);

		P(AAM_Ib, "AAM", false, OP(I,b,"",""), NOP_, NOP_);
		P(AAD_Ib, "AAD", false, OP(I,b,"",""), NOP_, NOP_);
		P(SALC, "SALC", false, NOP_, NOP_, NOP_);
		P(XLAT, "XLAT", false, NOP_, NOP_, NOP_);

		P(ESC0, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC1, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC2, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC3, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC4, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC5, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC6, "ESC", true, NOP_, NOP_, NOP_);
		P(ESC7, "ESC", true, NOP_, NOP_, NOP_);

		P(LOOPNZ_Jb, "LOOPNZ", false, OP(J,b,"",""), NOP_, NOP_);
		P(LOOPZ_Jb, "LOOPZ", false, OP(J,b,"",""), NOP_, NOP_);
		P(LOOP_Jb, "LOOP", false, OP(J,b,"",""), NOP_, NOP_);
		P(JeCXZ_Jb, "JECXZ", false, OP(J,b,"",""), NOP_, NOP_);

		P(IN_ALIb, "IN", false, OP(AL,b,"AL","AL"), OP(I,b,"",""), NOP_);
		P(IN_eAXIb, "IN", false, OP(eAX,v,"EAX","AX"), OP(I,b,"",""), NOP_);
		P(OUT_IbAL, "OUT", false, OP(I,b,"",""), OP(AL,b,"AL","AL"), NOP_);
		P(OUT_IbeAX, "OUT", false, OP(I,b,"",""), OP(eAX,v,"EAX","AX"), NOP_);

		P(CALL_Jv, "CALL", false, OP(J,v,"",""), NOP_, NOP_);
		P(JMP_Jv, "JMP", false, OP(J,v,"",""), NOP_, NOP_);
		P(JMP_Ap, "JMP", false, OP(A,p,"",""), NOP_, NOP_);
		P(JMP_Jb, "JMP", false, OP(J,b,"",""), NOP_, NOP_);

		P(IN_ALDX, "IN", false, OP(AL,b,"AL","AL"), OP(DX,None,"DX","DX"), NOP_);
		P(IN_eAXDX, "IN", false, OP(eAX,v,"EAX","AX"), OP(DX,None,"DX","DX"), NOP_);
		P(OUT_DXAL, "OUT", false, OP(DX,None,"DX","DX"), OP(AL,b,"AL","AL"), NOP_);
		P(OUT_DXeAX, "OUT", false, OP(DX,None,"DX","DX"), OP(eAX,v,"EAX","AX"), NOP_);

		P(LOCK, "LOCK", false, NOP_, NOP_, NOP_);
		P(INT1, "INT1", false, NOP_, NOP_, NOP_);
		P(REPNE, "REPNE", false, NOP_, NOP_, NOP_);
		P(REP, "REP", false, NOP_, NOP_, NOP_);

		P(HLT, "HLT", false, NOP_, NOP_, NOP_);
		P(CMC, "CMC", false, NOP_, NOP_, NOP_);

		// Group 3 (0xF6/0xF7): TEST/NOT/NEG/MUL/IMUL/DIV/IDIV, selected by ModRM.reg
		PG(GRP3_Eb, "GRP3", true, OP(E,b,"",""), NOP_, NOP_, 3);
		PG(GRP3_Ev, "GRP3", true, OP(E,v,"",""), NOP_, NOP_, 3);

		P(CLC, "CLC", false, NOP_, NOP_, NOP_);
		P(STC, "STC", false, NOP_, NOP_, NOP_);
		P(CLI, "CLI", false, NOP_, NOP_, NOP_);
		P(STI, "STI", false, NOP_, NOP_, NOP_);
		P(CLD, "CLD", false, NOP_, NOP_, NOP_);
		P(STD, "STD", false, NOP_, NOP_, NOP_);

		// Group 4 (0xFE): INC/DEC Eb.  Group 5 (0xFF): INC/DEC/CALL/JMP/PUSH Ev.
		PG(GRP4, "GRP4", true, NOP_, NOP_, NOP_, 4);
		PG(GRP5, "GRP5", true, NOP_, NOP_, NOP_, 5);



#undef NOP_
#undef OP
#undef s
#undef a
#undef P
#undef PG
#undef P16
#undef PG16

		// --- 64-bit mode: one-byte encodings removed in long mode (raise #UD). The table is
		// shared with 32-bit decoding, so isInvalid == "gone in 64-bit mode"; a 32-bit decode
		// ignores it. NOTE 0x63 ARPL is deliberately NOT here: in 64-bit it is repurposed to
		// MOVSXD, a change of meaning (not a removal) that a single bool cannot express.
#define INV(name) n[static_cast<uint32_t>(OPCODE::name)].isInvalid = true
		INV(PUSH_ES);  INV(POP_ES);  INV(PUSH_CS);
		INV(PUSH_SS);  INV(POP_SS);  INV(PUSH_DS);  INV(POP_DS);
		INV(DAA); INV(DAS); INV(AAA); INV(AAS);
		INV(PUSHA); INV(POPA);
		INV(BOUND_GvMa);
		INV(GRP1_EbIb2);           // 0x82, undocumented grp1 alias
		INV(CALL_Ap);              // 0x9A far call ptr16:16/32
		INV(LES_GvMp); INV(LDS_GvMp);   // 0xC4/0xC5 (reused as VEX escapes)
		INV(INTO);
		INV(AAM_Ib); INV(AAD_Ib);
		INV(SALC);
		INV(JMP_Ap);               // 0xEA far jmp ptr16:16/32
		// 0x40-0x4F are REX prefixes in 64-bit mode; the byte-eater consumes them before this
		// table is consulted, but the INC/DEC r32 short forms they held are 32-bit only.
		INV(INC_eAX); INV(INC_eCX); INV(INC_eDX); INV(INC_eBX);
		INV(INC_eSP); INV(INC_eBP); INV(INC_eSI); INV(INC_eDI);
		INV(DEC_eAX); INV(DEC_eCX); INV(DEC_eDX); INV(DEC_eBX);
		INV(DEC_eSP); INV(DEC_eBP); INV(DEC_eSI); INV(DEC_eDI);
#undef INV

		// --- REX.W 64-bit mnemonics (text64): the width-in-the-name family. Mirror of the
		// text16 entries a few rows up. Renderer picks text64 when REX.W, before text16/text.
#define T64(name,name64) n[static_cast<uint32_t>(OPCODE::name)].text64 = name64
		T64(CBW, "CDQE");         T64(CWD, "CQO");
		T64(PUSHF_Fv, "PUSHFQ");  T64(POPF_Fv, "POPFQ");
		T64(MOVSW_XvYv, "MOVSQ"); T64(CMPSW_XvYv, "CMPSQ");
		T64(STOSW_YveAX, "STOSQ");T64(LODSW_eAXXv, "LODSQ"); T64(SCASW_eAXYv, "SCASQ");
		T64(IRET, "IRETQ");
		// INSD/OUTSD (0x6D/0x6F) have no 64-bit form - I/O ports max out at 32-bit, no text64.
#undef T64

		// --- long-mode default operand size (field: def64). Ignored by a 32-bit decode.
		//   d64: defaults to 64, but 0x66 drops it to 16.   f64: forced 64, 0x66 ignored.
#define D64(name) n[static_cast<uint32_t>(OPCODE::name)].def64 = Instruction::OpcodeInfo::Default64::d64
#define F64(name) n[static_cast<uint32_t>(OPCODE::name)].def64 = Instruction::OpcodeInfo::Default64::f64
		// stack ops (d64)
		D64(PUSH_eAX); D64(PUSH_eCX); D64(PUSH_eDX); D64(PUSH_eBX);
		D64(PUSH_eSP); D64(PUSH_eBP); D64(PUSH_eSI); D64(PUSH_eDI);
		D64(POP_eAX);  D64(POP_eCX);  D64(POP_eDX);  D64(POP_eBX);
		D64(POP_eSP);  D64(POP_eBP);  D64(POP_eSI);  D64(POP_eDI);
		D64(PUSH_Iv);  D64(PUSH_Ib);  D64(POP_Ev);
		D64(PUSHF_Fv); D64(POPF_Fv);
		D64(ENTER_IwIb); D64(LEAVE);
		// near branches (f64)
		F64(CALL_Jv); F64(JMP_Jv); F64(JMP_Jb);
		F64(LOOPNZ_Jb); F64(LOOPZ_Jb); F64(LOOP_Jb); F64(JeCXZ_Jb);
		F64(JO);F64(JNO);F64(JB);F64(JNB);F64(JZ);F64(JNZ);F64(JBE);F64(JA);
		F64(JS);F64(JNS);F64(JP);F64(JNP);F64(JL);F64(JNL);F64(JLE);F64(JNLE);
#undef F64
#undef D64

		// --- long-mode name for XCHG's accumulator operand (op0 = reg 0, named in silicon).
		// The +r operands (PUSH/POP/MOV, and XCHG's op1) now decode through the Z addressing
		// mode, which resolves REX.B and width itself - only this fixed accumulator still needs
		// a baked 64-bit name. The renderer's default operand case picks value64 under REX.W.
#define V64_0(name,r64) n[static_cast<uint32_t>(OPCODE::name)].op[0].value64 = r64
		V64_0(XCHG_eAXeCX,"RAX"); V64_0(XCHG_eAXeDX,"RAX"); V64_0(XCHG_eAXeBX,"RAX"); V64_0(XCHG_eAXeSP,"RAX");
		V64_0(XCHG_eAXeBP,"RAX"); V64_0(XCHG_eAXeSI,"RAX"); V64_0(XCHG_eAXeDI,"RAX");
#undef V64_0

		return n;

	}


	static const std::array<Instruction::OpcodeInfo, 256>& opcodeTable() {
		static constexpr std::array<Instruction::OpcodeInfo, 256> t = buildOpcodes();
		return t;
	}

	// > 0 when the opcode is a group: the mnemonic must still be resolved from ModRM.reg.
	static int groupNoOf(uint32_t op) { return opcodeTable()[op].groupNo; }
	static bool isGroup(uint32_t op) { return opcodeTable()[op].groupNo > 0; }

	// An immediate is present iff some operand is decoded from the instruction stream:
	// I (immediate), J (rel), A (far ptr), O (moffs). Replaces the old hasImmediateByte flag.
	static bool hasImmediate(const Instruction::OpcodeInfo& info) {
		auto isImm = [](uint8_t m) {
			return m == static_cast<uint8_t>(ADDRESSING::I)
			    || m == static_cast<uint8_t>(ADDRESSING::J)
			    || m == static_cast<uint8_t>(ADDRESSING::A)
			    || m == static_cast<uint8_t>(ADDRESSING::O);
		};
		return isImm(info.op[0].addressingMode) || isImm(info.op[1].addressingMode) || isImm(info.op[2].addressingMode);
	}



		// No group entry's mnemonic moves with the operand size, so the 16-bit name is always empty here.
#define G(reg,text, hasRM, op1, op2, op3, group) n[reg] = Instruction::OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define GT(reg,text,group) G(reg, text, true, NOP_, NOP_, NOP_, group)
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define OP(mode,sz,val,val16) Instruction::TableOperand{ a(mode), s(sz), false, val, val, val16 }
#define NOP_ OP(None,None,"","")

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
	// alias of /0. /4../7 use AL/eAX implicitly - that is not encoded here. A size of None
	// on the operands means "the width the outer row records" (b for 0xF6, v for 0xF7).
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup3() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "TEST", true, OP(E,None,"",""), OP(I,None,"",""), NOP_, 3);
		G(1, "TEST", true, OP(E,None,"",""), OP(I,None,"",""), NOP_, 3);
		G(2, "NOT", true, OP(E,None,"",""), NOP_, NOP_, 3);
		G(3, "NEG", true, OP(E,None,"",""), NOP_, NOP_, 3);
		G(4, "MUL", true, OP(E,None,"",""), NOP_, NOP_, 3);
		G(5, "IMUL", true, OP(E,None,"",""), NOP_, NOP_, 3);
		G(6, "DIV", true, OP(E,None,"",""), NOP_, NOP_, 3);
		G(7, "IDIV", true, OP(E,None,"",""), NOP_, NOP_, 3);
		return n;
	}

	// 0xFE. /2../7 are illegal.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup4() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "INC", true, OP(E,b,"",""), NOP_, NOP_, 4);
		G(1, "DEC", true, OP(E,b,"",""), NOP_, NOP_, 4);
		return n;
	}

	// 0xFF. /3 and /5 are the far forms (m16:32); /7 is illegal.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildGroup5() {
		std::array<Instruction::OpcodeInfo, 8> n{};
		G(0, "INC", true, OP(E,v,"",""), NOP_, NOP_, 5);
		G(1, "DEC", true, OP(E,v,"",""), NOP_, NOP_, 5);
		G(2, "CALL", true, OP(E,v,"",""), NOP_, NOP_, 5);
		G(3, "CALLF", true, OP(M,p,"",""), NOP_, NOP_, 5);
		G(4, "JMP", true, OP(E,v,"",""), NOP_, NOP_, 5);
		G(5, "JMPF", true, OP(M,p,"",""), NOP_, NOP_, 5);
		G(6, "PUSH", true, OP(E,v,"",""), NOP_, NOP_, 5);
		return n;
	}

#undef NOP_
#undef OP
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

	static const Instruction::OpcodeInfo& groupEntryOf(uint32_t op, uint8_t reg) {
		return groupTableOf(op)[reg & 0x07];
	}


	// --- x87 FPU instructions

	// Memory forms, indexed (op-0xD8)*8 + reg. Empty rows (invalid /reg) stay "(bad)".
	static constexpr std::array<Instruction::OpcodeInfo, 64> buildX87Mem() {
		std::array<Instruction::OpcodeInfo, 64> n{};
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define M_ Instruction::TableOperand{ a(M), s(None), false, "", "", "" }
#define NO Instruction::TableOperand{ a(None), s(None), false, "", "", "" }
#define FM(idx,text) n[idx] = Instruction::OpcodeInfo{ text, "", true, M_, NO, NO, -1 }
		// D8 (o=0): m32fp   D9 (o=1): load/store/control   DA (o=2): m32int
		FM(0,"FADD");   FM(1,"FMUL");   FM(2,"FCOM");    FM(3,"FCOMP");
		FM(4,"FSUB");   FM(5,"FSUBR");  FM(6,"FDIV");    FM(7,"FDIVR");
		FM(8,"FLD");                    FM(10,"FST");    FM(11,"FSTP");     // D9 /1 invalid
		FM(12,"FLDENV");FM(13,"FLDCW"); FM(14,"FNSTENV");FM(15,"FNSTCW");
		FM(16,"FIADD"); FM(17,"FIMUL"); FM(18,"FICOM");  FM(19,"FICOMP");
		FM(20,"FISUB"); FM(21,"FISUBR");FM(22,"FIDIV");  FM(23,"FIDIVR");
		// DB (o=3): m32int + m80fp     DC (o=4): m64fp
		FM(24,"FILD");  FM(25,"FISTTP");FM(26,"FIST");   FM(27,"FISTP");
		                FM(29,"FLD");                    FM(31,"FSTP");     // DB /4,/6 invalid
		FM(32,"FADD");  FM(33,"FMUL");  FM(34,"FCOM");   FM(35,"FCOMP");
		FM(36,"FSUB");  FM(37,"FSUBR"); FM(38,"FDIV");   FM(39,"FDIVR");
		// DD (o=5): m64fp + state      DE (o=6): m16int
		FM(40,"FLD");   FM(41,"FISTTP");FM(42,"FST");    FM(43,"FSTP");
		FM(44,"FRSTOR");                FM(46,"FNSAVE"); FM(47,"FNSTSW");   // DD /5 invalid
		FM(48,"FIADD"); FM(49,"FIMUL"); FM(50,"FICOM");  FM(51,"FICOMP");
		FM(52,"FISUB"); FM(53,"FISUBR");FM(54,"FIDIV");  FM(55,"FIDIVR");
		// DF (o=7): m16int/m64int + m80dec
		FM(56,"FILD");  FM(57,"FISTTP");FM(58,"FIST");   FM(59,"FISTP");
		FM(60,"FBLD");  FM(61,"FILD");  FM(62,"FBSTP");  FM(63,"FISTP");
#undef FM
#undef NO
#undef M_
#undef s
#undef a
		return n;
	}

	// Register forms (mod == 11), indexed (op-0xD8)*64 + (ModRM & 0x3F), i.e. reg*8+rm.
	// Regular families (a reg selects the op, rm selects ST(i)) are filled by a loop;
	// the E0-FF individuals (one mnemonic per rm, mostly no operand) are set explicitly.
	static constexpr std::array<Instruction::OpcodeInfo, 512> buildX87Reg() {
		std::array<Instruction::OpcodeInfo, 512> n{};
		constexpr std::string_view st[8] =
			{ "ST(0)","ST(1)","ST(2)","ST(3)","ST(4)","ST(5)","ST(6)","ST(7)" };
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define STv(v) Instruction::TableOperand{ a(ST), s(None), false, v, v, v }
#define NO Instruction::TableOperand{ a(None), s(None), false, "", "", "" }
		// op-relative base + reg row, one entry per rm.
#define ROW2(o,reg,text,p0,p1)  for (uint8_t i=0;i<8;++i) n[(o)*64+(reg)*8+i] = Instruction::OpcodeInfo{ text, "", true, p0, p1, NO, -1 }
#define ROW1(o,reg,text,p0)     for (uint8_t i=0;i<8;++i) n[(o)*64+(reg)*8+i] = Instruction::OpcodeInfo{ text, "", true, p0, NO, NO, -1 }
#define ONE(o,reg,rm,text)      n[(o)*64+(reg)*8+(rm)] = Instruction::OpcodeInfo{ text, "", true, NO, NO, NO, -1 }

		// D8 (o=0): "<op> ST, ST(i)" - reg is the op, same as its memory form.
		ROW2(0,0,"FADD", STv("ST"),STv(st[i]));  ROW2(0,1,"FMUL", STv("ST"),STv(st[i]));
		ROW1(0,2,"FCOM", STv(st[i]));            ROW1(0,3,"FCOMP",STv(st[i]));
		ROW2(0,4,"FSUB", STv("ST"),STv(st[i]));  ROW2(0,5,"FSUBR",STv("ST"),STv(st[i]));
		ROW2(0,6,"FDIV", STv("ST"),STv(st[i]));  ROW2(0,7,"FDIVR",STv("ST"),STv(st[i]));

		// D9 (o=1): FLD/FXCH ST(i), then the constant/transcendental individuals.
		ROW1(1,0,"FLD", STv(st[i]));  ROW1(1,1,"FXCH",STv(st[i]));
		ONE(1,2,0,"FNOP");
		ONE(1,4,0,"FCHS"); ONE(1,4,1,"FABS"); ONE(1,4,4,"FTST"); ONE(1,4,5,"FXAM");
		ONE(1,5,0,"FLD1"); ONE(1,5,1,"FLDL2T");ONE(1,5,2,"FLDL2E");ONE(1,5,3,"FLDPI");
		ONE(1,5,4,"FLDLG2");ONE(1,5,5,"FLDLN2");ONE(1,5,6,"FLDZ");
		ONE(1,6,0,"F2XM1");ONE(1,6,1,"FYL2X"); ONE(1,6,2,"FPTAN"); ONE(1,6,3,"FPATAN");
		ONE(1,6,4,"FXTRACT");ONE(1,6,5,"FPREM1");ONE(1,6,6,"FDECSTP");ONE(1,6,7,"FINCSTP");
		ONE(1,7,0,"FPREM");ONE(1,7,1,"FYL2XP1");ONE(1,7,2,"FSQRT");ONE(1,7,3,"FSINCOS");
		ONE(1,7,4,"FRNDINT");ONE(1,7,5,"FSCALE");ONE(1,7,6,"FSIN");ONE(1,7,7,"FCOS");

		// DA (o=2): FCMOVcc ST, ST(i); DA E9 FUCOMPP.
		ROW2(2,0,"FCMOVB", STv("ST"),STv(st[i])); ROW2(2,1,"FCMOVE", STv("ST"),STv(st[i]));
		ROW2(2,2,"FCMOVBE",STv("ST"),STv(st[i])); ROW2(2,3,"FCMOVU", STv("ST"),STv(st[i]));
		ONE(2,5,1,"FUCOMPP");

		// DB (o=3): FCMOVNcc ST, ST(i); FNCLEX/FNINIT; FUCOMI/FCOMI ST, ST(i).
		ROW2(3,0,"FCMOVNB", STv("ST"),STv(st[i])); ROW2(3,1,"FCMOVNE", STv("ST"),STv(st[i]));
		ROW2(3,2,"FCMOVNBE",STv("ST"),STv(st[i])); ROW2(3,3,"FCMOVNU", STv("ST"),STv(st[i]));
		ONE(3,4,2,"FNCLEX"); ONE(3,4,3,"FNINIT");
		ROW2(3,5,"FUCOMI",STv("ST"),STv(st[i]));   ROW2(3,6,"FCOMI", STv("ST"),STv(st[i]));

		// DC (o=4): "<op> ST(i), ST" - note reg4/5 and reg6/7 are the reversed forms.
		ROW2(4,0,"FADD", STv(st[i]),STv("ST")); ROW2(4,1,"FMUL", STv(st[i]),STv("ST"));
		ROW2(4,4,"FSUBR",STv(st[i]),STv("ST")); ROW2(4,5,"FSUB", STv(st[i]),STv("ST"));
		ROW2(4,6,"FDIVR",STv(st[i]),STv("ST")); ROW2(4,7,"FDIV", STv(st[i]),STv("ST"));

		// DD (o=5): FFREE / FST / FSTP / FUCOM / FUCOMP ST(i).
		ROW1(5,0,"FFREE",STv(st[i]));  ROW1(5,2,"FST",  STv(st[i])); ROW1(5,3,"FSTP",STv(st[i]));
		ROW1(5,4,"FUCOM",STv(st[i]));  ROW1(5,5,"FUCOMP",STv(st[i]));

		// DE (o=6): the popping arithmetic "<op>P ST(i), ST"; DE D9 FCOMPP.
		ROW2(6,0,"FADDP", STv(st[i]),STv("ST")); ROW2(6,1,"FMULP", STv(st[i]),STv("ST"));
		ONE(6,3,1,"FCOMPP");
		ROW2(6,4,"FSUBRP",STv(st[i]),STv("ST")); ROW2(6,5,"FSUBP", STv(st[i]),STv("ST"));
		ROW2(6,6,"FDIVRP",STv(st[i]),STv("ST")); ROW2(6,7,"FDIVP", STv(st[i]),STv("ST"));

		// DF (o=7): FNSTSW AX (the one x87 op that names a GP reg); FUCOMIP/FCOMIP ST, ST(i).
		n[7*64+4*8+0] = Instruction::OpcodeInfo{ "FNSTSW","",true, STv("AX"), NO, NO, -1 };
		ROW2(7,5,"FUCOMIP",STv("ST"),STv(st[i])); ROW2(7,6,"FCOMIP",STv("ST"),STv(st[i]));

#undef ONE
#undef ROW1
#undef ROW2
#undef NO
#undef STv
#undef s
#undef a
		return n;
	}

	static const std::array<Instruction::OpcodeInfo, 64>& x87MemTable() {
		static constexpr std::array<Instruction::OpcodeInfo, 64> t = buildX87Mem();
		return t;
	}
	static const std::array<Instruction::OpcodeInfo, 512>& x87RegTable() {
		static constexpr std::array<Instruction::OpcodeInfo, 512> t = buildX87Reg();
		return t;
	}

	static bool isX87(uint32_t op) { return op >= 0xD8 && op <= 0xDF; }

	// Full-ModRM resolution for an x87 escape opcode: memory row by reg, or the
	// register row by the whole reg:rm pair.
	static Instruction::OpcodeInfo x87ResolvedInfo(uint32_t op, uint8_t modrm) {
		const uint32_t o = op - 0xD8;                       // 0..7
		if (modrm < 0xC0)                                    // mod != 11 -> memory form
			return x87MemTable()[o * 8 + ((modrm >> 3) & 0x07)];
		return x87RegTable()[o * 64 + (modrm & 0x3F)];       // mod == 11 -> register form
	}

	// Merge the flat one-byte row with the entry ModRM selects. Takes the WHOLE ModRM
	// byte now: groups still key on reg (bits 3-5), but x87 needs the full byte, so the
	// dispatch happens here rather than in the caller.
	static Instruction::OpcodeInfo resolvedInfo(uint32_t op, uint8_t modrm, bool is64Bit = false) {
		if (isX87(op)) return x87ResolvedInfo(op, modrm);


		static constexpr Instruction::OpcodeInfo conflictRow{
		"MOVSXD", "",                                                                                   // text, text16
		true,                                                                                           // hasRMByte

		// op[0]: Gv dest
		Instruction::TableOperand{ static_cast<uint8_t>(ADDRESSING::G),    static_cast<uint8_t>(SIZE::v),    false, "", "", "" },  

		// op[1]: Ed source
		Instruction::TableOperand{ static_cast<uint8_t>(ADDRESSING::E),    static_cast<uint8_t>(SIZE::v),    false, "", "", "" },  

		// op[2]: none
		Instruction::TableOperand{ static_cast<uint8_t>(ADDRESSING::None), static_cast<uint8_t>(SIZE::None), false, "", "", "" },  

		// groupNo (not a group)
		-1                                                                                             
			};

		const uint8_t reg = (modrm >> 3) & 0x07;
		if (is64Bit && op == 0x63) return conflictRow;
		const Instruction::OpcodeInfo& outer = opcodeTable()[op];
		if (!isGroup(op)) return outer;

		const Instruction::OpcodeInfo& entry = groupEntryOf(op, reg);
		if (entry.text.empty()) return entry;   // illegal /reg (FF /7, FE /2..7): "(bad)", no operands

		constexpr uint8_t none = static_cast<uint8_t>(SIZE::None);
		constexpr uint8_t noMode = static_cast<uint8_t>(ADDRESSING::None);

		Instruction::OpcodeInfo info = outer;
		info.text = entry.text;
		info.text16 = entry.text16;   // both names come from whichever row named the instruction

		const uint8_t opSize = (outer.op[0].size != none) ? outer.op[0].size : entry.op[0].size;

		auto take = [&](const Instruction::TableOperand& e, Instruction::TableOperand& dst) {
			if (e.addressingMode == noMode) return;              // entry says nothing: keep the outer row's
			dst.addressingMode = e.addressingMode;
			dst.size    = (e.size != none) ? e.size : opSize;
			dst.forcedSize = e.forcedSize;
				dst.value64 = e.value64;
				dst.value32 = e.value32;
			dst.value16 = e.value16;
		};
		take(entry.op[0], info.op[0]);
		take(entry.op[1], info.op[1]);
		take(entry.op[2], info.op[2]);

		return info;
	}


	// Two-byte (0F-escape) opcode map. A SEPARATE 256-entry table keyed by the byte
	// after 0F: 0F B6 is MOVZX, unrelated to one-byte B6. Never index opcodeTable()
	// with an 0F pair - that array is 0x00-0xFF and 0x0Fxx runs off the end.
	//
	// First cut: the integer opcodes compilers actually emit. Unpopulated slots stay
	// empty (rendered "(bad)") but still length-correct at 2 bytes, since they carry
	// no ModRM/immediate here. Deferred: SSE / mandatory-prefix forms (66/F2/F3 0F xx),
	// the three-byte 0F 38 / 0F 3A maps, and groups 6/7/9/15/16. Only group 8 (0F BA)
	// is wired, because it carries an imm8 and so affects length.
	static constexpr std::array<Instruction::OpcodeInfo, 256> buildTwoByteOpcodes() {
		std::array<Instruction::OpcodeInfo, 256> n{};

#define T16(idx,text,text16,hasRM,op1,op2,op3) n[idx] = Instruction::OpcodeInfo{text,text16, hasRM, op1, op2, op3, -1}
#define T(idx,text,hasRM,op1,op2,op3)          n[idx] = Instruction::OpcodeInfo{text,"", hasRM, op1, op2, op3, -1}
#define TG(idx,text,hasRM,op1,op2,op3,group)   n[idx] = Instruction::OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define OP(mode,sz,val,val16) Instruction::TableOperand{ a(mode), s(sz), false, val, val, val16 }
#define NOP_ OP(None,None,"","")

		T(0x0B, "UD2",   false, NOP_, NOP_, NOP_);
		T(0x31, "RDTSC", false, NOP_, NOP_, NOP_);
		T(0xA2, "CPUID", false, NOP_, NOP_, NOP_);

		// Fast system call/return - no ModRM, no operands, no immediate.
		T(0x05, "SYSCALL",  false, NOP_, NOP_, NOP_);
		T(0x07, "SYSRET",   false, NOP_, NOP_, NOP_);
		T(0x34, "SYSENTER", false, NOP_, NOP_, NOP_);
		T(0x35, "SYSEXIT",  false, NOP_, NOP_, NOP_);

		// Multi-byte NOP (0F 1F /0) - the padding between functions. Has a ModRM, so its
		// addressed bytes must be eaten or every run of it desyncs the sweep.
		T(0x1F, "NOP", true, OP(E,v,"",""), NOP_, NOP_);

		// CMOVcc Gv, Ev  (0F 40-4F)
		T(0x40,"CMOVO",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x41,"CMOVNO", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x42,"CMOVB",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x43,"CMOVNB", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x44,"CMOVZ",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x45,"CMOVNZ", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x46,"CMOVBE", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x47,"CMOVA",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x48,"CMOVS",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x49,"CMOVNS", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4A,"CMOVP",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4B,"CMOVNP", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4C,"CMOVL",  true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4D,"CMOVNL", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4E,"CMOVLE", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0x4F,"CMOVNLE",true, OP(G,v,"",""), OP(E,v,"",""), NOP_);

		// Jcc rel16/rel32  (0F 80-8F) - no ModRM; z immediate (rel16 under a 0x66)
		T(0x80,"JO",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x81,"JNO", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x82,"JB",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x83,"JNB", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x84,"JZ",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x85,"JNZ", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x86,"JBE", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x87,"JA",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x88,"JS",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x89,"JNS", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8A,"JP",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8B,"JNP", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8C,"JL",  false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8D,"JNL", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8E,"JLE", false, OP(J,z,"",""), NOP_, NOP_);
		T(0x8F,"JNLE",false, OP(J,z,"",""), NOP_, NOP_);

		// SETcc Eb  (0F 90-9F) - ModRM, no immediate
		T(0x90,"SETO",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x91,"SETNO", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x92,"SETB",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x93,"SETNB", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x94,"SETZ",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x95,"SETNZ", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x96,"SETBE", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x97,"SETA",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x98,"SETS",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x99,"SETNS", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9A,"SETP",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9B,"SETNP", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9C,"SETL",  true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9D,"SETNL", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9E,"SETLE", true, OP(E,b,"",""), NOP_, NOP_);
		T(0x9F,"SETNLE",true, OP(E,b,"",""), NOP_, NOP_);

		// Bit tests: reg forms Ev,Gv (no imm). 0F BA is the Ev,Ib group form (below).
		T(0xA3,"BT",  true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		T(0xAB,"BTS", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		T(0xB3,"BTR", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		T(0xBB,"BTC", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);

		T(0xAF,"IMUL", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);

		// CMPXCHG / XADD (lock-prefixable read-modify-write)
		T(0xB0,"CMPXCHG", true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		T(0xB1,"CMPXCHG", true, OP(E,v,"",""), OP(G,v,"",""), NOP_);
		T(0xC0,"XADD",    true, OP(E,b,"",""), OP(G,b,"",""), NOP_);
		T(0xC1,"XADD",    true, OP(E,v,"",""), OP(G,v,"",""), NOP_);

		// MOVZX / MOVSX  Gv, Eb/Ew
		T(0xB6,"MOVZX", true, OP(G,v,"",""), OP(E,b,"",""), NOP_);
		T(0xB7,"MOVZX", true, OP(G,v,"",""), OP(E,w,"",""), NOP_);
		T(0xBE,"MOVSX", true, OP(G,v,"",""), OP(E,b,"",""), NOP_);
		T(0xBF,"MOVSX", true, OP(G,v,"",""), OP(E,w,"",""), NOP_);

		// BSF / BSR  Gv, Ev
		T(0xBC,"BSF", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);
		T(0xBD,"BSR", true, OP(G,v,"",""), OP(E,v,"",""), NOP_);

		// Group 8: BT/BTS/BTR/BTC  Ev, Ib  (0F BA) - ModRM + imm8, name from ModRM.reg
		TG(0xBA,"GRP8", true, OP(E,v,"",""), OP(I,b,"",""), NOP_, 8);

		// BSWAP +r  (0F C8-CF) - register in low 3 opcode bits, no ModRM/immediate
		T(0xC8,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xC9,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCA,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCB,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCC,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCD,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCE,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);
		T(0xCF,"BSWAP", false, OP(Z,v,"",""), NOP_, NOP_);

#undef NOP_
#undef OP
#undef s
#undef a
#undef TG
#undef T
#undef T16
		return n;
	}

	static const std::array<Instruction::OpcodeInfo, 256>& twoByteTable() {
		static constexpr std::array<Instruction::OpcodeInfo, 256> t = buildTwoByteOpcodes();
		return t;
	}

	// 0F BA group 8: /4 BT /5 BTS /6 BTR /7 BTC (/0../3 illegal). Names only - the
	// operands (Ev, Ib) come from the outer 0F BA row, like one-byte grp1/2.
	static constexpr std::array<Instruction::OpcodeInfo, 8> buildTwoByteGroup8() {
		std::array<Instruction::OpcodeInfo, 8> n{};
#define a(name) static_cast<uint8_t>(ADDRESSING::name)
#define s(name) static_cast<uint8_t>(SIZE::name)
#define OP(mode,sz,val,val16) Instruction::TableOperand{ a(mode), s(sz), false, val, val, val16 }
#define NOP_ OP(None,None,"","")
#define GT(reg,text) n[reg] = Instruction::OpcodeInfo{text,"", true, NOP_, NOP_, NOP_, 8}
		GT(4,"BT"); GT(5,"BTS"); GT(6,"BTR"); GT(7,"BTC");
#undef GT
#undef NOP_
#undef OP
#undef s
#undef a
		return n;
	}

	static const std::array<Instruction::OpcodeInfo, 8>& twoByteGroup8Table() {
		static constexpr std::array<Instruction::OpcodeInfo, 8> t = buildTwoByteGroup8();
		return t;
	}

	static bool isTwoByteGroup(uint32_t op2) { return twoByteTable()[op2].groupNo > 0; }

	static const std::array<Instruction::OpcodeInfo, 8>& twoByteGroupTableOf(uint32_t op2) {
		switch (twoByteTable()[op2].groupNo) {
		case 8: return twoByteGroup8Table();
		default: throw std::runtime_error("Two-byte opcode is not an extension group..");
		}
	}

	// Same contract as resolvedInfo(), for the 0F map: a plain row for a non-group
	// opcode, or that row merged with the group entry ModRM.reg selects. Takes the
	// whole ModRM byte to match resolvedInfo()'s shape; the 0F map has no x87, so it
	// just extracts reg. Go through this from both the byte-eater and the printer so
	// they agree on length.
	static Instruction::OpcodeInfo twoByteResolvedInfo(uint32_t op2, uint8_t modrm) {
		const uint8_t reg = (modrm >> 3) & 0x07;
		const Instruction::OpcodeInfo& outer = twoByteTable()[op2];
		if (outer.groupNo <= 0) return outer;

		const Instruction::OpcodeInfo& entry = twoByteGroupTableOf(op2)[reg & 0x07];
		if (entry.text.empty()) return entry;   // illegal /reg

		constexpr uint8_t none   = static_cast<uint8_t>(SIZE::None);
		constexpr uint8_t noMode = static_cast<uint8_t>(ADDRESSING::None);

		Instruction::OpcodeInfo info = outer;
		info.text   = entry.text;
		info.text16 = entry.text16;

		const uint8_t opSize = (outer.op[0].size != none) ? outer.op[0].size : entry.op[0].size;
		auto take = [&](const Instruction::TableOperand& e, Instruction::TableOperand& dst) {
			if (e.addressingMode == noMode) return;
			dst.addressingMode = e.addressingMode;
			dst.size    = (e.size != none) ? e.size : opSize;
			dst.forcedSize = e.forcedSize;
				dst.value64 = e.value64;
				dst.value32 = e.value32;
			dst.value16 = e.value16;
		};
		take(entry.op[0], info.op[0]);
		take(entry.op[1], info.op[1]);
		take(entry.op[2], info.op[2]);
		return info;
	}


//   E=modrm r/m, G=modrm reg, I=imm, J=rel offset, O=moffs, S=seg reg, M=memory,
//   A=far ptr, Z=register in low 3 bits of opcode (+r), AL/eAX/DX/CL/One=implicit
//   ST=x87 operand whose text is baked into value (ST / ST(i) / AX): the renderer's
//   value short-circuit prints it verbatim, so no operand-switch case is needed - the
//   mode only has to be non-None (so it joins) and non-immediate (so it eats no bytes).
enum class ADDRESSING : uint8_t {
	None, E, G, I, J, O, S, M, A, Z, AL, eAX, DX, CL, One,
	eCX, eDX, eBX, eSP, eBP, eSI, eDI,
	ES, CS, SS, DS,
	X, Y, F,
	DL, BL, AH, CH, DH, BH,
	ST
};
enum class SIZE : uint8_t { None, b, w, v, z, p, a, bs };   // b=8 w=16 v=16/32 z=imm16/32 p=far a=bound; bs=imm8 sign-extended to operand size (83/6B/6A)

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

	// ---segment override

	CS = 0x2E,
	SS = 0x36,
	DS = 0x3E,
	ES = 0X26,
	FS = 0x64,
	GS = 0x65,

	// ---

	HLT = 0xF4,
	CMC = 0xF5,
	INT1 = 0xF1,
	OPSIZE = 0x66,

	// ---branch hints

	NT = 0x2E,
	T = 0x3E,

	// ---

	ADDRSIZE = 0x67


};

enum class OPCODE : uint32_t { // value is orientative
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
