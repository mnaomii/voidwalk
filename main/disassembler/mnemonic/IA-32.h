#include "../../address-space/address_space.h"
#include "../ELF/elf_disassembler.h"
#include "instruction.h"
#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <format>
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
	enum class STATE : int {
		IS_IMMEDIATE = 2,
		IS_MEMORY = 1,
		IS_REGISTER = 0,
		IS_NULL = -1
	};
private:

	uint16_t prefix, opcode;
	struct {
		uint32_t value;
		STATE state;
	} op1, op2;

	std::string instructionStr;


public:


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

enum class OPCODE : uint16_t {
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
	JNS =0x79,
	JP = 0x7A,
	JNP = 0x7B,
	JL = 0x7C,
	JNL = 0x7D,
	JLE = 0x7E,
	JNLE = 0x7F,


	ADD_EbIb = 0x80,
	ADD_EvIv = 0x81,
	SUB_EbIb = 0x82,
	SUB_EvIb = 0x83,
	
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
	JCXZ_Jb = 0xE3,
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



// addressing method (where an operand comes from) + its size, mirroring the
// Intel Eb/Gv/Iz notation already baked into the OPCODE enum names.
//   E=modrm r/m, G=modrm reg, I=imm, J=rel offset, O=moffs, S=seg reg, M=memory,
//   A=far ptr, Z=register in low 3 bits of opcode (+r), AL/eAX/DX/CL/One=implicit
enum class Am : uint8_t { None, E, G, I, J, O, S, M, A, Z, AL, eAX, DX, CL, One };
enum class Ot : uint8_t { None, b, w, v, z, p, a };   // b=8 w=16 v=16/32 z=imm16/32 p=far a=bound

struct Operand {
	Am am = Am::None;
	Ot ot = Ot::None;
};

struct OpInfo {
	std::string_view text;
	bool hasRMbyte;
	bool hasImmediateByte;
	std::array<Operand, 3> op{};    // up to 3 operands; {None,None} = unused slot
};

static constexpr std::array<OpInfo, 256> buildOpcodes() {
	std::array<OpInfo, 256> n{};              

#define M(name, txt) n[static_cast<uint16_t>(OPCODE::name)].text = txt

	M(ADD_EbGb,"add"); M(ADD_EvGv,"add"); M(ADD_GbEb,"add"); M(ADD_GvEv,"add"); M(ADD_ALIb,"add"); M(ADD_eAXIv,"add");
	M(PUSH_ES,"push"); M(POP_ES,"pop");
	M(OR_EbGb,"or"); M(OR_EvGv,"or"); M(OR_GbEb,"or"); M(OR_GvEv,"or"); M(OR_ALIb,"or"); M(OR_eAXIv,"or");
	M(PUSH_CS,"push");
	M(ADC_EbGb,"adc"); M(ADC_EvGv,"adc"); M(ADC_GbEb,"adc"); M(ADC_GvEv,"adc"); M(ADC_ALIb,"adc"); M(ADC_eAXIv,"adc");
	M(PUSH_SS,"push"); M(POP_SS,"pop");
	M(SBB_EbGb,"sbb"); M(SBB_EvGv,"sbb"); M(SBB_GbEb,"sbb"); M(SBB_GvEv,"sbb"); M(SBB_ALIb,"sbb"); M(SBB_eAXIv,"sbb");
	M(PUSH_DS,"push"); M(POP_DS,"pop");
	M(AND_EbGb,"and"); M(AND_EvGv,"and"); M(AND_GbEb,"and"); M(AND_GvEv,"and"); M(AND_ALIb,"and"); M(AND_eAXIv,"and");
	M(DAA,"daa");
	M(SUB_EbGb,"sub"); M(SUB_EvGv,"sub"); M(SUB_GbEb,"sub"); M(SUB_GvEv,"sub"); M(SUB_ALIb,"sub"); M(SUB_eAXIv,"sub");
	M(DAS,"das");
	M(XOR_EbGb,"xor"); M(XOR_EvGv,"xor"); M(XOR_GbEb,"xor"); M(XOR_GvEv,"xor"); M(XOR_ALIb,"xor"); M(XOR_eAXIv,"xor");
	M(AAA,"aaa");
	M(CMP_EbGb,"cmp"); M(CMP_EvGv,"cmp"); M(CMP_GbEb,"cmp"); M(CMP_GvEv,"cmp"); M(CMP_ALIb,"cmp"); M(CMP_eAXIv,"cmp");
	M(AAS,"aas");

	M(INC_eAX,"inc"); M(INC_eCX,"inc"); M(INC_eDX,"inc"); M(INC_eBX,"inc"); M(INC_eSP,"inc"); M(INC_eBP,"inc"); M(INC_eSI,"inc"); M(INC_eDI,"inc");
	M(DEC_eAX,"dec"); M(DEC_eCX,"dec"); M(DEC_eDX,"dec"); M(DEC_eBX,"dec"); M(DEC_eSP,"dec"); M(DEC_eBP,"dec"); M(DEC_eSI,"dec"); M(DEC_eDI,"dec");

	M(PUSH_eAX,"push"); M(PUSH_eCX,"push"); M(PUSH_eDX,"push"); M(PUSH_eBX,"push"); M(PUSH_eSP,"push"); M(PUSH_eBP,"push"); M(PUSH_eSI,"push"); M(PUSH_eDI,"push");
	M(POP_eAX,"pop"); M(POP_eCX,"pop"); M(POP_eDX,"pop"); M(POP_eBX,"pop"); M(POP_eSP,"pop"); M(POP_eBP,"pop"); M(POP_eSI,"pop"); M(POP_eDI,"pop");

	M(PUSHA,"pusha"); M(POPA,"popa"); M(BOUND_GvMa,"bound"); M(ARPL_EwGw,"arpl");
	M(PUSH_Iv,"push"); M(IMUL_GvEvIv,"imul"); M(PUSH_Ib,"push"); M(IMUL_GvEvIb,"imul");
	M(INSB_YbDX,"insb"); M(INSW_YzDX,"insw"); M(OUTSB_DXXb,"outsb"); M(OUTSW_DXXv,"outsw");

	M(JO,"jo"); M(JNO,"jno"); M(JB,"jb"); M(JNB,"jnb"); M(JZ,"jz"); M(JNZ,"jnz"); M(JBE,"jbe"); M(JA,"ja");
	M(JS,"js"); M(JNS,"jns"); M(JP,"jp"); M(JNP,"jnp"); M(JL,"jl"); M(JNL,"jnl"); M(JLE,"jle"); M(JNLE,"jnle");


	M(ADD_EbIb,"add"); M(ADD_EvIv,"add"); M(SUB_EbIb,"sub"); M(SUB_EvIb,"sub");
	M(TEST_EbGb,"test"); M(TEST_EvGv,"test"); M(XCHG_EbGb,"xchg"); M(XCHG_EvGv,"xchg");
	M(MOV_EbGb,"mov"); M(MOV_EvGv,"mov"); M(MOV_GbEb,"mov"); M(MOV_GvEv,"mov"); M(MOV_EwSw,"mov");
	M(LEA_GvM,"lea"); M(MOV_SwEw,"mov"); M(POP_Ev,"pop");

	M(NOP,"nop"); M(XCHG_eAXeCX,"xchg"); M(XCHG_eAXeDX,"xchg"); M(XCHG_eAXeBX,"xchg"); M(XCHG_eAXeSP,"xchg"); M(XCHG_eAXeBP,"xchg"); M(XCHG_eAXeSI,"xchg"); M(XCHG_eAXeDI,"xchg");
	M(CBW,"cbw"); M(CWD,"cwd"); M(CALL_Ap,"call"); M(FWAIT,"fwait"); M(PUSHF_Fv,"pushf"); M(POPF_Fv,"popf"); M(SAHF,"sahf"); M(LAHF,"lahf");

	M(MOV_ALOb,"mov"); M(MOV_eAXOv,"mov"); M(MOV_ObAL,"mov"); M(MOV_OveAX,"mov");
	M(MOVSB_XbYb,"movsb"); M(MOVSW_XvYv,"movsw"); M(CMPSB_XbYb,"cmpsb"); M(CMPSW_XvYv,"cmpsw");
	M(TEST_ALIb,"test"); M(TEST_eAXIv,"test");
	M(STOSB_YbAL,"stosb"); M(STOSW_YveAX,"stosw"); M(LODSB_ALXb,"lodsb"); M(LODSW_eAXXv,"lodsw"); M(SCASB_ALYb,"scasb"); M(SCASW_eAXYv,"scasw");

	M(MOV_ALIb,"mov"); M(MOV_CLIb,"mov"); M(MOV_DLIb,"mov"); M(MOV_BLIb,"mov"); M(MOV_AHIb,"mov"); M(MOV_CHIb,"mov"); M(MOV_DHIb,"mov"); M(MOV_BHIb,"mov");
	M(MOV_eAXIv,"mov"); M(MOV_eCXIv,"mov"); M(MOV_eDXIv,"mov"); M(MOV_eBXIv,"mov"); M(MOV_eSPIv,"mov"); M(MOV_eBPIv,"mov"); M(MOV_eSIIv,"mov"); M(MOV_eDIIv,"mov");

	M(RET_Iw,"ret"); M(RET,"ret"); M(LES_GvMp,"les"); M(LDS_GvMp,"lds"); M(MOV_EbIb,"mov"); M(MOV_EvIv,"mov");
	M(ENTER_IwIb,"enter"); M(LEAVE,"leave"); M(RETF_Iw,"retf"); M(RETF,"retf");
	M(INT3,"int3"); M(INT_Ib,"int"); M(INTO,"into"); M(IRET,"iret");

	M(AAM_Ib,"aam"); M(AAD_Ib,"aad"); M(SALC,"salc"); M(XLAT,"xlat");

	M(LOOPNZ_Jb,"loopnz"); M(LOOPZ_Jb,"loopz"); M(LOOP_Jb,"loop"); M(JCXZ_Jb,"jcxz");
	M(IN_ALIb,"in"); M(IN_eAXIb,"in"); M(OUT_IbAL,"out"); M(OUT_IbeAX,"out");
	M(CALL_Jv,"call"); M(JMP_Jv,"jmp"); M(JMP_Ap,"jmp"); M(JMP_Jb,"jmp");
	M(IN_ALDX,"in"); M(IN_eAXDX,"in"); M(OUT_DXAL,"out"); M(OUT_DXeAX,"out");


	M(INT1,"int1"); M(HLT,"hlt"); M(CMC,"cmc");
	M(CLC,"clc"); M(STC,"stc"); M(CLI,"cli"); M(STI,"sti"); M(CLD,"cld"); M(STD,"std");

#undef M

#define RM(name) n[static_cast<uint16_t>(OPCODE::name)].hasRMbyte = true
	RM(ADD_EbGb); RM(ADD_EvGv); RM(ADD_GbEb); RM(ADD_GvEv);
	RM(OR_EbGb);  RM(OR_EvGv);  RM(OR_GbEb);  RM(OR_GvEv);
	RM(ADC_EbGb); RM(ADC_EvGv); RM(ADC_GbEb); RM(ADC_GvEv);
	RM(SBB_EbGb); RM(SBB_EvGv); RM(SBB_GbEb); RM(SBB_GvEv);
	RM(AND_EbGb); RM(AND_EvGv); RM(AND_GbEb); RM(AND_GvEv);
	RM(SUB_EbGb); RM(SUB_EvGv); RM(SUB_GbEb); RM(SUB_GvEv);
	RM(XOR_EbGb); RM(XOR_EvGv); RM(XOR_GbEb); RM(XOR_GvEv);
	RM(CMP_EbGb); RM(CMP_EvGv); RM(CMP_GbEb); RM(CMP_GvEv);
	RM(BOUND_GvMa); RM(ARPL_EwGw); RM(IMUL_GvEvIv); RM(IMUL_GvEvIb);
	RM(ADD_EbIb); RM(ADD_EvIv); RM(SUB_EbIb); RM(SUB_EvIb);
	RM(TEST_EbGb); RM(TEST_EvGv); RM(XCHG_EbGb); RM(XCHG_EvGv);
	RM(MOV_EbGb); RM(MOV_EvGv); RM(MOV_GbEb); RM(MOV_GvEv); RM(MOV_EwSw); RM(LEA_GvM); RM(MOV_SwEw); RM(POP_Ev);
	RM(GRP2_EbIb); RM(GRP2_EvIb); RM(LES_GvMp); RM(LDS_GvMp); RM(MOV_EbIb); RM(MOV_EvIv);
	RM(GRP2_Eb1); RM(GRP2_Ev1); RM(GRP2_EbCL); RM(GRP2_EvCL);
	RM(ESC0); RM(ESC1); RM(ESC2); RM(ESC3); RM(ESC4); RM(ESC5); RM(ESC6); RM(ESC7);
	RM(GRP3_Eb); RM(GRP3_Ev); RM(GRP4); RM(GRP5);
#undef RM

#define IMM(name) n[static_cast<uint16_t>(OPCODE::name)].hasImmediateByte = true
	IMM(ADD_ALIb); IMM(ADD_eAXIv); IMM(OR_ALIb);  IMM(OR_eAXIv);
	IMM(ADC_ALIb); IMM(ADC_eAXIv); IMM(SBB_ALIb); IMM(SBB_eAXIv);
	IMM(AND_ALIb); IMM(AND_eAXIv); IMM(SUB_ALIb); IMM(SUB_eAXIv);
	IMM(XOR_ALIb); IMM(XOR_eAXIv); IMM(CMP_ALIb); IMM(CMP_eAXIv);
	IMM(PUSH_Iv); IMM(PUSH_Ib); IMM(IMUL_GvEvIv); IMM(IMUL_GvEvIb);
	IMM(ADD_EbIb); IMM(ADD_EvIv); IMM(SUB_EbIb); IMM(SUB_EvIb);
	IMM(TEST_ALIb); IMM(TEST_eAXIv);
	IMM(MOV_ALIb); IMM(MOV_CLIb); IMM(MOV_DLIb); IMM(MOV_BLIb); IMM(MOV_AHIb); IMM(MOV_CHIb); IMM(MOV_DHIb); IMM(MOV_BHIb);
	IMM(MOV_eAXIv); IMM(MOV_eCXIv); IMM(MOV_eDXIv); IMM(MOV_eBXIv); IMM(MOV_eSPIv); IMM(MOV_eBPIv); IMM(MOV_eSIIv); IMM(MOV_eDIIv);
	IMM(GRP2_EbIb); IMM(GRP2_EvIb);
	IMM(RET_Iw); IMM(MOV_EbIb); IMM(MOV_EvIv); IMM(ENTER_IwIb); IMM(RETF_Iw); IMM(INT_Ib);
	IMM(AAM_Ib); IMM(AAD_Ib);
	IMM(IN_ALIb); IMM(IN_eAXIb); IMM(OUT_IbAL); IMM(OUT_IbeAX);
#undef IMM

	// --- operands, for printing.  op[i] = { addressing method, size } ---
	{
		using O = Operand;
		constexpr O _{},
			Eb{Am::E,Ot::b}, Ev{Am::E,Ot::v}, Ew{Am::E,Ot::w},
			Gb{Am::G,Ot::b}, Gv{Am::G,Ot::v}, Gw{Am::G,Ot::w},
			Ib{Am::I,Ot::b}, Iw{Am::I,Ot::w}, Iz{Am::I,Ot::z},
			Jb{Am::J,Ot::b}, Jz{Am::J,Ot::z},
			Ob{Am::O,Ot::b}, Ov{Am::O,Ot::v}, Sw{Am::S,Ot::w},
			Mv{Am::M,Ot::v}, Ma{Am::M,Ot::a}, Mp{Am::M,Ot::p}, Ap{Am::A,Ot::p},
			Zb{Am::Z,Ot::b}, Zv{Am::Z,Ot::v},
			AL{Am::AL,Ot::b}, eAX{Am::eAX,Ot::v}, DX{Am::DX,Ot::w}, CL{Am::CL,Ot::b}, c1{Am::One,Ot::b};

#define OPS(name, a, b, c) n[static_cast<uint16_t>(OPCODE::name)].op = { a, b, c }
		OPS(ADD_EbGb,Eb,Gb,_); OPS(ADD_EvGv,Ev,Gv,_); OPS(ADD_GbEb,Gb,Eb,_); OPS(ADD_GvEv,Gv,Ev,_); OPS(ADD_ALIb,AL,Ib,_); OPS(ADD_eAXIv,eAX,Iz,_);
		OPS(OR_EbGb,Eb,Gb,_); OPS(OR_EvGv,Ev,Gv,_); OPS(OR_GbEb,Gb,Eb,_); OPS(OR_GvEv,Gv,Ev,_); OPS(OR_ALIb,AL,Ib,_); OPS(OR_eAXIv,eAX,Iz,_);
		OPS(ADC_EbGb,Eb,Gb,_); OPS(ADC_EvGv,Ev,Gv,_); OPS(ADC_GbEb,Gb,Eb,_); OPS(ADC_GvEv,Gv,Ev,_); OPS(ADC_ALIb,AL,Ib,_); OPS(ADC_eAXIv,eAX,Iz,_);
		OPS(SBB_EbGb,Eb,Gb,_); OPS(SBB_EvGv,Ev,Gv,_); OPS(SBB_GbEb,Gb,Eb,_); OPS(SBB_GvEv,Gv,Ev,_); OPS(SBB_ALIb,AL,Ib,_); OPS(SBB_eAXIv,eAX,Iz,_);
		OPS(AND_EbGb,Eb,Gb,_); OPS(AND_EvGv,Ev,Gv,_); OPS(AND_GbEb,Gb,Eb,_); OPS(AND_GvEv,Gv,Ev,_); OPS(AND_ALIb,AL,Ib,_); OPS(AND_eAXIv,eAX,Iz,_);
		OPS(SUB_EbGb,Eb,Gb,_); OPS(SUB_EvGv,Ev,Gv,_); OPS(SUB_GbEb,Gb,Eb,_); OPS(SUB_GvEv,Gv,Ev,_); OPS(SUB_ALIb,AL,Ib,_); OPS(SUB_eAXIv,eAX,Iz,_);
		OPS(XOR_EbGb,Eb,Gb,_); OPS(XOR_EvGv,Ev,Gv,_); OPS(XOR_GbEb,Gb,Eb,_); OPS(XOR_GvEv,Gv,Ev,_); OPS(XOR_ALIb,AL,Ib,_); OPS(XOR_eAXIv,eAX,Iz,_);
		OPS(CMP_EbGb,Eb,Gb,_); OPS(CMP_EvGv,Ev,Gv,_); OPS(CMP_GbEb,Gb,Eb,_); OPS(CMP_GvEv,Gv,Ev,_); OPS(CMP_ALIb,AL,Ib,_); OPS(CMP_eAXIv,eAX,Iz,_);

		OPS(INC_eAX,Zv,_,_); OPS(INC_eCX,Zv,_,_); OPS(INC_eDX,Zv,_,_); OPS(INC_eBX,Zv,_,_); OPS(INC_eSP,Zv,_,_); OPS(INC_eBP,Zv,_,_); OPS(INC_eSI,Zv,_,_); OPS(INC_eDI,Zv,_,_);
		OPS(DEC_eAX,Zv,_,_); OPS(DEC_eCX,Zv,_,_); OPS(DEC_eDX,Zv,_,_); OPS(DEC_eBX,Zv,_,_); OPS(DEC_eSP,Zv,_,_); OPS(DEC_eBP,Zv,_,_); OPS(DEC_eSI,Zv,_,_); OPS(DEC_eDI,Zv,_,_);
		OPS(PUSH_eAX,Zv,_,_); OPS(PUSH_eCX,Zv,_,_); OPS(PUSH_eDX,Zv,_,_); OPS(PUSH_eBX,Zv,_,_); OPS(PUSH_eSP,Zv,_,_); OPS(PUSH_eBP,Zv,_,_); OPS(PUSH_eSI,Zv,_,_); OPS(PUSH_eDI,Zv,_,_);
		OPS(POP_eAX,Zv,_,_); OPS(POP_eCX,Zv,_,_); OPS(POP_eDX,Zv,_,_); OPS(POP_eBX,Zv,_,_); OPS(POP_eSP,Zv,_,_); OPS(POP_eBP,Zv,_,_); OPS(POP_eSI,Zv,_,_); OPS(POP_eDI,Zv,_,_);

		OPS(BOUND_GvMa,Gv,Ma,_); OPS(ARPL_EwGw,Ew,Gw,_);
		OPS(PUSH_Iv,Iz,_,_); OPS(IMUL_GvEvIv,Gv,Ev,Iz); OPS(PUSH_Ib,Ib,_,_); OPS(IMUL_GvEvIb,Gv,Ev,Ib);

		OPS(JO,Jb,_,_); OPS(JNO,Jb,_,_); OPS(JB,Jb,_,_); OPS(JNB,Jb,_,_); OPS(JZ,Jb,_,_); OPS(JNZ,Jb,_,_); OPS(JBE,Jb,_,_); OPS(JA,Jb,_,_);
		OPS(JS,Jb,_,_); OPS(JNS,Jb,_,_); OPS(JP,Jb,_,_); OPS(JNP,Jb,_,_); OPS(JL,Jb,_,_); OPS(JNL,Jb,_,_); OPS(JLE,Jb,_,_); OPS(JNLE,Jb,_,_);

		OPS(ADD_EbIb,Eb,Ib,_); OPS(ADD_EvIv,Ev,Iz,_); OPS(SUB_EbIb,Eb,Ib,_); OPS(SUB_EvIb,Ev,Ib,_);
		OPS(TEST_EbGb,Eb,Gb,_); OPS(TEST_EvGv,Ev,Gv,_); OPS(XCHG_EbGb,Eb,Gb,_); OPS(XCHG_EvGv,Ev,Gv,_);
		OPS(MOV_EbGb,Eb,Gb,_); OPS(MOV_EvGv,Ev,Gv,_); OPS(MOV_GbEb,Gb,Eb,_); OPS(MOV_GvEv,Gv,Ev,_); OPS(MOV_EwSw,Ew,Sw,_);
		OPS(LEA_GvM,Gv,Mv,_); OPS(MOV_SwEw,Sw,Ew,_); OPS(POP_Ev,Ev,_,_);

		OPS(XCHG_eAXeCX,eAX,Zv,_); OPS(XCHG_eAXeDX,eAX,Zv,_); OPS(XCHG_eAXeBX,eAX,Zv,_); OPS(XCHG_eAXeSP,eAX,Zv,_); OPS(XCHG_eAXeBP,eAX,Zv,_); OPS(XCHG_eAXeSI,eAX,Zv,_); OPS(XCHG_eAXeDI,eAX,Zv,_);
		OPS(CALL_Ap,Ap,_,_);

		OPS(MOV_ALOb,AL,Ob,_); OPS(MOV_eAXOv,eAX,Ov,_); OPS(MOV_ObAL,Ob,AL,_); OPS(MOV_OveAX,Ov,eAX,_);
		OPS(TEST_ALIb,AL,Ib,_); OPS(TEST_eAXIv,eAX,Iz,_);

		OPS(MOV_ALIb,Zb,Ib,_); OPS(MOV_CLIb,Zb,Ib,_); OPS(MOV_DLIb,Zb,Ib,_); OPS(MOV_BLIb,Zb,Ib,_); OPS(MOV_AHIb,Zb,Ib,_); OPS(MOV_CHIb,Zb,Ib,_); OPS(MOV_DHIb,Zb,Ib,_); OPS(MOV_BHIb,Zb,Ib,_);
		OPS(MOV_eAXIv,Zv,Iz,_); OPS(MOV_eCXIv,Zv,Iz,_); OPS(MOV_eDXIv,Zv,Iz,_); OPS(MOV_eBXIv,Zv,Iz,_); OPS(MOV_eSPIv,Zv,Iz,_); OPS(MOV_eBPIv,Zv,Iz,_); OPS(MOV_eSIIv,Zv,Iz,_); OPS(MOV_eDIIv,Zv,Iz,_);

		OPS(GRP2_EbIb,Eb,Ib,_); OPS(GRP2_EvIb,Ev,Ib,_);
		OPS(RET_Iw,Iw,_,_); OPS(LES_GvMp,Gv,Mp,_); OPS(LDS_GvMp,Gv,Mp,_); OPS(MOV_EbIb,Eb,Ib,_); OPS(MOV_EvIv,Ev,Iz,_);
		OPS(ENTER_IwIb,Iw,Ib,_); OPS(RETF_Iw,Iw,_,_); OPS(INT_Ib,Ib,_,_);
		OPS(GRP2_Eb1,Eb,c1,_); OPS(GRP2_Ev1,Ev,c1,_); OPS(GRP2_EbCL,Eb,CL,_); OPS(GRP2_EvCL,Ev,CL,_);
		OPS(AAM_Ib,Ib,_,_); OPS(AAD_Ib,Ib,_,_);

		OPS(LOOPNZ_Jb,Jb,_,_); OPS(LOOPZ_Jb,Jb,_,_); OPS(LOOP_Jb,Jb,_,_); OPS(JCXZ_Jb,Jb,_,_);
		OPS(IN_ALIb,AL,Ib,_); OPS(IN_eAXIb,eAX,Ib,_); OPS(OUT_IbAL,Ib,AL,_); OPS(OUT_IbeAX,Ib,eAX,_);
		OPS(CALL_Jv,Jz,_,_); OPS(JMP_Jv,Jz,_,_); OPS(JMP_Ap,Ap,_,_); OPS(JMP_Jb,Jb,_,_);
		OPS(IN_ALDX,AL,DX,_); OPS(IN_eAXDX,eAX,DX,_); OPS(OUT_DXAL,DX,AL,_); OPS(OUT_DXeAX,DX,eAX,_);

		OPS(GRP3_Eb,Eb,_,_); OPS(GRP3_Ev,Ev,_,_); OPS(GRP4,Eb,_,_); OPS(GRP5,Ev,_,_);
#undef OPS
	}

	return n;
}

static const std::array<OpInfo, 256>& opcodeTable() {
	static constexpr std::array<OpInfo, 256> t = buildOpcodes();
	return t;
}

static std::string_view opcodeStrOf(uint8_t op)  { return opcodeTable()[op].text; }
static bool hasRMbyte(uint8_t op)                { return opcodeTable()[op].hasRMbyte; }
static bool hasImmediateByte(uint8_t op)         { return opcodeTable()[op].hasImmediateByte; }

// operand metadata for printing
static std::array<Operand, 3> operandsOf(uint8_t op) { return opcodeTable()[op].op; }

static uint8_t operandCount(uint8_t op) {
	uint8_t c = 0;
	for (const Operand& x : opcodeTable()[op].op) if (x.am != Am::None) ++c;
	return c;
}

// bytes an immediate occupies (0 if none). opsize16 = a 0x66 prefix was seen.
static uint8_t immSize(uint8_t op, bool opsize16 = false) {
	for (const Operand& x : opcodeTable()[op].op) {
		if (x.am != Am::I) continue;
		switch (x.ot) {
			case Ot::b: return 1;
			case Ot::w: return 2;
			case Ot::v:
			case Ot::z: return opsize16 ? 2 : 4;
			default:    return 0;
		}
	}
	return 0;
}


static constexpr std::array<std::string_view, 256> buildPrefixes() {
	std::array<std::string_view, 256> n{};

#define P(name, text) n[static_cast<uint16_t>(Prefix::name)] = text

	P(LOCK,"lock"); P(REPNE,"repne"); P(REP,"rep");                          
	P(CS,"cs"); P(SS,"ss"); P(DS,"ds"); P(ES,"es"); P(FS,"fs"); P(GS,"gs");  
	P(OPSIZE,"opsize");                                                      
	P(ADDRSIZE,"addrsize");                                                  

#undef P
	return n;
}

static const std::array<std::string_view, 256>& prefixTable() {
	static constexpr std::array<std::string_view, 256> prefix_str = buildPrefixes();
	return prefix_str;
}

static std::string_view prefixStrOf(uint8_t op) { return prefixTable()[op]; }
static bool isPrefix(uint8_t op) { return !prefixTable()[op].empty(); }


enum class REGISTER : uint16_t {
	AX = 0x00,
	CX = 0x01,
	DX = 0x02,
	BX = 0x03,
	SP = 0x04 , // also AH
	BP = 0x05 , // also CH
	SI = 0x06 , // also DH
	DI = 0x07 // also BH
};

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