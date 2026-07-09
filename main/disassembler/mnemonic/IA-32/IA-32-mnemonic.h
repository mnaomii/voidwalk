#include <cstdint>
#pragma once

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
	SP = 0x04, // also AH
	BP = 0x05, // also CH
	SI = 0x06, // also DH
	DI = 0x07 // also BH
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

