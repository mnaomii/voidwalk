#include "arch/x86_64/x86_64_mnemonic.hpp"

#include <stdexcept>

// Definitions of the x86/x86-64 opcode tables declared in x86_64_mnemonic.hpp.
//
// Each accessor returns a reference to a function-local `static constexpr` array,
// so every table is materialised at compile time into .rodata with no run-time
// initialisation and no guard variable.

namespace voidwalk {
namespace {

// Aliases so the table macros below can keep saying OPCODE::MOV rather than
// x86_64_Mnemonic::OPCODE::MOV.
using ADDRESSING = x86_64_Mnemonic::ADDRESSING;
using SIZE       = x86_64_Mnemonic::SIZE;
using REGISTER   = x86_64_Mnemonic::REGISTER;
using OPCODE     = x86_64_Mnemonic::OPCODE;
using Prefix     = x86_64_Mnemonic::Prefix;
using OpcodeInfo   = x86_64_Mnemonic::OpcodeInfo;
using TableOperand = x86_64_Mnemonic::TableOperand;

constexpr std::array<std::string_view, 256> buildPrefixes() {
	std::array<std::string_view, 256> table{};

#define P(name, text) table[static_cast<uint16_t>(Prefix::name)] = text

	P(LOCK, "LOCK"); P(REPNE, "REPNE"); P(REP, "REP");
	P(CS, "CS:"); P(SS, "SS:"); P(DS, "DS:"); P(ES, "ES:"); P(FS, "FS:"); P(GS, "GS:");
	P(OPSIZE, "OPSIZE");
	P(ADDRSIZE, "ADDRSIZE");

#undef P
	return table;
}

constexpr std::array<OpcodeInfo, 256> buildOpcodes() {

	std::array<OpcodeInfo, 256> table{};

	// A row is: mnemonic (32-bit name), then op1/op2/op3, then the group number.
	//   P    - plain opcode, no group.
	//   PG   - plain opcode that is an extension group (group number last).
	//   P16  - the mnemonic itself flips with the operand size (32-bit name, then 16-bit name).
	//   PG16 - the general form both build on.
	// Each operand is OP(addressingMode, size, value), or NOP_ for an absent one.
	// value carries an operand named outright ("AL", "DX"); "" means "decode it from the
	// bytes" or, for eAX-class and X/Y operands, "name it from the operand / address width".
#define PG16(name,text,text16, hasRM, op1, op2, op3, group) table[static_cast<uint32_t>(OPCODE::name)] = OpcodeInfo{text,text16, hasRM, op1, op2, op3, group}
#define PG(name,text, hasRM, op1, op2, op3, group) table[static_cast<uint32_t>(OPCODE::name)] = OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define P16(name,text,text16, hasRM, op1, op2, op3) table[static_cast<uint32_t>(OPCODE::name)] = OpcodeInfo{text,text16, hasRM, op1, op2, op3, -1}
#define P(name,text, hasRM, op1, op2, op3) table[static_cast<uint32_t>(OPCODE::name)] = OpcodeInfo{text,"", hasRM, op1, op2, op3, -1}
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")

	P(ADD_EbGb, "ADD", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(ADD_EvGv, "ADD", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(ADD_GbEb, "ADD", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(ADD_GvEv, "ADD", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(ADD_ALIb, "ADD", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(ADD_eAXIv, "ADD", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(PUSH_ES, "PUSH", false, OP(ES,None,"ES"), NOP_, NOP_);
	P(POP_ES, "POP", false, OP(ES,None,"ES"), NOP_, NOP_);

	P(OR_EbGb, "OR", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(OR_EvGv, "OR", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(OR_GbEb, "OR", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(OR_GvEv, "OR", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(OR_ALIb, "OR", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(OR_eAXIv, "OR", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(PUSH_CS, "PUSH", false, OP(CS,None,"CS"), NOP_, NOP_);
	P(TWOBYTE, "2BYTE", false, NOP_, NOP_, NOP_);

	P(ADC_EbGb, "ADC", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(ADC_EvGv, "ADC", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(ADC_GbEb, "ADC", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(ADC_GvEv, "ADC", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(ADC_ALIb, "ADC", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(ADC_eAXIv, "ADC", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(PUSH_SS, "PUSH", false, OP(SS,None,"SS"), NOP_, NOP_);
	P(POP_SS, "POP", false, OP(SS,None,"SS"), NOP_, NOP_);

	P(SBB_EbGb, "SBB", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(SBB_EvGv, "SBB", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(SBB_GbEb, "SBB", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(SBB_GvEv, "SBB", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(SBB_ALIb, "SBB", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(SBB_eAXIv, "SBB", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(PUSH_DS, "PUSH", false, OP(DS,None,"DS"), NOP_, NOP_);
	P(POP_DS, "POP", false, OP(DS,None,"DS"), NOP_, NOP_);

	P(AND_EbGb, "AND", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(AND_EvGv, "AND", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(AND_GbEb, "AND", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(AND_GvEv, "AND", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(AND_ALIb, "AND", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(AND_eAXIv, "AND", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(ES, "ES", false, NOP_, NOP_, NOP_);
	P(DAA, "DAA", false, NOP_, NOP_, NOP_);

	P(SUB_EbGb, "SUB", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(SUB_EvGv, "SUB", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(SUB_GbEb, "SUB", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(SUB_GvEv, "SUB", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(SUB_ALIb, "SUB", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(SUB_eAXIv, "SUB", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(CS, "CS", false, NOP_, NOP_, NOP_);
	P(DAS, "DAS", false, NOP_, NOP_, NOP_);

	P(XOR_EbGb, "XOR", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(XOR_EvGv, "XOR", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(XOR_GbEb, "XOR", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(XOR_GvEv, "XOR", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(XOR_ALIb, "XOR", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(XOR_eAXIv, "XOR", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(SS, "SS", false, NOP_, NOP_, NOP_);
	P(AAA, "AAA", false, NOP_, NOP_, NOP_);

	P(CMP_EbGb, "CMP", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(CMP_EvGv, "CMP", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(CMP_GbEb, "CMP", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(CMP_GvEv, "CMP", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(CMP_ALIb, "CMP", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(CMP_eAXIv, "CMP", false, OP(eAX,v,""), OP(I,z,""), NOP_);

	P(DS, "DS", false, NOP_, NOP_, NOP_);
	P(AAS, "AAS", false, NOP_, NOP_, NOP_);

	P(INC_eAX, "INC", false, OP(eAX,v,""), NOP_, NOP_);
	P(INC_eCX, "INC", false, OP(eCX,v,""), NOP_, NOP_);
	P(INC_eDX, "INC", false, OP(eDX,v,""), NOP_, NOP_);
	P(INC_eBX, "INC", false, OP(eBX,v,""), NOP_, NOP_);
	P(INC_eSP, "INC", false, OP(eSP,v,""), NOP_, NOP_);
	P(INC_eBP, "INC", false, OP(eBP,v,""), NOP_, NOP_);
	P(INC_eSI, "INC", false, OP(eSI,v,""), NOP_, NOP_);
	P(INC_eDI, "INC", false, OP(eDI,v,""), NOP_, NOP_);

	P(DEC_eAX, "DEC", false, OP(eAX,v,""), NOP_, NOP_);
	P(DEC_eCX, "DEC", false, OP(eCX,v,""), NOP_, NOP_);
	P(DEC_eDX, "DEC", false, OP(eDX,v,""), NOP_, NOP_);
	P(DEC_eBX, "DEC", false, OP(eBX,v,""), NOP_, NOP_);
	P(DEC_eSP, "DEC", false, OP(eSP,v,""), NOP_, NOP_);
	P(DEC_eBP, "DEC", false, OP(eBP,v,""), NOP_, NOP_);
	P(DEC_eSI, "DEC", false, OP(eSI,v,""), NOP_, NOP_);
	P(DEC_eDI, "DEC", false, OP(eDI,v,""), NOP_, NOP_);

	// 50-5F PUSH/POP r: the register is the opcode's low 3 bits (+r). Z decodes it from
	// the byte (and folds in REX.B for R8-R15); no baked names. PUSH/POP are d64.
	P(PUSH_eAX, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eCX, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eDX, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eBX, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eSP, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eBP, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eSI, "PUSH", false, OP(Z,v,""), NOP_, NOP_);
	P(PUSH_eDI, "PUSH", false, OP(Z,v,""), NOP_, NOP_);

	P(POP_eAX, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eCX, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eDX, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eBX, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eSP, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eBP, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eSI, "POP", false, OP(Z,v,""), NOP_, NOP_);
	P(POP_eDI, "POP", false, OP(Z,v,""), NOP_, NOP_);

	P16(PUSHA, "PUSHAD", "PUSHA", false, NOP_, NOP_, NOP_);
	P16(POPA, "POPAD", "POPA", false, NOP_, NOP_, NOP_);

	P(BOUND_GvMa, "BOUND", true, OP(G,v,""), OP(M,a,""), NOP_);
	P(ARPL_EwGw, "ARPL", true, OP(E,w,""), OP(G,w,""), NOP_);
	P(FS, "FS", false, NOP_, NOP_, NOP_);
	P(GS, "GS", false, NOP_, NOP_, NOP_);
	P(OPSIZE, "OPSIZE", false, NOP_, NOP_, NOP_);
	P(ADSIZE, "ADSIZE", false, NOP_, NOP_, NOP_);

	P(PUSH_Iv, "PUSH", false, OP(I,z,""), NOP_, NOP_);
	P(IMUL_GvEvIv, "IMUL", true, OP(G,v,""), OP(E,v,""), OP(I,z,""));
	P(PUSH_Ib, "PUSH", false, OP(I,bs,""), NOP_, NOP_);            // imm8 sign-extended to opsize
	P(IMUL_GvEvIb, "IMUL", true, OP(G,v,""), OP(E,v,""), OP(I,bs,""));   // imm8 sign-extended

	P(INSB_YbDX, "INSB", false, OP(Y,b,""), OP(DX,None,"DX"), NOP_);
	P16(INSW_YzDX, "INSD", "INSW", false, OP(Y,z,""), OP(DX,None,"DX"), NOP_);
	P(OUTSB_DXXb, "OUTSB", false, OP(DX,None,"DX"), OP(X,b,""), NOP_);
	P16(OUTSW_DXXv, "OUTSD", "OUTSW", false, OP(DX,None,"DX"), OP(X,v,""), NOP_);

	P(JO, "JO", false, OP(J,b,""), NOP_, NOP_);
	P(JNO, "JNO", false, OP(J,b,""), NOP_, NOP_);
	P(JB, "JB", false, OP(J,b,""), NOP_, NOP_);
	P(JNB, "JNB", false, OP(J,b,""), NOP_, NOP_);
	P(JZ, "JZ", false, OP(J,b,""), NOP_, NOP_);
	P(JNZ, "JNZ", false, OP(J,b,""), NOP_, NOP_);
	P(JBE, "JBE", false, OP(J,b,""), NOP_, NOP_);
	P(JA, "JA", false, OP(J,b,""), NOP_, NOP_);
	P(JS, "JS", false, OP(J,b,""), NOP_, NOP_);
	P(JNS, "JNS", false, OP(J,b,""), NOP_, NOP_);
	P(JP, "JP", false, OP(J,b,""), NOP_, NOP_);
	P(JNP, "JNP", false, OP(J,b,""), NOP_, NOP_);
	P(JL, "JL", false, OP(J,b,""), NOP_, NOP_);
	P(JNL, "JNL", false, OP(J,b,""), NOP_, NOP_);
	P(JLE, "JLE", false, OP(J,b,""), NOP_, NOP_);
	P(JNLE, "JNLE", false, OP(J,b,""), NOP_, NOP_);

	// Group 1 (0x80-0x83): ADD/OR/ADC/SBB/AND/SUB/XOR/CMP, selected by ModRM.reg
	PG(GRP1_EbIb, "GRP1", true, OP(E,b,""), OP(I,b,""), NOP_, 1);
	PG(GRP1_EvIz, "GRP1", true, OP(E,v,""), OP(I,z,""), NOP_, 1);
	PG(GRP1_EbIb2, "GRP1", true, OP(E,b,""), OP(I,b,""), NOP_, 1);
	PG(GRP1_EvIb, "GRP1", true, OP(E,v,""), OP(I,bs,""), NOP_, 1);   // 83 /r ib: imm8 sign-extended to Ev width

	P(TEST_EbGb, "TEST", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(TEST_EvGv, "TEST", true, OP(E,v,""), OP(G,v,""), NOP_);

	P(XCHG_EbGb, "XCHG", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(XCHG_EvGv, "XCHG", true, OP(E,v,""), OP(G,v,""), NOP_);

	P(MOV_EbGb, "MOV", true, OP(E,b,""), OP(G,b,""), NOP_);
	P(MOV_EvGv, "MOV", true, OP(E,v,""), OP(G,v,""), NOP_);
	P(MOV_GbEb, "MOV", true, OP(G,b,""), OP(E,b,""), NOP_);
	P(MOV_GvEv, "MOV", true, OP(G,v,""), OP(E,v,""), NOP_);
	P(MOV_EwSw, "MOV", true, OP(E,w,""), OP(S,w,""), NOP_);

	P(LEA_GvM, "LEA", true, OP(G,v,""), OP(M,None,""), NOP_);

	P(MOV_SwEw, "MOV", true, OP(S,w,""), OP(E,w,""), NOP_);

	P(POP_Ev, "POP", true, OP(E,v,""), NOP_, NOP_);

	P(NOP, "NOP", false, NOP_, NOP_, NOP_);
	// 90-97 XCHG eAX,r: op0 is the accumulator, op1 the +r register decoded from the
	// opcode via Z. Not d64, so 64-bit only under REX.W.
	P(XCHG_eAXeCX, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeDX, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeBX, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeSP, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeBP, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeSI, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);
	P(XCHG_eAXeDI, "XCHG", false, OP(eAX,v,""), OP(Z,v,""), NOP_);

	// 0x98/0x99 name their operands in silicon and take no operand slots, so the operand
	// size can only show up in the mnemonic: CWDE sign-extends AX into EAX, CBW AL into AX;
	// CDQ sign-extends EAX into EDX:EAX, CWD AX into DX:AX.
	P16(CBW, "CWDE", "CBW", false, NOP_, NOP_, NOP_);
	P16(CWD, "CDQ", "CWD", false, NOP_, NOP_, NOP_);
	P(CALL_Ap, "CALL", false, OP(A,p,""), NOP_, NOP_);
	P(FWAIT, "FWAIT", false, NOP_, NOP_, NOP_);
	P16(PUSHF_Fv, "PUSHFD", "PUSHF", false, OP(F,v,""), NOP_, NOP_);
	P16(POPF_Fv, "POPFD", "POPF", false, OP(F,v,""), NOP_, NOP_);
	P(SAHF, "SAHF", false, NOP_, NOP_, NOP_);
	P(LAHF, "LAHF", false, NOP_, NOP_, NOP_);

	// MOV to/from accumulator, direct memory offset (moffs). The offset is a memory
	// reference, not an immediate - value is "" so the decoder builds [addr].
	P(MOV_ALOb, "MOV", false, OP(AL,b,"AL"), OP(O,b,""), NOP_);
	P(MOV_eAXOv, "MOV", false, OP(eAX,v,""), OP(O,v,""), NOP_);
	P(MOV_ObAL, "MOV", false, OP(O,b,""), OP(AL,b,"AL"), NOP_);
	P(MOV_OveAX, "MOV", false, OP(O,v,""), OP(eAX,v,""), NOP_);

	// MOVS copies [rSI] into [rDI]: the destination Y prints first. CMPS keeps X, Y.
	P(MOVSB_XbYb, "MOVSB", false, OP(Y,b,""), OP(X,b,""), NOP_);
	// Intel names the dword string ops MOVSD/CMPSD, which collide with the SSE2 scalar-double
	// MOVSD/CMPSD (F2 0F 10, F2 0F C2). Same spelling, unrelated instructions - the operands
	// tell them apart. AT&T sidesteps the clash by spelling these movsl/cmpsl instead.
	P16(MOVSW_XvYv, "MOVSD", "MOVSW", false, OP(Y,v,""), OP(X,v,""), NOP_);
	P(CMPSB_XbYb, "CMPSB", false, OP(X,b,""), OP(Y,b,""), NOP_);
	P16(CMPSW_XvYv, "CMPSD", "CMPSW", false, OP(X,v,""), OP(Y,v,""), NOP_);
	P(TEST_ALIb, "TEST", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(TEST_eAXIv, "TEST", false, OP(eAX,v,""), OP(I,z,""), NOP_);
	P(STOSB_YbAL, "STOSB", false, OP(Y,b,""), OP(AL,b,"AL"), NOP_);
	P16(STOSW_YveAX, "STOSD", "STOSW", false, OP(Y,v,""), OP(eAX,v,""), NOP_);
	P(LODSB_ALXb, "LODSB", false, OP(AL,b,"AL"), OP(X,b,""), NOP_);
	P16(LODSW_eAXXv, "LODSD", "LODSW", false, OP(eAX,v,""), OP(X,v,""), NOP_);
	P(SCASB_ALYb, "SCASB", false, OP(AL,b,"AL"), OP(Y,b,""), NOP_);
	P16(SCASW_eAXYv, "SCASD", "SCASW", false, OP(eAX,v,""), OP(Y,v,""), NOP_);

	P(MOV_ALIb, "MOV", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(MOV_CLIb, "MOV", false, OP(CL,b,"CL"), OP(I,b,""), NOP_);
	P(MOV_DLIb, "MOV", false, OP(DL,b,"DL"), OP(I,b,""), NOP_);
	P(MOV_BLIb, "MOV", false, OP(BL,b,"BL"), OP(I,b,""), NOP_);
	P(MOV_AHIb, "MOV", false, OP(AH,b,"AH"), OP(I,b,""), NOP_);
	P(MOV_CHIb, "MOV", false, OP(CH,b,"CH"), OP(I,b,""), NOP_);
	P(MOV_DHIb, "MOV", false, OP(DH,b,"DH"), OP(I,b,""), NOP_);
	P(MOV_BHIb, "MOV", false, OP(BH,b,"BH"), OP(I,b,""), NOP_);

	// B8-BF MOV r,imm: op0 is the +r register (Z); imm is v (imm64 under REX.W -> movabs).
	P(MOV_eAXIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eCXIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eDXIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eBXIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eSPIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eBPIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eSIIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);
	P(MOV_eDIIv, "MOV", false, OP(Z,v,""), OP(I,v,""), NOP_);

	// Group 2 (0xC0/0xC1, 0xD0-0xD3): ROL/ROR/RCL/RCR/SHL/SHR/SAL/SAR, selected by ModRM.reg
	PG(GRP2_EbIb, "GRP2", true, OP(E,b,""), OP(I,b,""), NOP_, 2);
	PG(GRP2_EvIb, "GRP2", true, OP(E,v,""), OP(I,b,""), NOP_, 2);
	P(RET_Iw, "RET", false, OP(I,w,""), NOP_, NOP_);
	P(RET, "RET", false, NOP_, NOP_, NOP_);
	P(LES_GvMp, "LES", true, OP(G,v,""), OP(M,p,""), NOP_);
	P(LDS_GvMp, "LDS", true, OP(G,v,""), OP(M,p,""), NOP_);
	P(MOV_EbIb, "MOV", true, OP(E,b,""), OP(I,b,""), NOP_);
	P(MOV_EvIv, "MOV", true, OP(E,v,""), OP(I,z,""), NOP_);
	P(ENTER_IwIb, "ENTER", false, OP(I,w,""), OP(I,b,""), NOP_);
	P(LEAVE, "LEAVE", false, NOP_, NOP_, NOP_);
	P(RETF_Iw, "RETF", false, OP(I,w,""), NOP_, NOP_);
	P(RETF, "RETF", false, NOP_, NOP_, NOP_);
	P(INT3, "INT3", false, NOP_, NOP_, NOP_);
	P(INT_Ib, "INT", false, OP(I,b,""), NOP_, NOP_);
	P(INTO, "INTO", false, NOP_, NOP_, NOP_);
	P16(IRET, "IRETD", "IRET", false, NOP_, NOP_, NOP_);

	PG(GRP2_Eb1, "GRP2", true, OP(E,b,""), OP(One,None,"1"), NOP_, 2);
	PG(GRP2_Ev1, "GRP2", true, OP(E,v,""), OP(One,None,"1"), NOP_, 2);
	PG(GRP2_EbCL, "GRP2", true, OP(E,b,""), OP(CL,None,"CL"), NOP_, 2);
	PG(GRP2_EvCL, "GRP2", true, OP(E,v,""), OP(CL,None,"CL"), NOP_, 2);

	P(AAM_Ib, "AAM", false, OP(I,b,""), NOP_, NOP_);
	P(AAD_Ib, "AAD", false, OP(I,b,""), NOP_, NOP_);
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

	P(LOOPNZ_Jb, "LOOPNZ", false, OP(J,b,""), NOP_, NOP_);
	P(LOOPZ_Jb, "LOOPZ", false, OP(J,b,""), NOP_, NOP_);
	P(LOOP_Jb, "LOOP", false, OP(J,b,""), NOP_, NOP_);
	P(JeCXZ_Jb, "JECXZ", false, OP(J,b,""), NOP_, NOP_);

	P(IN_ALIb, "IN", false, OP(AL,b,"AL"), OP(I,b,""), NOP_);
	P(IN_eAXIb, "IN", false, OP(eAX,z,""), OP(I,b,""), NOP_);
	P(OUT_IbAL, "OUT", false, OP(I,b,""), OP(AL,b,"AL"), NOP_);
	P(OUT_IbeAX, "OUT", false, OP(I,b,""), OP(eAX,z,""), NOP_);

	P(CALL_Jv, "CALL", false, OP(J,v,""), NOP_, NOP_);
	P(JMP_Jv, "JMP", false, OP(J,v,""), NOP_, NOP_);
	P(JMP_Ap, "JMP", false, OP(A,p,""), NOP_, NOP_);
	P(JMP_Jb, "JMP", false, OP(J,b,""), NOP_, NOP_);

	P(IN_ALDX, "IN", false, OP(AL,b,"AL"), OP(DX,None,"DX"), NOP_);
	P(IN_eAXDX, "IN", false, OP(eAX,z,""), OP(DX,None,"DX"), NOP_);
	P(OUT_DXAL, "OUT", false, OP(DX,None,"DX"), OP(AL,b,"AL"), NOP_);
	P(OUT_DXeAX, "OUT", false, OP(DX,None,"DX"), OP(eAX,z,""), NOP_);

	P(LOCK, "LOCK", false, NOP_, NOP_, NOP_);
	P(INT1, "INT1", false, NOP_, NOP_, NOP_);
	P(REPNE, "REPNE", false, NOP_, NOP_, NOP_);
	P(REP, "REP", false, NOP_, NOP_, NOP_);

	P(HLT, "HLT", false, NOP_, NOP_, NOP_);
	P(CMC, "CMC", false, NOP_, NOP_, NOP_);

	// Group 3 (0xF6/0xF7): TEST/NOT/NEG/MUL/IMUL/DIV/IDIV, selected by ModRM.reg
	PG(GRP3_Eb, "GRP3", true, OP(E,b,""), NOP_, NOP_, 3);
	PG(GRP3_Ev, "GRP3", true, OP(E,v,""), NOP_, NOP_, 3);

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
#define INV(name) table[static_cast<uint32_t>(OPCODE::name)].isInvalid = true
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

	// --- 64-bit mnemonics (text64): the width-in-the-name family. Mirror of the text16
	// entries a few rows up. Renderer picks text64 at a 64-bit operand size, text16 at 16.
#define T64(name,name64) table[static_cast<uint32_t>(OPCODE::name)].text64 = name64
	T64(CBW, "CDQE");         T64(CWD, "CQO");
	T64(PUSHF_Fv, "PUSHFQ");  T64(POPF_Fv, "POPFQ");
	T64(MOVSW_XvYv, "MOVSQ"); T64(CMPSW_XvYv, "CMPSQ");
	T64(STOSW_YveAX, "STOSQ");T64(LODSW_eAXXv, "LODSQ"); T64(SCASW_eAXYv, "SCASQ");
	T64(IRET, "IRETQ");
	// INSD/OUTSD (0x6D/0x6F) have no 64-bit form - I/O ports max out at 32-bit, no text64.
#undef T64

	// --- long-mode default operand size (field: default64). Ignored by a 32-bit decode.
	//   d64: defaults to 64, but 0x66 drops it to 16.   f64: forced 64, 0x66 ignored.
#define D64(name) table[static_cast<uint32_t>(OPCODE::name)].default64 = OpcodeInfo::Default64::d64
#define F64(name) table[static_cast<uint32_t>(OPCODE::name)].default64 = OpcodeInfo::Default64::f64
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

	return table;

}

#define G(reg,text, hasRM, op1, op2, op3, group) table[reg] = OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define GT(reg,text,group) G(reg, text, true, NOP_, NOP_, NOP_, group)
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")

constexpr std::array<OpcodeInfo, 8> buildGroup1() {
	std::array<OpcodeInfo, 8> table{};
	GT(0, "ADD", 1);
	GT(1, "OR", 1);
	GT(2, "ADC", 1);
	GT(3, "SBB", 1);
	GT(4, "AND", 1);
	GT(5, "SUB", 1);
	GT(6, "XOR", 1);
	GT(7, "CMP", 1);
	return table;
}

constexpr std::array<OpcodeInfo, 8> buildGroup2() {
	std::array<OpcodeInfo, 8> table{};
	GT(0, "ROL", 2);
	GT(1, "ROR", 2);
	GT(2, "RCL", 2);
	GT(3, "RCR", 2);
	GT(4, "SHL", 2);
	GT(5, "SHR", 2);
	GT(6, "SAL", 2);
	GT(7, "SAR", 2);
	return table;
}

constexpr std::array<OpcodeInfo, 8> buildGroup3() {
	std::array<OpcodeInfo, 8> table{};
	G(0, "TEST", true, OP(E,None,""), OP(I,None,""), NOP_, 3);
	G(1, "TEST", true, OP(E,None,""), OP(I,None,""), NOP_, 3);
	G(2, "NOT", true, OP(E,None,""), NOP_, NOP_, 3);
	G(3, "NEG", true, OP(E,None,""), NOP_, NOP_, 3);
	G(4, "MUL", true, OP(E,None,""), NOP_, NOP_, 3);
	G(5, "IMUL", true, OP(E,None,""), NOP_, NOP_, 3);
	G(6, "DIV", true, OP(E,None,""), NOP_, NOP_, 3);
	G(7, "IDIV", true, OP(E,None,""), NOP_, NOP_, 3);
	return table;
}

constexpr std::array<OpcodeInfo, 8> buildGroup4() {
	std::array<OpcodeInfo, 8> table{};
	G(0, "INC", true, OP(E,b,""), NOP_, NOP_, 4);
	G(1, "DEC", true, OP(E,b,""), NOP_, NOP_, 4);
	return table;
}

constexpr std::array<OpcodeInfo, 8> buildGroup5() {
	std::array<OpcodeInfo, 8> table{};
	G(0, "INC", true, OP(E,v,""), NOP_, NOP_, 5);
	G(1, "DEC", true, OP(E,v,""), NOP_, NOP_, 5);
	G(2, "CALL", true, OP(E,v,""), NOP_, NOP_, 5);
	G(3, "CALLF", true, OP(M,p,""), NOP_, NOP_, 5);
	G(4, "JMP", true, OP(E,v,""), NOP_, NOP_, 5);
	G(5, "JMPF", true, OP(M,p,""), NOP_, NOP_, 5);
	G(6, "PUSH", true, OP(E,v,""), NOP_, NOP_, 5);
	// Long-mode defaults (see the D64/F64 block in buildOpcodes); resolvedInfo() carries
	// them over the outer FF row.
	table[2].default64 = table[4].default64 = OpcodeInfo::Default64::f64;
	table[6].default64 = OpcodeInfo::Default64::d64;
	return table;
}

#undef NOP_
#undef OP
#undef s
#undef a
#undef GT
#undef G

constexpr std::array<OpcodeInfo, 64> buildX87Mem() {
	std::array<OpcodeInfo, 64> table{};
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define M_ TableOperand{ a(M), s(None), "" }
#define NO TableOperand{ a(None), s(None), "" }
#define FM(idx,text) table[idx] = OpcodeInfo{ text, "", true, M_, NO, NO, -1 }
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
	return table;
}

constexpr std::array<OpcodeInfo, 512> buildX87Reg() {
	std::array<OpcodeInfo, 512> table{};
	constexpr std::string_view stackRegisters[8] =
		{ "ST(0)","ST(1)","ST(2)","ST(3)","ST(4)","ST(5)","ST(6)","ST(7)" };
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define STv(v) TableOperand{ a(ST), s(None), v }
#define NO TableOperand{ a(None), s(None), "" }
	// op-relative base + reg row, one entry per rm.
#define ROW2(o,reg,text,p0,p1)  for (uint8_t i=0;i<8;++i) table[(o)*64+(reg)*8+i] = OpcodeInfo{ text, "", true, p0, p1, NO, -1 }
#define ROW1(o,reg,text,p0)     for (uint8_t i=0;i<8;++i) table[(o)*64+(reg)*8+i] = OpcodeInfo{ text, "", true, p0, NO, NO, -1 }
#define ONE(o,reg,rm,text)      table[(o)*64+(reg)*8+(rm)] = OpcodeInfo{ text, "", true, NO, NO, NO, -1 }

	// D8 (o=0): "<op> ST, ST(i)" - reg is the op, same as its memory form.
	ROW2(0,0,"FADD", STv("ST"),STv(stackRegisters[i]));  ROW2(0,1,"FMUL", STv("ST"),STv(stackRegisters[i]));
	ROW1(0,2,"FCOM", STv(stackRegisters[i]));            ROW1(0,3,"FCOMP",STv(stackRegisters[i]));
	ROW2(0,4,"FSUB", STv("ST"),STv(stackRegisters[i]));  ROW2(0,5,"FSUBR",STv("ST"),STv(stackRegisters[i]));
	ROW2(0,6,"FDIV", STv("ST"),STv(stackRegisters[i]));  ROW2(0,7,"FDIVR",STv("ST"),STv(stackRegisters[i]));

	// D9 (o=1): FLD/FXCH ST(i), then the constant/transcendental individuals.
	ROW1(1,0,"FLD", STv(stackRegisters[i]));  ROW1(1,1,"FXCH",STv(stackRegisters[i]));
	ONE(1,2,0,"FNOP");
	ONE(1,4,0,"FCHS"); ONE(1,4,1,"FABS"); ONE(1,4,4,"FTST"); ONE(1,4,5,"FXAM");
	ONE(1,5,0,"FLD1"); ONE(1,5,1,"FLDL2T");ONE(1,5,2,"FLDL2E");ONE(1,5,3,"FLDPI");
	ONE(1,5,4,"FLDLG2");ONE(1,5,5,"FLDLN2");ONE(1,5,6,"FLDZ");
	ONE(1,6,0,"F2XM1");ONE(1,6,1,"FYL2X"); ONE(1,6,2,"FPTAN"); ONE(1,6,3,"FPATAN");
	ONE(1,6,4,"FXTRACT");ONE(1,6,5,"FPREM1");ONE(1,6,6,"FDECSTP");ONE(1,6,7,"FINCSTP");
	ONE(1,7,0,"FPREM");ONE(1,7,1,"FYL2XP1");ONE(1,7,2,"FSQRT");ONE(1,7,3,"FSINCOS");
	ONE(1,7,4,"FRNDINT");ONE(1,7,5,"FSCALE");ONE(1,7,6,"FSIN");ONE(1,7,7,"FCOS");

	// DA (o=2): FCMOVcc ST, ST(i); DA E9 FUCOMPP.
	ROW2(2,0,"FCMOVB", STv("ST"),STv(stackRegisters[i])); ROW2(2,1,"FCMOVE", STv("ST"),STv(stackRegisters[i]));
	ROW2(2,2,"FCMOVBE",STv("ST"),STv(stackRegisters[i])); ROW2(2,3,"FCMOVU", STv("ST"),STv(stackRegisters[i]));
	ONE(2,5,1,"FUCOMPP");

	// DB (o=3): FCMOVNcc ST, ST(i); FNCLEX/FNINIT; FUCOMI/FCOMI ST, ST(i).
	ROW2(3,0,"FCMOVNB", STv("ST"),STv(stackRegisters[i])); ROW2(3,1,"FCMOVNE", STv("ST"),STv(stackRegisters[i]));
	ROW2(3,2,"FCMOVNBE",STv("ST"),STv(stackRegisters[i])); ROW2(3,3,"FCMOVNU", STv("ST"),STv(stackRegisters[i]));
	ONE(3,4,2,"FNCLEX"); ONE(3,4,3,"FNINIT");
	ROW2(3,5,"FUCOMI",STv("ST"),STv(stackRegisters[i]));   ROW2(3,6,"FCOMI", STv("ST"),STv(stackRegisters[i]));

	// DC (o=4): "<op> ST(i), ST" - note reg4/5 and reg6/7 are the reversed forms.
	ROW2(4,0,"FADD", STv(stackRegisters[i]),STv("ST")); ROW2(4,1,"FMUL", STv(stackRegisters[i]),STv("ST"));
	ROW2(4,4,"FSUBR",STv(stackRegisters[i]),STv("ST")); ROW2(4,5,"FSUB", STv(stackRegisters[i]),STv("ST"));
	ROW2(4,6,"FDIVR",STv(stackRegisters[i]),STv("ST")); ROW2(4,7,"FDIV", STv(stackRegisters[i]),STv("ST"));

	// DD (o=5): FFREE / FST / FSTP / FUCOM / FUCOMP ST(i).
	ROW1(5,0,"FFREE",STv(stackRegisters[i]));  ROW1(5,2,"FST",  STv(stackRegisters[i])); ROW1(5,3,"FSTP",STv(stackRegisters[i]));
	ROW1(5,4,"FUCOM",STv(stackRegisters[i]));  ROW1(5,5,"FUCOMP",STv(stackRegisters[i]));

	// DE (o=6): the popping arithmetic "<op>P ST(i), ST"; DE D9 FCOMPP.
	ROW2(6,0,"FADDP", STv(stackRegisters[i]),STv("ST")); ROW2(6,1,"FMULP", STv(stackRegisters[i]),STv("ST"));
	ONE(6,3,1,"FCOMPP");
	ROW2(6,4,"FSUBRP",STv(stackRegisters[i]),STv("ST")); ROW2(6,5,"FSUBP", STv(stackRegisters[i]),STv("ST"));
	ROW2(6,6,"FDIVRP",STv(stackRegisters[i]),STv("ST")); ROW2(6,7,"FDIVP", STv(stackRegisters[i]),STv("ST"));

	// DF (o=7): FNSTSW AX (the one x87 op that names a GP reg); FUCOMIP/FCOMIP ST, ST(i).
	table[7*64+4*8+0] = OpcodeInfo{ "FNSTSW","",true, STv("AX"), NO, NO, -1 };
	ROW2(7,5,"FUCOMIP",STv("ST"),STv(stackRegisters[i])); ROW2(7,6,"FCOMIP",STv("ST"),STv(stackRegisters[i]));

#undef ONE
#undef ROW1
#undef ROW2
#undef NO
#undef STv
#undef s
#undef a
	return table;
}

constexpr std::array<OpcodeInfo, 256> buildTwoByteOpcodes() {
	std::array<OpcodeInfo, 256> table{};

#define T16(idx,text,text16,hasRM,op1,op2,op3) table[idx] = OpcodeInfo{text,text16, hasRM, op1, op2, op3, -1}
#define T(idx,text,hasRM,op1,op2,op3)          table[idx] = OpcodeInfo{text,"", hasRM, op1, op2, op3, -1}
#define TG(idx,text,hasRM,op1,op2,op3,group)   table[idx] = OpcodeInfo{text,"", hasRM, op1, op2, op3, group}
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")

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
	T(0x1F, "NOP", true, OP(E,v,""), NOP_, NOP_);

	// 0F 1E - reserved hint-NOP in the base ISA. Under an F3 prefix with ModRM
	// FA/FB it is ENDBR64/ENDBR32, CET's indirect-branch landing pad, which sits
	// at the head of essentially every function in a CET-enabled binary (254 of
	// them in /bin/ls). This table is keyed by the opcode byte alone and cannot
	// see the mandatory prefix, so the ENDBR name is applied in the renderer;
	// the row here is what gives it its ModRM byte.
	T(0x1E, "NOP", true, OP(E,v,""), NOP_, NOP_);

	// Double-precision shifts: Ev, Gv, Ib / Ev, Gv, CL. The Ib forms carry an
	// immediate, so leaving them unnamed also left them the wrong length.
	T(0xA4,"SHLD", true, OP(E,v,""), OP(G,v,""), OP(I,b,""));
	T(0xA5,"SHLD", true, OP(E,v,""), OP(G,v,""), NOP_);
	T(0xAC,"SHRD", true, OP(E,v,""), OP(G,v,""), OP(I,b,""));
	T(0xAD,"SHRD", true, OP(E,v,""), OP(G,v,""), NOP_);

	// CMOVcc Gv, Ev  (0F 40-4F)
	T(0x40,"CMOVO",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x41,"CMOVNO", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x42,"CMOVB",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x43,"CMOVNB", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x44,"CMOVZ",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x45,"CMOVNZ", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x46,"CMOVBE", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x47,"CMOVA",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x48,"CMOVS",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x49,"CMOVNS", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4A,"CMOVP",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4B,"CMOVNP", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4C,"CMOVL",  true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4D,"CMOVNL", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4E,"CMOVLE", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0x4F,"CMOVNLE",true, OP(G,v,""), OP(E,v,""), NOP_);

	// Jcc rel16/rel32  (0F 80-8F) - no ModRM; z immediate (rel16 under a 0x66)
	T(0x80,"JO",  false, OP(J,z,""), NOP_, NOP_);
	T(0x81,"JNO", false, OP(J,z,""), NOP_, NOP_);
	T(0x82,"JB",  false, OP(J,z,""), NOP_, NOP_);
	T(0x83,"JNB", false, OP(J,z,""), NOP_, NOP_);
	T(0x84,"JZ",  false, OP(J,z,""), NOP_, NOP_);
	T(0x85,"JNZ", false, OP(J,z,""), NOP_, NOP_);
	T(0x86,"JBE", false, OP(J,z,""), NOP_, NOP_);
	T(0x87,"JA",  false, OP(J,z,""), NOP_, NOP_);
	T(0x88,"JS",  false, OP(J,z,""), NOP_, NOP_);
	T(0x89,"JNS", false, OP(J,z,""), NOP_, NOP_);
	T(0x8A,"JP",  false, OP(J,z,""), NOP_, NOP_);
	T(0x8B,"JNP", false, OP(J,z,""), NOP_, NOP_);
	T(0x8C,"JL",  false, OP(J,z,""), NOP_, NOP_);
	T(0x8D,"JNL", false, OP(J,z,""), NOP_, NOP_);
	T(0x8E,"JLE", false, OP(J,z,""), NOP_, NOP_);
	T(0x8F,"JNLE",false, OP(J,z,""), NOP_, NOP_);

	// SETcc Eb  (0F 90-9F) - ModRM, no immediate
	T(0x90,"SETO",  true, OP(E,b,""), NOP_, NOP_);
	T(0x91,"SETNO", true, OP(E,b,""), NOP_, NOP_);
	T(0x92,"SETB",  true, OP(E,b,""), NOP_, NOP_);
	T(0x93,"SETNB", true, OP(E,b,""), NOP_, NOP_);
	T(0x94,"SETZ",  true, OP(E,b,""), NOP_, NOP_);
	T(0x95,"SETNZ", true, OP(E,b,""), NOP_, NOP_);
	T(0x96,"SETBE", true, OP(E,b,""), NOP_, NOP_);
	T(0x97,"SETA",  true, OP(E,b,""), NOP_, NOP_);
	T(0x98,"SETS",  true, OP(E,b,""), NOP_, NOP_);
	T(0x99,"SETNS", true, OP(E,b,""), NOP_, NOP_);
	T(0x9A,"SETP",  true, OP(E,b,""), NOP_, NOP_);
	T(0x9B,"SETNP", true, OP(E,b,""), NOP_, NOP_);
	T(0x9C,"SETL",  true, OP(E,b,""), NOP_, NOP_);
	T(0x9D,"SETNL", true, OP(E,b,""), NOP_, NOP_);
	T(0x9E,"SETLE", true, OP(E,b,""), NOP_, NOP_);
	T(0x9F,"SETNLE",true, OP(E,b,""), NOP_, NOP_);

	// Bit tests: reg forms Ev,Gv (no imm). 0F BA is the Ev,Ib group form (below).
	T(0xA3,"BT",  true, OP(E,v,""), OP(G,v,""), NOP_);
	T(0xAB,"BTS", true, OP(E,v,""), OP(G,v,""), NOP_);
	T(0xB3,"BTR", true, OP(E,v,""), OP(G,v,""), NOP_);
	T(0xBB,"BTC", true, OP(E,v,""), OP(G,v,""), NOP_);

	T(0xAF,"IMUL", true, OP(G,v,""), OP(E,v,""), NOP_);

	// CMPXCHG / XADD (lock-prefixable read-modify-write)
	T(0xB0,"CMPXCHG", true, OP(E,b,""), OP(G,b,""), NOP_);
	T(0xB1,"CMPXCHG", true, OP(E,v,""), OP(G,v,""), NOP_);
	T(0xC0,"XADD",    true, OP(E,b,""), OP(G,b,""), NOP_);
	T(0xC1,"XADD",    true, OP(E,v,""), OP(G,v,""), NOP_);

	// MOVZX / MOVSX  Gv, Eb/Ew
	T(0xB6,"MOVZX", true, OP(G,v,""), OP(E,b,""), NOP_);
	T(0xB7,"MOVZX", true, OP(G,v,""), OP(E,w,""), NOP_);
	T(0xBE,"MOVSX", true, OP(G,v,""), OP(E,b,""), NOP_);
	T(0xBF,"MOVSX", true, OP(G,v,""), OP(E,w,""), NOP_);

	// BSF / BSR  Gv, Ev
	T(0xBC,"BSF", true, OP(G,v,""), OP(E,v,""), NOP_);
	T(0xBD,"BSR", true, OP(G,v,""), OP(E,v,""), NOP_);

	// Group 8: BT/BTS/BTR/BTC  Ev, Ib  (0F BA) - ModRM + imm8, name from ModRM.reg
	TG(0xBA,"GRP8", true, OP(E,v,""), OP(I,b,""), NOP_, 8);

	// BSWAP +r  (0F C8-CF) - register in low 3 opcode bits, no ModRM/immediate
	T(0xC8,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xC9,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCA,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCB,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCC,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCD,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCE,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);
	T(0xCF,"BSWAP", false, OP(Z,v,""), NOP_, NOP_);

	// MMX / SSE with no mandatory prefix; the 66 / F3 / F2 forms are in buildPrefixed().
#define V_ OP(V,None,"")
#define W_ OP(W,None,"")
#define P_ OP(P,None,"")
#define Q_ OP(Q,None,"")
#define VW(idx,text) T(idx, text, true, V_, W_, NOP_)
#define PQ(idx,text) T(idx, text, true, P_, Q_, NOP_)
	VW(0x10,"MOVUPS");   T(0x11,"MOVUPS", true, W_, V_, NOP_);
	VW(0x12,"MOVLPS");   T(0x13,"MOVLPS", true, W_, V_, NOP_);   // reg form: MOVHLPS (twoByteResolvedInfo)
	VW(0x14,"UNPCKLPS"); VW(0x15,"UNPCKHPS");
	VW(0x16,"MOVHPS");   T(0x17,"MOVHPS", true, W_, V_, NOP_);   // reg form: MOVLHPS
	VW(0x28,"MOVAPS");   T(0x29,"MOVAPS", true, W_, V_, NOP_);
	T(0x2A,"CVTPI2PS", true, V_, Q_, NOP_);  T(0x2B,"MOVNTPS", true, W_, V_, NOP_);
	T(0x2C,"CVTTPS2PI", true, P_, W_, NOP_); T(0x2D,"CVTPS2PI", true, P_, W_, NOP_);
	VW(0x2E,"UCOMISS");  VW(0x2F,"COMISS");
	T(0x50,"MOVMSKPS", true, OP(G,d,""), W_, NOP_);
	VW(0x51,"SQRTPS"); VW(0x52,"RSQRTPS"); VW(0x53,"RCPPS"); VW(0x54,"ANDPS");
	VW(0x55,"ANDNPS"); VW(0x56,"ORPS");    VW(0x57,"XORPS"); VW(0x58,"ADDPS");
	VW(0x59,"MULPS");  VW(0x5A,"CVTPS2PD");VW(0x5B,"CVTDQ2PS"); VW(0x5C,"SUBPS");
	VW(0x5D,"MINPS");  VW(0x5E,"DIVPS");   VW(0x5F,"MAXPS");
	PQ(0x60,"PUNPCKLBW"); PQ(0x61,"PUNPCKLWD"); PQ(0x62,"PUNPCKLDQ"); PQ(0x63,"PACKSSWB");
	PQ(0x64,"PCMPGTB");   PQ(0x65,"PCMPGTW");   PQ(0x66,"PCMPGTD");   PQ(0x67,"PACKUSWB");
	PQ(0x68,"PUNPCKHBW"); PQ(0x69,"PUNPCKHWD"); PQ(0x6A,"PUNPCKHDQ"); PQ(0x6B,"PACKSSDW");
	T(0x6E,"MOVD", true, P_, OP(E,v,""), NOP_);  table[0x6E].text64 = "MOVQ";
	PQ(0x6F,"MOVQ");
	T(0x70,"PSHUFW", true, P_, Q_, OP(I,b,""));
	TG(0x71,"GRP12", true, Q_, OP(I,b,""), NOP_, 12);
	TG(0x72,"GRP13", true, Q_, OP(I,b,""), NOP_, 13);
	TG(0x73,"GRP14", true, Q_, OP(I,b,""), NOP_, 14);
	PQ(0x74,"PCMPEQB"); PQ(0x75,"PCMPEQW"); PQ(0x76,"PCMPEQD");
	T(0x7E,"MOVD", true, OP(E,v,""), P_, NOP_);  table[0x7E].text64 = "MOVQ";
	T(0x7F,"MOVQ", true, Q_, P_, NOP_);
	T(0x77,"EMMS", false, NOP_, NOP_, NOP_);
	// 3DNow!: FEMMS, the 0F 0D prefetches, and 0F 0F, whose real opcode is the byte after
	// its operands - the decoder swaps in that threeDNowTable() row.
	T(0x0E,"FEMMS", false, NOP_, NOP_, NOP_);
	TG(0x0D,"GRP18", true, NOP_, NOP_, NOP_, 18);
	T(0x0F,"3DNOW", true, P_, Q_, NOP_);
	TG(0x18,"GRP16", true, NOP_, NOP_, NOP_, 16);   // PREFETCHh
	TG(0xAE,"GRP15", true, NOP_, NOP_, NOP_, 15);   // FXSAVE..CLFLUSH, and the fences
	T(0xC2,"CMPPS",  true, V_, W_, OP(I,b,""));
	T(0xC3,"MOVNTI", true, OP(M,None,""), OP(G,v,""), NOP_);
	T(0xC4,"PINSRW", true, P_, OP(E,d,""), OP(I,b,""));
	T(0xC5,"PEXTRW", true, OP(G,d,""), Q_, OP(I,b,""));
	T(0xC6,"SHUFPS", true, V_, W_, OP(I,b,""));
	PQ(0xD1,"PSRLW");   PQ(0xD2,"PSRLD");   PQ(0xD3,"PSRLQ");  PQ(0xD4,"PADDQ"); PQ(0xD5,"PMULLW");
	T(0xD7,"PMOVMSKB", true, OP(G,d,""), Q_, NOP_);
	PQ(0xD8,"PSUBUSB"); PQ(0xD9,"PSUBUSW"); PQ(0xDA,"PMINUB"); PQ(0xDB,"PAND");
	PQ(0xDC,"PADDUSB"); PQ(0xDD,"PADDUSW"); PQ(0xDE,"PMAXUB"); PQ(0xDF,"PANDN");
	PQ(0xE0,"PAVGB");   PQ(0xE1,"PSRAW");   PQ(0xE2,"PSRAD");  PQ(0xE3,"PAVGW");
	PQ(0xE4,"PMULHUW"); PQ(0xE5,"PMULHW");  T(0xE7,"MOVNTQ", true, Q_, P_, NOP_);
	PQ(0xE8,"PSUBSB");  PQ(0xE9,"PSUBSW");  PQ(0xEA,"PMINSW"); PQ(0xEB,"POR");
	PQ(0xEC,"PADDSB");  PQ(0xED,"PADDSW");  PQ(0xEE,"PMAXSW"); PQ(0xEF,"PXOR");
	PQ(0xF1,"PSLLW");   PQ(0xF2,"PSLLD");   PQ(0xF3,"PSLLQ");  PQ(0xF4,"PMULUDQ");
	PQ(0xF5,"PMADDWD"); PQ(0xF6,"PSADBW");  PQ(0xF7,"MASKMOVQ");
	PQ(0xF8,"PSUBB");   PQ(0xF9,"PSUBW");   PQ(0xFA,"PSUBD");  PQ(0xFB,"PSUBQ");
	PQ(0xFC,"PADDB");   PQ(0xFD,"PADDW");   PQ(0xFE,"PADDD");
#undef PQ
#undef VW
#undef Q_
#undef P_
#undef W_
#undef V_

	// Every row still unnamed takes a ModRM. That is the safe default here, not
	// an approximation: the unnamed part of the 0F map is almost entirely MMX/SSE
	// (0F 10-6F, 0F 74-7F, 0F D0-FF), and every one of those is ModRM-addressed.
	// The rows that genuinely have no ModRM - SYSCALL, RDTSC, CPUID, UD2, Jcc,
	// BSWAP - are all named above and keep hasRMByte = false.
	//
	// The old default of false made an unnamed row two bytes long, so 0F 29 44 24
	// 50 (MOVAPS) consumed 2 instead of 5 and the sweep resumed mid-instruction,
	// mis-decoding the rest of .text. Consuming the ModRM (and the SIB and
	// displacement it implies) keeps the stream aligned even when we cannot name
	// the opcode: one "(bad)" line of the correct length, then back in step.
	for (auto& row : table)
		if (row.text.empty()) {
			row.text = "(bad)";
			row.hasRMByte = true;
		}

#undef NOP_
#undef OP
#undef s
#undef a
#undef TG
#undef T
#undef T16
	return table;
}

// The 0F rows a mandatory prefix selects, slot * 256 + opcode (0 = 66, 1 = F3, 2 = F2).
// An MMX row's SSE2 twin under 66 - same name, XMM operands - is not listed here:
// twoByteResolvedInfo() derives it from the plain row.
constexpr std::array<OpcodeInfo, 768> buildPrefixed() {
	std::array<OpcodeInfo, 768> table{};
	constexpr uint8_t slotPrefix[3] = { 0x66, 0xF3, 0xF2 };

#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")
#define V_ OP(V,None,"")
#define W_ OP(W,None,"")
#define P_ OP(P,None,"")
#define Q_ OP(Q,None,"")
#define R(slot,idx,text,op1,op2,op3) table[(slot)*256+(idx)] = OpcodeInfo{text,"", true, op1, op2, op3, -1, false, "", OpcodeInfo::Default64::None, slotPrefix[slot]}
#define VW(slot,idx,text) R(slot,idx,text,V_,W_,NOP_)
#define WV(slot,idx,text) R(slot,idx,text,W_,V_,NOP_)
#define Ib OP(I,b,"")
#define Ev OP(E,v,"")
#define Gv OP(G,v,"")
	// 66: the packed-double forms, plus the SSE2 integer rows not named after their MMX twin.
	VW(0,0x10,"MOVUPD");   WV(0,0x11,"MOVUPD");   VW(0,0x12,"MOVLPD");  WV(0,0x13,"MOVLPD");
	VW(0,0x14,"UNPCKLPD"); VW(0,0x15,"UNPCKHPD"); VW(0,0x16,"MOVHPD");  WV(0,0x17,"MOVHPD");
	VW(0,0x28,"MOVAPD");   WV(0,0x29,"MOVAPD");   R(0,0x2A,"CVTPI2PD",V_,Q_,NOP_); WV(0,0x2B,"MOVNTPD");
	R(0,0x2C,"CVTTPD2PI",P_,W_,NOP_); R(0,0x2D,"CVTPD2PI",P_,W_,NOP_);
	VW(0,0x2E,"UCOMISD");  VW(0,0x2F,"COMISD");   R(0,0x50,"MOVMSKPD",OP(G,d,""),W_,NOP_);
	VW(0,0x51,"SQRTPD");   VW(0,0x54,"ANDPD");    VW(0,0x55,"ANDNPD");  VW(0,0x56,"ORPD");
	VW(0,0x57,"XORPD");    VW(0,0x58,"ADDPD");    VW(0,0x59,"MULPD");   VW(0,0x5A,"CVTPD2PS");
	VW(0,0x5B,"CVTPS2DQ"); VW(0,0x5C,"SUBPD");    VW(0,0x5D,"MINPD");   VW(0,0x5E,"DIVPD");
	VW(0,0x5F,"MAXPD");    VW(0,0x6C,"PUNPCKLQDQ"); VW(0,0x6D,"PUNPCKHQDQ");
	VW(0,0x6F,"MOVDQA");   R(0,0x70,"PSHUFD",V_,W_,Ib); WV(0,0x7F,"MOVDQA");
	R(0,0xC2,"CMPPD",V_,W_,Ib); R(0,0xC6,"SHUFPD",V_,W_,Ib); WV(0,0xD6,"MOVQ");
	VW(0,0xE6,"CVTTPD2DQ");WV(0,0xE7,"MOVNTDQ");  VW(0,0xF7,"MASKMOVDQU");
	VW(0,0x7C,"HADDPD");   VW(0,0x7D,"HSUBPD");   VW(0,0xD0,"ADDSUBPD");   // SSE3

	// F3: scalar single, MOVDQU, and the bit-count ops that would otherwise read as REP.
	VW(1,0x10,"MOVSS");    WV(1,0x11,"MOVSS");    VW(1,0x12,"MOVSLDUP"); VW(1,0x16,"MOVSHDUP");
	R(1,0x2A,"CVTSI2SS",V_,Ev,NOP_); R(1,0x2C,"CVTTSS2SI",Gv,W_,NOP_); R(1,0x2D,"CVTSS2SI",Gv,W_,NOP_);
	VW(1,0x51,"SQRTSS");   VW(1,0x52,"RSQRTSS");  VW(1,0x53,"RCPSS");   VW(1,0x58,"ADDSS");
	VW(1,0x59,"MULSS");    VW(1,0x5A,"CVTSS2SD"); VW(1,0x5B,"CVTTPS2DQ"); VW(1,0x5C,"SUBSS");
	VW(1,0x5D,"MINSS");    VW(1,0x5E,"DIVSS");    VW(1,0x5F,"MAXSS");
	VW(1,0x6F,"MOVDQU");   R(1,0x70,"PSHUFHW",V_,W_,Ib); VW(1,0x7E,"MOVQ"); WV(1,0x7F,"MOVDQU");
	R(1,0xB8,"POPCNT",Gv,Ev,NOP_); R(1,0xBC,"TZCNT",Gv,Ev,NOP_); R(1,0xBD,"LZCNT",Gv,Ev,NOP_);
	R(1,0xC2,"CMPSS",V_,W_,Ib); R(1,0xD6,"MOVQ2DQ",V_,Q_,NOP_); VW(1,0xE6,"CVTDQ2PD");

	// F2: scalar double.
	VW(2,0x10,"MOVSD");    WV(2,0x11,"MOVSD");    VW(2,0x12,"MOVDDUP");
	R(2,0x2A,"CVTSI2SD",V_,Ev,NOP_); R(2,0x2C,"CVTTSD2SI",Gv,W_,NOP_); R(2,0x2D,"CVTSD2SI",Gv,W_,NOP_);
	VW(2,0x51,"SQRTSD");   VW(2,0x58,"ADDSD");    VW(2,0x59,"MULSD");   VW(2,0x5A,"CVTSD2SS");
	VW(2,0x5C,"SUBSD");    VW(2,0x5D,"MINSD");    VW(2,0x5E,"DIVSD");   VW(2,0x5F,"MAXSD");
	R(2,0x70,"PSHUFLW",V_,W_,Ib); R(2,0xC2,"CMPSD",V_,W_,Ib); R(2,0xD6,"MOVDQ2Q",P_,W_,NOP_);
	VW(2,0xE6,"CVTPD2DQ"); VW(2,0xF0,"LDDQU");
	VW(2,0x7C,"HADDPS");   VW(2,0x7D,"HSUBPS");   VW(2,0xD0,"ADDSUBPS");   // SSE3
#undef Gv
#undef Ev
#undef Ib
#undef WV
#undef VW
#undef R
#undef Q_
#undef P_
#undef W_
#undef V_
#undef NOP_
#undef OP
#undef s
#undef a
	return table;
}

// The 0F groups, (groupNumber - 8) * 8 + ModRM.reg. Names - and for 15 / 16 the memory
// operand - only; the rest comes from the outer row, like one-byte grp1/2.
constexpr std::array<OpcodeInfo, 128> buildTwoByteGroups() {
	std::array<OpcodeInfo, 128> table{};
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")
#define GT(g,reg,text) table[((g)-8)*8+(reg)] = OpcodeInfo{text,"", true, NOP_, NOP_, NOP_, g}
#define GM(g,reg,text) table[((g)-8)*8+(reg)] = OpcodeInfo{text,"", true, OP(M,None,""), NOP_, NOP_, g}
	// 0F BA group 8: /4 BT /5 BTS /6 BTR /7 BTC (/0../3 illegal). Operands Ev, Ib.
	GT(8,4,"BT"); GT(8,5,"BTS"); GT(8,6,"BTR"); GT(8,7,"BTC");
	// 0F 71-73 groups 12-14: shift an MMX / XMM register by an imm8.
	GT(12,2,"PSRLW"); GT(12,4,"PSRAW");  GT(12,6,"PSLLW");
	GT(13,2,"PSRLD"); GT(13,4,"PSRAD");  GT(13,6,"PSLLD");
	GT(14,2,"PSRLQ"); GT(14,3,"PSRLDQ"); GT(14,6,"PSLLQ"); GT(14,7,"PSLLDQ");
	// 0F AE group 15: memory forms here, register forms (the fences) under pseudo group 17.
	GM(15,0,"FXSAVE"); GM(15,1,"FXRSTOR"); GM(15,2,"LDMXCSR");  GM(15,3,"STMXCSR");
	GM(15,4,"XSAVE");  GM(15,5,"XRSTOR");  GM(15,6,"XSAVEOPT"); GM(15,7,"CLFLUSH");
	GT(17,5,"LFENCE"); GT(17,6,"MFENCE");  GT(17,7,"SFENCE");
	// 0F 18 group 16: /0../3 PREFETCHh, /4../7 reserved hint NOPs.
	GM(16,0,"PREFETCHNTA"); GM(16,1,"PREFETCHT0"); GM(16,2,"PREFETCHT1"); GM(16,3,"PREFETCHT2");
	for (int r = 4; r < 8; ++r) table[(16-8)*8+r] = OpcodeInfo{"NOP","", true, OP(E,v,""), NOP_, NOP_, 16};
	// 0F 0D group 18 (3DNow!): /1 PREFETCHW, every other /reg PREFETCH.
	for (int r = 0; r < 8; ++r) GM(18, r, (r == 1) ? "PREFETCHW" : "PREFETCH");
	// VEX groups; the operands come from the vexTable() row. 19-21 (66 0F 71-73) shift
	// into vvvv's register, 22 (0F AE) is the MXCSR pair, 23 (0F 38 F3) is BMI1.
	GT(19,2,"VPSRLW"); GT(19,4,"VPSRAW");  GT(19,6,"VPSLLW");
	GT(20,2,"VPSRLD"); GT(20,4,"VPSRAD");  GT(20,6,"VPSLLD");
	GT(21,2,"VPSRLQ"); GT(21,3,"VPSRLDQ"); GT(21,6,"VPSLLQ"); GT(21,7,"VPSLLDQ");
	GM(22,2,"VLDMXCSR"); GM(22,3,"VSTMXCSR");
	GT(23,1,"BLSR");   GT(23,2,"BLSMSK");  GT(23,3,"BLSI");
#undef GM
#undef GT
#undef NOP_
#undef OP
#undef s
#undef a
	return table;
}

// The 0F 38 / 0F 3A rows, ((map - 2) * 2 + slot) * 256 + opcode with slot 0 = no prefix,
// 1 = 66. No prefix: SSSE3's MMX forms (the 66 / XMM forms are not named). 66: SSE4.1 and
// SSE4.2. CRC32, the one row behind F2, is in threeByteResolvedInfo().
constexpr std::array<OpcodeInfo, 1024> buildThreeByte() {
	std::array<OpcodeInfo, 1024> table{};
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")
#define V_ OP(V,None,"")
#define W_ OP(W,None,"")
#define Ib OP(I,b,"")
#define Ed OP(E,d,"")
#define Ey OP(E,y,"")
#define XMM0 OP(ST,None,"XMM0")   // implicit operand: like x87's ST(i), printed from its value
#define PQ(map,idx,text,op3) table[((map)-2)*512+(idx)] = OpcodeInfo{text,"", true, OP(P,None,""), OP(Q,None,""), op3, -1}
#define R(map,idx,text,text64,op1,op2,op3) table[((map)-2)*512+256+(idx)] = OpcodeInfo{text,"", true, op1, op2, op3, -1, false, text64, OpcodeInfo::Default64::None, 0x66}
#define VW(map,idx,text) R(map,idx,text,"",V_,W_,NOP_)
#define VWI(map,idx,text) R(map,idx,text,"",V_,W_,Ib)
	// SSSE3, MMX forms.
	PQ(2,0x00,"PSHUFB",NOP_);    PQ(2,0x01,"PHADDW",NOP_); PQ(2,0x02,"PHADDD",NOP_); PQ(2,0x03,"PHADDSW",NOP_);
	PQ(2,0x04,"PMADDUBSW",NOP_); PQ(2,0x05,"PHSUBW",NOP_); PQ(2,0x06,"PHSUBD",NOP_); PQ(2,0x07,"PHSUBSW",NOP_);
	PQ(2,0x08,"PSIGNB",NOP_);    PQ(2,0x09,"PSIGNW",NOP_); PQ(2,0x0A,"PSIGND",NOP_); PQ(2,0x0B,"PMULHRSW",NOP_);
	PQ(2,0x1C,"PABSB",NOP_);     PQ(2,0x1D,"PABSW",NOP_);  PQ(2,0x1E,"PABSD",NOP_);  PQ(3,0x0F,"PALIGNR",Ib);

	// SSE4.1
	R(2,0x10,"PBLENDVB","",V_,W_,XMM0); R(2,0x14,"BLENDVPS","",V_,W_,XMM0); R(2,0x15,"BLENDVPD","",V_,W_,XMM0);
	VW(2,0x17,"PTEST");
	VW(2,0x20,"PMOVSXBW"); VW(2,0x21,"PMOVSXBD"); VW(2,0x22,"PMOVSXBQ"); VW(2,0x23,"PMOVSXWD"); VW(2,0x24,"PMOVSXWQ"); VW(2,0x25,"PMOVSXDQ");
	VW(2,0x28,"PMULDQ");   VW(2,0x29,"PCMPEQQ");  VW(2,0x2A,"MOVNTDQA"); VW(2,0x2B,"PACKUSDW");
	VW(2,0x30,"PMOVZXBW"); VW(2,0x31,"PMOVZXBD"); VW(2,0x32,"PMOVZXBQ"); VW(2,0x33,"PMOVZXWD"); VW(2,0x34,"PMOVZXWQ"); VW(2,0x35,"PMOVZXDQ");
	VW(2,0x38,"PMINSB");   VW(2,0x39,"PMINSD");   VW(2,0x3A,"PMINUW");   VW(2,0x3B,"PMINUD");
	VW(2,0x3C,"PMAXSB");   VW(2,0x3D,"PMAXSD");   VW(2,0x3E,"PMAXUW");   VW(2,0x3F,"PMAXUD");
	VW(2,0x40,"PMULLD");   VW(2,0x41,"PHMINPOSUW");
	VWI(3,0x08,"ROUNDPS"); VWI(3,0x09,"ROUNDPD"); VWI(3,0x0A,"ROUNDSS"); VWI(3,0x0B,"ROUNDSD");
	VWI(3,0x0C,"BLENDPS"); VWI(3,0x0D,"BLENDPD"); VWI(3,0x0E,"PBLENDW");
	R(3,0x14,"PEXTRB","",Ed,V_,Ib);  R(3,0x15,"PEXTRW","",Ed,V_,Ib);
	R(3,0x16,"PEXTRD","PEXTRQ",Ey,V_,Ib); R(3,0x17,"EXTRACTPS","",Ed,V_,Ib);
	R(3,0x20,"PINSRB","",V_,Ed,Ib);  VWI(3,0x21,"INSERTPS"); R(3,0x22,"PINSRD","PINSRQ",V_,Ey,Ib);
	VWI(3,0x40,"DPPS");    VWI(3,0x41,"DPPD");    VWI(3,0x42,"MPSADBW");

	// SSE4.2
	VW(2,0x37,"PCMPGTQ");
	VWI(3,0x60,"PCMPESTRM"); VWI(3,0x61,"PCMPESTRI"); VWI(3,0x62,"PCMPISTRM"); VWI(3,0x63,"PCMPISTRI");
#undef VWI
#undef VW
#undef R
#undef PQ
#undef XMM0
#undef Ey
#undef Ed
#undef Ib
#undef W_
#undef V_
#undef NOP_
#undef OP
#undef s
#undef a
	return table;
}

// 3DNow! (0F 0F /r ib), keyed by the trailing byte. Every one is Pq, Qq.
constexpr std::array<OpcodeInfo, 256> buildThreeDNow() {
	std::array<OpcodeInfo, 256> table{};
#define D(idx,text) table[idx] = OpcodeInfo{text,"", true, { { ADDRESSING::P, SIZE::None, "" }, { ADDRESSING::Q, SIZE::None, "" }, {} }, -1}
	D(0x0C,"PI2FW");    D(0x0D,"PI2FD");   D(0x1C,"PF2IW");    D(0x1D,"PF2ID");
	D(0x8A,"PFNACC");   D(0x8E,"PFPNACC"); D(0x90,"PFCMPGE");  D(0x94,"PFMIN");
	D(0x96,"PFRCP");    D(0x97,"PFRSQRT"); D(0x9A,"PFSUB");    D(0x9E,"PFADD");
	D(0xA0,"PFCMPGT");  D(0xA4,"PFMAX");   D(0xA6,"PFRCPIT1"); D(0xA7,"PFRSQIT1");
	D(0xAA,"PFSUBR");   D(0xAE,"PFACC");   D(0xB0,"PFCMPEQ");  D(0xB4,"PFMUL");
	D(0xB6,"PFRCPIT2"); D(0xB7,"PMULHRW"); D(0xBB,"PSWAPD");   D(0xBF,"PAVGUSB");
#undef D
	return table;
}

// The VEX rows, ((map - 1) * 4 + pp) * 256 + opcode - so slot 0-3 is 0F, 4-7 is 0F 38 and
// 8-11 is 0F 3A, each as none / 66 / F3 / F2. AVX, AVX2 (its YMM integer forms are the
// same rows under VEX.L), FMA and BMI1/2. text64 is the VEX.W1 name.
constexpr std::array<OpcodeInfo, 3072> buildVex() {
	std::array<OpcodeInfo, 3072> table{};
#define a(name) ADDRESSING::name
#define s(name) SIZE::name
#define OP(mode,sz,val) TableOperand{ a(mode), s(sz), val }
#define NOP_ OP(None,None,"")
#define Vx  OP(V,None,"")
#define Vdq OP(V,dq,"")
#define Wx  OP(W,None,"")
#define Wdq OP(W,dq,"")
#define Hx  OP(H,None,"")
#define Hdq OP(H,dq,"")
#define By  OP(H,y,"")
#define Gy  OP(G,y,"")
#define Ey  OP(E,y,"")
#define Gd  OP(G,d,"")
#define Ed  OP(E,d,"")
#define Lx  OP(L,b,"")
#define Ib  OP(I,b,"")
#define VSIBx OP(VSIB,None,"")
#define RW(slot,idx,text,text64,nds,op1,op2,op3) table[(slot)*256+(idx)] = OpcodeInfo{text,"", true, op1, op2, op3, -1, false, text64, OpcodeInfo::Default64::None, 0, nds}
#define R(slot,idx,text,nds,op1,op2,op3) table[(slot)*256+(idx)] = OpcodeInfo{text,"", true, op1, op2, op3, -1, false, "", OpcodeInfo::Default64::None, 0, nds}
#define RG(slot,idx,group,op1,op2,op3) table[(slot)*256+(idx)] = OpcodeInfo{"GRP","", true, op1, op2, op3, group}
#define VHW(slot,idx,text)  R(slot,idx,text,true,Vx,Wx,NOP_)     // V, H, W
#define VHWI(slot,idx,text) R(slot,idx,text,true,Vx,Wx,Ib)       // V, H, W, imm8
#define VHWs(slot,idx,text) R(slot,idx,text,true,Vdq,Wdq,NOP_)   // scalar: XMM whatever L says
#define VHWc(slot,idx,text) R(slot,idx,text,true,Vx,Wdq,NOP_)    // shift by the count in an XMM
#define VW(slot,idx,text)   R(slot,idx,text,false,Vx,Wx,NOP_)    // V, W
#define VWI(slot,idx,text)  R(slot,idx,text,false,Vx,Wx,Ib)      // V, W, imm8
#define VWs(slot,idx,text)  R(slot,idx,text,false,Vdq,Wdq,NOP_)
#define VWx(slot,idx,text)  R(slot,idx,text,false,Vx,Wdq,NOP_)   // widening from an XMM: VPMOVSX, broadcasts
#define WV(slot,idx,text)   R(slot,idx,text,false,Wx,Vx,NOP_)    // stores

	// 0F, no prefix
	VW(0,0x10,"VMOVUPS");   WV(0,0x11,"VMOVUPS");
	R(0,0x12,"VMOVLPS",true,Vdq,Wdq,NOP_); R(0,0x13,"VMOVLPS",false,Wdq,Vdq,NOP_);   // reg form: VMOVHLPS
	VHW(0,0x14,"VUNPCKLPS"); VHW(0,0x15,"VUNPCKHPS");
	R(0,0x16,"VMOVHPS",true,Vdq,Wdq,NOP_); R(0,0x17,"VMOVHPS",false,Wdq,Vdq,NOP_);   // reg form: VMOVLHPS
	VW(0,0x28,"VMOVAPS");   WV(0,0x29,"VMOVAPS");   WV(0,0x2B,"VMOVNTPS");
	VWs(0,0x2E,"VUCOMISS"); VWs(0,0x2F,"VCOMISS");  R(0,0x50,"VMOVMSKPS",false,Gd,Wx,NOP_);
	VW(0,0x51,"VSQRTPS");   VW(0,0x52,"VRSQRTPS");  VW(0,0x53,"VRCPPS");
	VHW(0,0x54,"VANDPS");   VHW(0,0x55,"VANDNPS");  VHW(0,0x56,"VORPS");     VHW(0,0x57,"VXORPS");
	VHW(0,0x58,"VADDPS");   VHW(0,0x59,"VMULPS");   VWx(0,0x5A,"VCVTPS2PD"); VW(0,0x5B,"VCVTDQ2PS");
	VHW(0,0x5C,"VSUBPS");   VHW(0,0x5D,"VMINPS");   VHW(0,0x5E,"VDIVPS");    VHW(0,0x5F,"VMAXPS");
	table[0x77] = OpcodeInfo{"VZEROUPPER","", false, NOP_, NOP_, NOP_, -1};   // VZEROALL under L
	RG(0,0xAE,22,NOP_,NOP_,NOP_);
	VHWI(0,0xC2,"VCMPPS");  VHWI(0,0xC6,"VSHUFPS");

	// 66 0F
	VW(1,0x10,"VMOVUPD");   WV(1,0x11,"VMOVUPD");
	R(1,0x12,"VMOVLPD",true,Vdq,Wdq,NOP_); R(1,0x13,"VMOVLPD",false,Wdq,Vdq,NOP_);
	VHW(1,0x14,"VUNPCKLPD"); VHW(1,0x15,"VUNPCKHPD");
	R(1,0x16,"VMOVHPD",true,Vdq,Wdq,NOP_); R(1,0x17,"VMOVHPD",false,Wdq,Vdq,NOP_);
	VW(1,0x28,"VMOVAPD");   WV(1,0x29,"VMOVAPD");   WV(1,0x2B,"VMOVNTPD");
	VWs(1,0x2E,"VUCOMISD"); VWs(1,0x2F,"VCOMISD");  R(1,0x50,"VMOVMSKPD",false,Gd,Wx,NOP_);
	VW(1,0x51,"VSQRTPD");
	VHW(1,0x54,"VANDPD");   VHW(1,0x55,"VANDNPD");  VHW(1,0x56,"VORPD");     VHW(1,0x57,"VXORPD");
	VHW(1,0x58,"VADDPD");   VHW(1,0x59,"VMULPD");   R(1,0x5A,"VCVTPD2PS",false,Vdq,Wx,NOP_); VW(1,0x5B,"VCVTPS2DQ");
	VHW(1,0x5C,"VSUBPD");   VHW(1,0x5D,"VMINPD");   VHW(1,0x5E,"VDIVPD");    VHW(1,0x5F,"VMAXPD");
	VHW(1,0x60,"VPUNPCKLBW"); VHW(1,0x61,"VPUNPCKLWD"); VHW(1,0x62,"VPUNPCKLDQ"); VHW(1,0x63,"VPACKSSWB");
	VHW(1,0x64,"VPCMPGTB");   VHW(1,0x65,"VPCMPGTW");   VHW(1,0x66,"VPCMPGTD");   VHW(1,0x67,"VPACKUSWB");
	VHW(1,0x68,"VPUNPCKHBW"); VHW(1,0x69,"VPUNPCKHWD"); VHW(1,0x6A,"VPUNPCKHDQ"); VHW(1,0x6B,"VPACKSSDW");
	VHW(1,0x6C,"VPUNPCKLQDQ"); VHW(1,0x6D,"VPUNPCKHQDQ");
	RW(1,0x6E,"VMOVD","VMOVQ",false,Vdq,Ey,NOP_); VW(1,0x6F,"VMOVDQA"); VWI(1,0x70,"VPSHUFD");
	RG(1,0x71,19,Hx,Wx,Ib); RG(1,0x72,20,Hx,Wx,Ib); RG(1,0x73,21,Hx,Wx,Ib);
	VHW(1,0x74,"VPCMPEQB"); VHW(1,0x75,"VPCMPEQW"); VHW(1,0x76,"VPCMPEQD");
	VHW(1,0x7C,"VHADDPD");  VHW(1,0x7D,"VHSUBPD");
	RW(1,0x7E,"VMOVD","VMOVQ",false,Ey,Vdq,NOP_); WV(1,0x7F,"VMOVDQA");
	VHWI(1,0xC2,"VCMPPD");  R(1,0xC4,"VPINSRW",true,Vdq,Ed,Ib); R(1,0xC5,"VPEXTRW",false,Gd,Wdq,Ib); VHWI(1,0xC6,"VSHUFPD");
	VHW(1,0xD0,"VADDSUBPD");
	VHWc(1,0xD1,"VPSRLW");  VHWc(1,0xD2,"VPSRLD");  VHWc(1,0xD3,"VPSRLQ");  VHW(1,0xD4,"VPADDQ");  VHW(1,0xD5,"VPMULLW");
	R(1,0xD6,"VMOVQ",false,Wdq,Vdq,NOP_); R(1,0xD7,"VPMOVMSKB",false,Gd,Wx,NOP_);
	VHW(1,0xD8,"VPSUBUSB"); VHW(1,0xD9,"VPSUBUSW"); VHW(1,0xDA,"VPMINUB"); VHW(1,0xDB,"VPAND");
	VHW(1,0xDC,"VPADDUSB"); VHW(1,0xDD,"VPADDUSW"); VHW(1,0xDE,"VPMAXUB"); VHW(1,0xDF,"VPANDN");
	VHW(1,0xE0,"VPAVGB");   VHWc(1,0xE1,"VPSRAW");  VHWc(1,0xE2,"VPSRAD"); VHW(1,0xE3,"VPAVGW");
	VHW(1,0xE4,"VPMULHUW"); VHW(1,0xE5,"VPMULHW");  R(1,0xE6,"VCVTTPD2DQ",false,Vdq,Wx,NOP_); WV(1,0xE7,"VMOVNTDQ");
	VHW(1,0xE8,"VPSUBSB");  VHW(1,0xE9,"VPSUBSW");  VHW(1,0xEA,"VPMINSW"); VHW(1,0xEB,"VPOR");
	VHW(1,0xEC,"VPADDSB");  VHW(1,0xED,"VPADDSW");  VHW(1,0xEE,"VPMAXSW"); VHW(1,0xEF,"VPXOR");
	VHWc(1,0xF1,"VPSLLW");  VHWc(1,0xF2,"VPSLLD");  VHWc(1,0xF3,"VPSLLQ"); VHW(1,0xF4,"VPMULUDQ");
	VHW(1,0xF5,"VPMADDWD"); VHW(1,0xF6,"VPSADBW");  VWs(1,0xF7,"VMASKMOVDQU");
	VHW(1,0xF8,"VPSUBB");   VHW(1,0xF9,"VPSUBW");   VHW(1,0xFA,"VPSUBD");  VHW(1,0xFB,"VPSUBQ");
	VHW(1,0xFC,"VPADDB");   VHW(1,0xFD,"VPADDW");   VHW(1,0xFE,"VPADDD");

	// F3 0F
	R(2,0x10,"VMOVSS",true,Vdq,Wdq,NOP_);  R(2,0x11,"VMOVSS",true,Wdq,Vdq,NOP_);   // memory forms take no vvvv
	VW(2,0x12,"VMOVSLDUP"); VW(2,0x16,"VMOVSHDUP");
	R(2,0x2A,"VCVTSI2SS",true,Vdq,Ey,NOP_); R(2,0x2C,"VCVTTSS2SI",false,Gy,Wdq,NOP_); R(2,0x2D,"VCVTSS2SI",false,Gy,Wdq,NOP_);
	VHWs(2,0x51,"VSQRTSS"); VHWs(2,0x52,"VRSQRTSS"); VHWs(2,0x53,"VRCPSS");
	VHWs(2,0x58,"VADDSS");  VHWs(2,0x59,"VMULSS");  VHWs(2,0x5A,"VCVTSS2SD"); VW(2,0x5B,"VCVTTPS2DQ");
	VHWs(2,0x5C,"VSUBSS");  VHWs(2,0x5D,"VMINSS");  VHWs(2,0x5E,"VDIVSS");    VHWs(2,0x5F,"VMAXSS");
	VW(2,0x6F,"VMOVDQU");   VWI(2,0x70,"VPSHUFHW"); VWs(2,0x7E,"VMOVQ");      WV(2,0x7F,"VMOVDQU");
	R(2,0xC2,"VCMPSS",true,Vdq,Wdq,Ib); VWx(2,0xE6,"VCVTDQ2PD");

	// F2 0F
	R(3,0x10,"VMOVSD",true,Vdq,Wdq,NOP_);  R(3,0x11,"VMOVSD",true,Wdq,Vdq,NOP_);
	VW(3,0x12,"VMOVDDUP");
	R(3,0x2A,"VCVTSI2SD",true,Vdq,Ey,NOP_); R(3,0x2C,"VCVTTSD2SI",false,Gy,Wdq,NOP_); R(3,0x2D,"VCVTSD2SI",false,Gy,Wdq,NOP_);
	VHWs(3,0x51,"VSQRTSD"); VHWs(3,0x58,"VADDSD");  VHWs(3,0x59,"VMULSD");  VHWs(3,0x5A,"VCVTSD2SS");
	VHWs(3,0x5C,"VSUBSD");  VHWs(3,0x5D,"VMINSD");  VHWs(3,0x5E,"VDIVSD");  VHWs(3,0x5F,"VMAXSD");
	VWI(3,0x70,"VPSHUFLW"); VHW(3,0x7C,"VHADDPS");  VHW(3,0x7D,"VHSUBPS");
	R(3,0xC2,"VCMPSD",true,Vdq,Wdq,Ib); VHW(3,0xD0,"VADDSUBPS");
	R(3,0xE6,"VCVTPD2DQ",false,Vdq,Wx,NOP_); VW(3,0xF0,"VLDDQU");

	// 66 0F 38
	VHW(5,0x00,"VPSHUFB");    VHW(5,0x01,"VPHADDW");  VHW(5,0x02,"VPHADDD");  VHW(5,0x03,"VPHADDSW");
	VHW(5,0x04,"VPMADDUBSW"); VHW(5,0x05,"VPHSUBW");  VHW(5,0x06,"VPHSUBD");  VHW(5,0x07,"VPHSUBSW");
	VHW(5,0x08,"VPSIGNB");    VHW(5,0x09,"VPSIGNW");  VHW(5,0x0A,"VPSIGND");  VHW(5,0x0B,"VPMULHRSW");
	VHW(5,0x0C,"VPERMILPS");  VHW(5,0x0D,"VPERMILPD"); VW(5,0x0E,"VTESTPS"); VW(5,0x0F,"VTESTPD");
	VHW(5,0x16,"VPERMPS");    VW(5,0x17,"VPTEST");
	VWx(5,0x18,"VBROADCASTSS"); VWx(5,0x19,"VBROADCASTSD"); VWx(5,0x1A,"VBROADCASTF128");
	VW(5,0x1C,"VPABSB");      VW(5,0x1D,"VPABSW");    VW(5,0x1E,"VPABSD");
	VWx(5,0x20,"VPMOVSXBW");  VWx(5,0x21,"VPMOVSXBD"); VWx(5,0x22,"VPMOVSXBQ");
	VWx(5,0x23,"VPMOVSXWD");  VWx(5,0x24,"VPMOVSXWQ"); VWx(5,0x25,"VPMOVSXDQ");
	VHW(5,0x28,"VPMULDQ");    VHW(5,0x29,"VPCMPEQQ"); VW(5,0x2A,"VMOVNTDQA"); VHW(5,0x2B,"VPACKUSDW");
	VHW(5,0x2C,"VMASKMOVPS"); VHW(5,0x2D,"VMASKMOVPD");
	R(5,0x2E,"VMASKMOVPS",true,Wx,Vx,NOP_); R(5,0x2F,"VMASKMOVPD",true,Wx,Vx,NOP_);
	VWx(5,0x30,"VPMOVZXBW");  VWx(5,0x31,"VPMOVZXBD"); VWx(5,0x32,"VPMOVZXBQ");
	VWx(5,0x33,"VPMOVZXWD");  VWx(5,0x34,"VPMOVZXWQ"); VWx(5,0x35,"VPMOVZXDQ");
	VHW(5,0x36,"VPERMD");     VHW(5,0x37,"VPCMPGTQ");
	VHW(5,0x38,"VPMINSB");    VHW(5,0x39,"VPMINSD");  VHW(5,0x3A,"VPMINUW");  VHW(5,0x3B,"VPMINUD");
	VHW(5,0x3C,"VPMAXSB");    VHW(5,0x3D,"VPMAXSD");  VHW(5,0x3E,"VPMAXUW");  VHW(5,0x3F,"VPMAXUD");
	VHW(5,0x40,"VPMULLD");    VWs(5,0x41,"VPHMINPOSUW");
	RW(5,0x45,"VPSRLVD","VPSRLVQ",true,Vx,Wx,NOP_); VHW(5,0x46,"VPSRAVD"); RW(5,0x47,"VPSLLVD","VPSLLVQ",true,Vx,Wx,NOP_);
	VWx(5,0x58,"VPBROADCASTD"); VWx(5,0x59,"VPBROADCASTQ"); VWx(5,0x5A,"VBROADCASTI128");
	VWx(5,0x78,"VPBROADCASTB"); VWx(5,0x79,"VPBROADCASTW");
	RW(5,0x8C,"VPMASKMOVD","VPMASKMOVQ",true,Vx,Wx,NOP_); RW(5,0x8E,"VPMASKMOVD","VPMASKMOVQ",true,Wx,Vx,NOP_);
	// Gathers: dest, [base + vector index*scale], mask - sized for dword elements here;
	// vexResolvedInfo() resizes the qword (W1) forms.
	RW(5,0x90,"VPGATHERDD","VPGATHERDQ",false,Vx,VSIBx,Hx);  RW(5,0x91,"VPGATHERQD","VPGATHERQQ",false,Vdq,VSIBx,Hdq);
	RW(5,0x92,"VGATHERDPS","VGATHERDPD",false,Vx,VSIBx,Hx);  RW(5,0x93,"VGATHERQPS","VGATHERQPD",false,Vdq,VSIBx,Hdq);

	// FMA: PS / SS under W0, PD / SD under W1.
#define FP(idx,ps,pd) RW(5,idx,ps,pd,true,Vx,Wx,NOP_)
#define FS(idx,ss,sd) RW(5,idx,ss,sd,true,Vdq,Wdq,NOP_)
	FP(0x96,"VFMADDSUB132PS","VFMADDSUB132PD"); FP(0x97,"VFMSUBADD132PS","VFMSUBADD132PD");
	FP(0x98,"VFMADD132PS","VFMADD132PD");       FS(0x99,"VFMADD132SS","VFMADD132SD");
	FP(0x9A,"VFMSUB132PS","VFMSUB132PD");       FS(0x9B,"VFMSUB132SS","VFMSUB132SD");
	FP(0x9C,"VFNMADD132PS","VFNMADD132PD");     FS(0x9D,"VFNMADD132SS","VFNMADD132SD");
	FP(0x9E,"VFNMSUB132PS","VFNMSUB132PD");     FS(0x9F,"VFNMSUB132SS","VFNMSUB132SD");
	FP(0xA6,"VFMADDSUB213PS","VFMADDSUB213PD"); FP(0xA7,"VFMSUBADD213PS","VFMSUBADD213PD");
	FP(0xA8,"VFMADD213PS","VFMADD213PD");       FS(0xA9,"VFMADD213SS","VFMADD213SD");
	FP(0xAA,"VFMSUB213PS","VFMSUB213PD");       FS(0xAB,"VFMSUB213SS","VFMSUB213SD");
	FP(0xAC,"VFNMADD213PS","VFNMADD213PD");     FS(0xAD,"VFNMADD213SS","VFNMADD213SD");
	FP(0xAE,"VFNMSUB213PS","VFNMSUB213PD");     FS(0xAF,"VFNMSUB213SS","VFNMSUB213SD");
	FP(0xB6,"VFMADDSUB231PS","VFMADDSUB231PD"); FP(0xB7,"VFMSUBADD231PS","VFMSUBADD231PD");
	FP(0xB8,"VFMADD231PS","VFMADD231PD");       FS(0xB9,"VFMADD231SS","VFMADD231SD");
	FP(0xBA,"VFMSUB231PS","VFMSUB231PD");       FS(0xBB,"VFMSUB231SS","VFMSUB231SD");
	FP(0xBC,"VFNMADD231PS","VFNMADD231PD");     FS(0xBD,"VFNMADD231SS","VFNMADD231SD");
	FP(0xBE,"VFNMSUB231PS","VFNMSUB231PD");     FS(0xBF,"VFNMSUB231SS","VFNMSUB231SD");
#undef FS
#undef FP

	// BMI1 / BMI2: general registers, 32-bit or (W1) 64-bit.
	R(4,0xF2,"ANDN",true,Gy,Ey,NOP_);   RG(4,0xF3,23,By,Ey,NOP_);
	R(4,0xF5,"BZHI",false,Gy,Ey,By);    R(4,0xF7,"BEXTR",false,Gy,Ey,By);
	R(5,0xF7,"SHLX",false,Gy,Ey,By);
	R(6,0xF5,"PEXT",true,Gy,Ey,NOP_);   R(6,0xF7,"SARX",false,Gy,Ey,By);
	R(7,0xF5,"PDEP",true,Gy,Ey,NOP_);   R(7,0xF6,"MULX",true,Gy,Ey,NOP_);  R(7,0xF7,"SHRX",false,Gy,Ey,By);
	R(11,0xF0,"RORX",false,Gy,Ey,Ib);

	// 66 0F 3A
	VWI(9,0x00,"VPERMQ");    VWI(9,0x01,"VPERMPD");   VHWI(9,0x02,"VPBLENDD");
	VWI(9,0x04,"VPERMILPS"); VWI(9,0x05,"VPERMILPD"); VHWI(9,0x06,"VPERM2F128");
	VWI(9,0x08,"VROUNDPS");  VWI(9,0x09,"VROUNDPD");
	R(9,0x0A,"VROUNDSS",true,Vdq,Wdq,Ib); R(9,0x0B,"VROUNDSD",true,Vdq,Wdq,Ib);
	VHWI(9,0x0C,"VBLENDPS"); VHWI(9,0x0D,"VBLENDPD"); VHWI(9,0x0E,"VPBLENDW"); VHWI(9,0x0F,"VPALIGNR");
	R(9,0x14,"VPEXTRB",false,Ed,Vdq,Ib); R(9,0x15,"VPEXTRW",false,Ed,Vdq,Ib);
	RW(9,0x16,"VPEXTRD","VPEXTRQ",false,Ey,Vdq,Ib); R(9,0x17,"VEXTRACTPS",false,Ed,Vdq,Ib);
	R(9,0x18,"VINSERTF128",true,Vx,Wdq,Ib); R(9,0x19,"VEXTRACTF128",false,Wdq,Vx,Ib);
	R(9,0x20,"VPINSRB",true,Vdq,Ed,Ib); R(9,0x21,"VINSERTPS",true,Vdq,Wdq,Ib); RW(9,0x22,"VPINSRD","VPINSRQ",true,Vdq,Ey,Ib);
	R(9,0x38,"VINSERTI128",true,Vx,Wdq,Ib); R(9,0x39,"VEXTRACTI128",false,Wdq,Vx,Ib);
	VHWI(9,0x40,"VDPPS");    VHWI(9,0x41,"VDPPD");    VHWI(9,0x42,"VMPSADBW"); VHWI(9,0x46,"VPERM2I128");
	R(9,0x4A,"VBLENDVPS",true,Vx,Wx,Lx); R(9,0x4B,"VBLENDVPD",true,Vx,Wx,Lx); R(9,0x4C,"VPBLENDVB",true,Vx,Wx,Lx);
	R(9,0x60,"VPCMPESTRM",false,Vdq,Wdq,Ib); R(9,0x61,"VPCMPESTRI",false,Vdq,Wdq,Ib);
	R(9,0x62,"VPCMPISTRM",false,Vdq,Wdq,Ib); R(9,0x63,"VPCMPISTRI",false,Vdq,Wdq,Ib);
#undef WV
#undef VWx
#undef VWs
#undef VWI
#undef VW
#undef VHWc
#undef VHWs
#undef VHWI
#undef VHW
#undef RG
#undef R
#undef RW
#undef VSIBx
#undef Ib
#undef Lx
#undef Ed
#undef Gd
#undef Ey
#undef Gy
#undef By
#undef Hdq
#undef Hx
#undef Wdq
#undef Wx
#undef Vdq
#undef Vx
#undef NOP_
#undef OP
#undef s
#undef a
	return table;
}

} // namespace

const std::array<std::string_view, 256>& x86_64_Mnemonic::prefixTable() {
	static constexpr std::array<std::string_view, 256> prefixes = buildPrefixes();
	return prefixes;
}

const std::array<OpcodeInfo, 256>& x86_64_Mnemonic::opcodeTable() {
	static constexpr std::array<OpcodeInfo, 256> table = buildOpcodes();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::grp1Table() {
	static constexpr std::array<OpcodeInfo, 8> table = buildGroup1();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::grp2Table() {
	static constexpr std::array<OpcodeInfo, 8> table = buildGroup2();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::grp3Table() {
	static constexpr std::array<OpcodeInfo, 8> table = buildGroup3();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::grp4Table() {
	static constexpr std::array<OpcodeInfo, 8> table = buildGroup4();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::grp5Table() {
	static constexpr std::array<OpcodeInfo, 8> table = buildGroup5();
	return table;
}

const std::array<OpcodeInfo, 8>& x86_64_Mnemonic::groupTableOf(uint32_t opcode) {
	switch (groupNoOf(opcode)) {
	case 1: return grp1Table();
	case 2: return grp2Table();
	case 3: return grp3Table();
	case 4: return grp4Table();
	case 5: return grp5Table();
	default:
		throw std::runtime_error("Opcode is not an extension group..");
	}
}

const OpcodeInfo& x86_64_Mnemonic::groupEntryOf(uint32_t opcode, uint8_t reg) {
	return groupTableOf(opcode)[reg & 0x07];
}

const std::array<OpcodeInfo, 64>& x86_64_Mnemonic::x87MemTable() {
	static constexpr std::array<OpcodeInfo, 64> table = buildX87Mem();
	return table;
}

const std::array<OpcodeInfo, 512>& x86_64_Mnemonic::x87RegTable() {
	static constexpr std::array<OpcodeInfo, 512> table = buildX87Reg();
	return table;
}

OpcodeInfo x86_64_Mnemonic::x87ResolvedInfo(uint32_t opcode, uint8_t modRM) {
	const uint32_t escapeIndex = opcode - 0xD8;                   // 0..7
	if (modRM < 0xC0)                                             // mod != 11 -> memory form
		return x87MemTable()[escapeIndex * 8 + ((modRM >> 3) & 0x07)];
	return x87RegTable()[escapeIndex * 64 + (modRM & 0x3F)];      // mod == 11 -> register form
}

namespace {

// A group's real row: the outer row, renamed - and given any operands and long-mode
// default - by the entry ModRM.reg selected.
OpcodeInfo merge(const OpcodeInfo& outer, const OpcodeInfo& entry) {
	if (entry.text.empty()) return entry;   // illegal /reg (FF /7, FE /2..7): "(bad)", no operands

	OpcodeInfo info = outer;
	info.text = entry.text;
	info.text16 = entry.text16;   // both names come from whichever row named the instruction
	if (entry.default64 != OpcodeInfo::Default64::None) info.default64 = entry.default64;

	const SIZE opSize = (outer.operands[0].size != SIZE::None) ? outer.operands[0].size : entry.operands[0].size;
	for (int i = 0; i < 3; ++i) {
		const TableOperand& entryOperand = entry.operands[i];
		if (entryOperand.addressingMode == ADDRESSING::None) continue;   // entry says nothing: keep the outer row's
		// An immediate never widens to 64 bits with the operand size (only B8+r MOV does, and
		// it is not a group): Ev pairs with Iz, so an inherited `v` caps at `z` for an I operand.
		const SIZE inherited = (entryOperand.addressingMode == ADDRESSING::I && opSize == SIZE::v) ? SIZE::z : opSize;
		info.operands[i] = { entryOperand.addressingMode, (entryOperand.size != SIZE::None) ? entryOperand.size : inherited, entryOperand.value };
	}
	return info;
}

} // namespace

OpcodeInfo x86_64_Mnemonic::resolvedInfo(uint32_t opcode, uint8_t modRM, bool is64Bit) {
	if (isX87(opcode)) return x87ResolvedInfo(opcode, modRM);

	// 0x63 is ARPL in 32-bit mode and MOVSXD in long mode: Gv from a dword.
	static constexpr OpcodeInfo movsxd{ "MOVSXD", "", true,
		{ { ADDRESSING::G, SIZE::v, "" }, { ADDRESSING::E, SIZE::d, "" }, {} }, -1 };
	// C6 F8 / C7 F8 are RTM's XABORT imm8 / XBEGIN rel16/32, not MOV - same length, other meaning.
	static constexpr OpcodeInfo xabort{ "XABORT", "", true, { { ADDRESSING::I, SIZE::b, "" }, {}, {} }, -1 };
	static constexpr OpcodeInfo xbegin{ "XBEGIN", "", true, { { ADDRESSING::J, SIZE::z, "" }, {}, {} }, -1 };

	if (is64Bit && opcode == 0x63) return movsxd;
	if (modRM == 0xF8 && (opcode == 0xC6 || opcode == 0xC7)) return (opcode == 0xC6) ? xabort : xbegin;
	if (!isGroup(opcode)) return opcodeTable()[opcode];
	return merge(opcodeTable()[opcode], groupEntryOf(opcode, (modRM >> 3) & 0x07));
}

const OpcodeInfo& x86_64_Mnemonic::threeByteRow(bool hasImm8) {
	static constexpr OpcodeInfo plain{ "(bad)", "", true, {}, -1 };
	static constexpr OpcodeInfo withImm{ "(bad)", "", true, { { ADDRESSING::I, SIZE::b, "" }, {}, {} }, -1 };
	return hasImm8 ? withImm : plain;
}

const std::array<OpcodeInfo, 256>& x86_64_Mnemonic::twoByteTable() {
	static constexpr std::array<OpcodeInfo, 256> table = buildTwoByteOpcodes();
	return table;
}

const std::array<OpcodeInfo, 768>& x86_64_Mnemonic::prefixedTable() {
	static constexpr std::array<OpcodeInfo, 768> table = buildPrefixed();
	return table;
}

const std::array<OpcodeInfo, 128>& x86_64_Mnemonic::twoByteGroupTable() {
	static constexpr std::array<OpcodeInfo, 128> table = buildTwoByteGroups();
	return table;
}

const std::array<OpcodeInfo, 1024>& x86_64_Mnemonic::threeByteTable() {
	static constexpr std::array<OpcodeInfo, 1024> table = buildThreeByte();
	return table;
}

const std::array<OpcodeInfo, 256>& x86_64_Mnemonic::threeDNowTable() {
	static constexpr std::array<OpcodeInfo, 256> table = buildThreeDNow();
	return table;
}

const std::array<OpcodeInfo, 3072>& x86_64_Mnemonic::vexTable() {
	static constexpr std::array<OpcodeInfo, 3072> table = buildVex();
	return table;
}

OpcodeInfo x86_64_Mnemonic::twoByteResolvedInfo(uint32_t opcode, uint8_t modRM, const Simd& simd) {
	// CET landing pads F3 0F 1E FA / FB: the hint-NOP row, promoted by its mandatory F3.
	static constexpr OpcodeInfo endbr64{ "ENDBR64", "", true, {}, -1, false, "", OpcodeInfo::Default64::None, 0xF3 };
	static constexpr OpcodeInfo endbr32{ "ENDBR32", "", true, {}, -1, false, "", OpcodeInfo::Default64::None, 0xF3 };
	if (opcode == 0x1E && simd.pp == 2 && (modRM == 0xFA || modRM == 0xFB)) return (modRM == 0xFA) ? endbr64 : endbr32;
	// SSE3 MONITOR / MWAIT: 0F 01 C8 / C9, two register forms of the (unbuilt) group 7.
	static constexpr OpcodeInfo monitor{ "MONITOR", "", true, {}, -1 };
	static constexpr OpcodeInfo mwait{ "MWAIT", "", true, {}, -1 };
	if (opcode == 0x01 && (modRM == 0xC8 || modRM == 0xC9)) return (modRM == 0xC8) ? monitor : mwait;

	OpcodeInfo outer = (simd.pp && !prefixedTable()[(simd.pp - 1) * 256 + opcode].text.empty())
		? prefixedTable()[(simd.pp - 1) * 256 + opcode] : twoByteTable()[opcode];

	// Under 66 an MMX row is its SSE2 twin: same name, XMM where it had MMX.
	if (simd.pp == 1 && !outer.mandatoryPrefix)
		for (auto& operand : outer.operands)
			if (operand.addressingMode == ADDRESSING::P || operand.addressingMode == ADDRESSING::Q) {
				operand.addressingMode = (operand.addressingMode == ADDRESSING::P) ? ADDRESSING::V : ADDRESSING::W;
				outer.mandatoryPrefix = 0x66;
			}

	// 0F 12 / 0F 16 with no memory operand move between the halves of two registers.
	if (!outer.mandatoryPrefix && modRM >= 0xC0 && (opcode == 0x12 || opcode == 0x16))
		outer.text = (opcode == 0x12) ? "MOVHLPS" : "MOVLHPS";

	if (outer.groupNumber <= 0) return outer;
	const int group = (outer.groupNumber == 15 && modRM >= 0xC0) ? 17 : outer.groupNumber;
	return merge(outer, twoByteGroupTable()[(group - 8) * 8 + ((modRM >> 3) & 0x07)]);
}

OpcodeInfo x86_64_Mnemonic::threeByteResolvedInfo(uint8_t opcode, const Simd& simd) {
	// SSE4.2 CRC32: F2 0F 38 F0 (Eb) / F1 (Ev), into a 32- or (REX.W) 64-bit register.
	static constexpr OpcodeInfo crc32b{ "CRC32", "", true, { { ADDRESSING::G, SIZE::y, "" }, { ADDRESSING::E, SIZE::b, "" }, {} }, -1, false, "", OpcodeInfo::Default64::None, 0xF2 };
	static constexpr OpcodeInfo crc32v{ "CRC32", "", true, { { ADDRESSING::G, SIZE::y, "" }, { ADDRESSING::E, SIZE::v, "" }, {} }, -1, false, "", OpcodeInfo::Default64::None, 0xF2 };
	if (simd.map == 2 && simd.pp == 3 && (opcode & 0xFE) == 0xF0) return (opcode & 1) ? crc32v : crc32b;
	if (simd.pp > 1) return threeByteRow(simd.map == 3);   // only the none / 66 columns are tabulated

	const OpcodeInfo& row = threeByteTable()[((simd.map - 2) * 2 + simd.pp) * 256 + opcode];
	return row.text.empty() ? threeByteRow(simd.map == 3) : row;
}

OpcodeInfo x86_64_Mnemonic::vexResolvedInfo(uint8_t opcode, uint8_t modRM, const Simd& simd) {
	const uint8_t map = simd.map, pp = simd.pp;
	OpcodeInfo info = vexTable()[((map - 1) * 4 + pp) * 256 + opcode];
	if (info.text.empty()) return threeByteRow(map == 3);   // unnamed: ModRM (+ imm8 in 0F 3A), so the length holds

	const bool memory = modRM < 0xC0;
	if (map == 1 && opcode == 0x77 && simd.L) info.text = "VZEROALL";
	// VMOVSS / VMOVSD merge vvvv in only between registers; 0F 12 / 16 between registers
	// are VMOVHLPS / VMOVLHPS.
	if (map == 1 && pp >= 2 && (opcode == 0x10 || opcode == 0x11) && memory) info.nds = false;
	if (map == 1 && pp == 0 && !memory && (opcode == 0x12 || opcode == 0x16)) info.text = (opcode == 0x12) ? "VMOVHLPS" : "VMOVLHPS";
	// W1 gathers move qwords: a dword index then fills only an XMM, a qword index a full register.
	if (map == 2 && simd.W && opcode >= 0x90 && opcode <= 0x93) {
		if (opcode & 1) info.operands[0].size = info.operands[2].size = SIZE::None;
		else info.operands[1].size = SIZE::dq;
	}

	if (info.groupNumber <= 0) return info;
	return merge(info, twoByteGroupTable()[(info.groupNumber - 8) * 8 + ((modRM >> 3) & 0x07)]);
}

} // namespace voidwalk
