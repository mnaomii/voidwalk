#pragma once
#include "arch/instruction.hpp"
#include "arch/x86_64/x86_64_mnemonic.hpp"
#include <string>
#include <string_view>
#include <cstdint>
#include <format>

namespace voidwalk {

// One decoded x86 / x86-64 instruction.
//
// Split of responsibility with X86Decoder: the decoder eats the byte stream and
// resolves everything - the table row, the prefixes still in force, the operand
// and address widths, every field's value - into one Fields. This class only
// formats that into the two strings Instruction holds; it re-reads nothing and
// re-derives nothing.
class x86_64: public Instruction{
public:

	using ADDRESSING = x86_64_Mnemonic::ADDRESSING;
	using SIZE = x86_64_Mnemonic::SIZE;
	using TableOperand = x86_64_Mnemonic::TableOperand;

	// Everything X86Decoder learned about one instruction.
	struct Fields {
		uint8_t bytes[15]{};     // every byte consumed, in stream order
		uint8_t length = 0;
		bool is64Bit = false;
		bool isInvalid = false;  // gone in long mode, or over 15 bytes: renders "(bad)"

		// Legacy prefixes still in force once the opcode has taken its mandatory one.
		bool hasLock = false;
		uint8_t repPrefix = 0;       // F2 / F3, or 0
		uint8_t segmentPrefix = 0;   // the last segment override, or 0
		uint8_t rexPrefix = 0;       // the REX right before the opcode, or 0

		unsigned opSize = 32;    // effective operand size in bits: 16 / 32 / 64
		unsigned addrSize = 32;  // effective address size in bits: 16 / 32 / 64

		// Opcode map, prefix column and VEX state are in the Simd that comes with these.
		// VEX's R / X / B / W are also folded into rexPrefix in long mode.
		uint8_t opcode = 0;
		x86_64_Mnemonic::OpcodeInfo info{};   // the resolved row

		bool hasModRM = false;
		uint8_t mod = 0, reg = 0, rm = 0;         // reg / rm with REX.R / REX.B folded in
		bool hasSIB = false;
		uint8_t scale = 1, index = 0, base = 0;   // index / base with REX.X / REX.B folded in
		bool hasDisplacement = false;
		int64_t displacement = 0;

		// The I / J / A / O operands, in order. A J is already its absolute target, an
		// immediate is already sign-extended where the CPU extends it, and an A is
		// segment << 32 | offset.
		uint64_t immediates[2]{};
	};

// Renders `fields` into machineCode and instructionText.
//
// Never throws and never fails: anything the decoder rejected renders as "(bad)"
// plus the bytes it consumed, so a sweep always gets a row back.
explicit x86_64(const Fields& fields, const x86_64_Mnemonic::Simd& simd) {
	for (uint8_t i = 0; i < fields.length; ++i)
		machineCode += std::format("{:02x} ", fields.bytes[i]);

	const auto& info = fields.info;
	if (fields.isInvalid || info.text.empty() || info.text == "(bad)") {
		instructionText = "(bad)";
		return;
	}

	auto registerName = [&](unsigned regNumber, unsigned width) { return x86_64_Mnemonic::registerOf(regNumber, width, fields.rexPrefix != 0); };
	auto widthOf = [&](SIZE size) -> unsigned {
		switch (size) {
		case SIZE::b: return 8;
		case SIZE::w: return 16;
		case SIZE::d: return 32;
		case SIZE::z: return fields.opSize == 16 ? 16 : 32;
		case SIZE::y: return fields.opSize == 64 ? 64 : 32;
		default:      return fields.opSize;
		}
	};
	auto toHex = [](uint64_t value) { return std::format("{:#x}", value); };
	// An XMM, or a YMM under VEX.L unless the operand is sized dq (always XMM).
	auto vectorName = [&](unsigned regNumber, SIZE size) { return (simd.L && size != SIZE::dq ? "YMM" : "XMM") + std::to_string(regNumber); };

	const uint8_t regField = fields.reg & 7;
	//checks whether the instruction is an x86 indirect near CALL or JMP
	const bool isIndirect = simd.map == 0 && fields.opcode == 0xFF && (regField == 2 || regField == 4);
	const bool isBranch = isIndirect
		|| (simd.map == 0 && ((fields.opcode >= 0x70 && fields.opcode <= 0x7F) || fields.opcode == 0xC2 || fields.opcode == 0xC3
		                   || fields.opcode == 0xE8 || fields.opcode == 0xE9 || fields.opcode == 0xEB))
		|| (simd.map == 1 && fields.opcode >= 0x80 && fields.opcode <= 0x8F);
	const bool hasNotrack = isIndirect && fields.segmentPrefix == 0x3E;   // CET: indirect branch exempt from ENDBR

	// A segment override shows on the memory operand it applies to. One that nothing
	// used (a branch hint, say) is printed as a prefix word instead, as objdump does.
	const std::string segmentText = (fields.segmentPrefix && !hasNotrack) ? std::string(x86_64_Mnemonic::prefixTable()[fields.segmentPrefix]) : "";
	bool segmentUsed = false;


	// The one memory operand, if any: [base + index*scale +/- disp]. Whichever operand asks
	// for E/M/W/Q/VSIB takes this text; with mod 11 there is no memory and those are registers.
	// A VSIB (gather) indexes with a vector register instead.
	const TableOperand* vsib = nullptr;
	for (const auto& operand : info.operands)
		if (operand.addressingMode == ADDRESSING::VSIB) vsib = &operand;
	std::string memoryOperand;
	if (fields.hasModRM && fields.mod != 3) {
		const uint8_t rmField = fields.rm & 7;
		if (fields.addrSize == 16) {
			static constexpr std::string_view modes16[] = { "BX + SI","BX + DI","BP + SI","BP + DI","SI","DI","BP","BX" };
			if (!(fields.mod == 0 && rmField == 6)) memoryOperand = modes16[rmField];
		}
		else if (fields.hasSIB) {
			if (!(fields.mod == 0 && (fields.base & 7) == 5)) memoryOperand = registerName(fields.base, fields.addrSize);
			if (fields.index != 4 || vsib)   // 100 without REX.X: no index - in a VSIB it is XMM4 / YMM4
				memoryOperand += (memoryOperand.empty() ? "" : " + ")
				              + (vsib ? vectorName(fields.index, vsib->size) : registerName(fields.index, fields.addrSize))
				              + "*" + std::to_string(fields.scale);
		}
		else if (fields.mod == 0 && rmField == 5) {
			if (fields.is64Bit) memoryOperand = (fields.addrSize == 64) ? "RIP" : "EIP";
		}
		else memoryOperand = registerName(fields.rm, fields.addrSize);

		// Off a register (RIP included) the displacement is signed. With nothing to be
		// relative to it is an absolute address: unsigned, at the address width.
		if (fields.hasDisplacement) {
			const uint64_t mask = (fields.addrSize == 64) ? ~0ull : (1ull << fields.addrSize) - 1;
			memoryOperand += memoryOperand.empty() ? toHex(static_cast<uint64_t>(fields.displacement) & mask)
			        : (fields.displacement < 0) ? " - " + toHex(static_cast<uint64_t>(-fields.displacement))
			        : " + " + toHex(static_cast<uint64_t>(fields.displacement));
		}
		memoryOperand = "[" + memoryOperand + "]";
	}
	auto takeMemoryOperand = [&] { segmentUsed = true; return segmentText + memoryOperand; };


	// The row's operands, plus - for an nds row - VEX.vvvv right after the first one, in
	// that operand's register class and width.
	TableOperand list[4]{};
	int count = 0;
	for (int i = 0; i < 3 && info.operands[i].addressingMode != ADDRESSING::None; ++i) {
		list[count++] = info.operands[i];
		if (i == 0 && info.nds)
			list[count++] = { ADDRESSING::H, (info.operands[0].addressingMode == ADDRESSING::G) ? SIZE::y : info.operands[0].size, "" };
	}

	std::string operands;
	int immIndex = 0;
	for (int i = 0; i < count; ++i) {
		const auto& operand = list[i];
		std::string operandText;
		switch (operand.addressingMode) {
		case ADDRESSING::E:
		case ADDRESSING::M: operandText = (fields.mod == 3) ? registerName(fields.rm, widthOf(operand.size)) : takeMemoryOperand(); break;
		case ADDRESSING::G: operandText = registerName(fields.reg, widthOf(operand.size)); break;
		case ADDRESSING::V: operandText = vectorName(fields.reg, operand.size); break;
		case ADDRESSING::W: operandText = (fields.mod == 3) ? vectorName(fields.rm, operand.size) : takeMemoryOperand(); break;
		case ADDRESSING::H: operandText = (operand.size == SIZE::y) ? registerName(simd.vvvv, widthOf(operand.size)) : vectorName(simd.vvvv, operand.size); break;
		case ADDRESSING::L: operandText = vectorName((fields.immediates[immIndex++] >> 4) & (fields.is64Bit ? 15 : 7), operand.size); break;
		case ADDRESSING::VSIB: operandText = takeMemoryOperand(); break;
		case ADDRESSING::P: operandText = "MM" + std::to_string(regField); break;
		case ADDRESSING::Q: operandText = (fields.mod == 3) ? "MM" + std::to_string(fields.rm & 7) : takeMemoryOperand(); break;
		case ADDRESSING::S: operandText = x86_64_Mnemonic::segmentOf(fields.reg); break;
		case ADDRESSING::Z: operandText = registerName((fields.opcode & 7) | (fields.rexPrefix & 1) << 3, widthOf(operand.size)); break;   // +r, REX.B extends
		case ADDRESSING::I:
		case ADDRESSING::J: operandText = toHex(fields.immediates[immIndex++]); break;
		case ADDRESSING::O: segmentUsed = true; operandText = segmentText + "[" + toHex(fields.immediates[immIndex++]) + "]"; break;
		case ADDRESSING::A: operandText = toHex(fields.immediates[immIndex] >> 32) + ":" + toHex(fields.immediates[immIndex] & 0xFFFFFFFF); ++immIndex; break;
		case ADDRESSING::X: segmentUsed = true; operandText = segmentText + "[" + registerName(6, fields.addrSize) + "]"; break;   // [rSI]
		case ADDRESSING::Y: operandText = "[" + registerName(7, fields.addrSize) + "]"; break;                                // [rDI], always ES
		case ADDRESSING::eAX: operandText = registerName(0, widthOf(operand.size)); break;
		case ADDRESSING::eCX: case ADDRESSING::eDX: case ADDRESSING::eBX: case ADDRESSING::eSP:
		case ADDRESSING::eBP: case ADDRESSING::eSI: case ADDRESSING::eDI:
			operandText = registerName(static_cast<unsigned>(operand.addressingMode) - static_cast<unsigned>(ADDRESSING::eCX) + 1, widthOf(operand.size));
			break;
		default: operandText = operand.value; break;   // named outright: AL, DX, CL, 1, ES, ST(i)...
		}
		operands += (i ? ", " : "") + operandText;
	}


	// Control flow, so frontends need not re-parse the text for it.
	const uint8_t opcode = fields.opcode;
	if (simd.map == 0) {
		if (opcode == 0xE8 || opcode == 0x9A || (opcode == 0xFF && (regField == 2 || regField == 3)))                            flow_ = Flow::Call;
		else if (opcode == 0xE9 || opcode == 0xEB || opcode == 0xEA || (opcode == 0xFF && (regField == 4 || regField == 5)))    flow_ = Flow::Jump;
		else if ((opcode >= 0x70 && opcode <= 0x7F) || (opcode >= 0xE0 && opcode <= 0xE3))                                   flow_ = Flow::CondJump;
		else if (opcode == 0xC2 || opcode == 0xC3 || opcode == 0xCA || opcode == 0xCB || opcode == 0xCF)                      flow_ = Flow::Return;
	}
	else if (simd.map == 1 && opcode >= 0x80 && opcode <= 0x8F) flow_ = Flow::CondJump;
	if (flow_ != Flow::None && info.operands[0].addressingMode == ADDRESSING::J) target_ = fields.immediates[0];

	std::string_view mnemonic = ((fields.opSize == 64 || simd.W) && !info.text64.empty()) ? info.text64
	                      : (fields.opSize == 16 && !info.text16.empty()) ? info.text16 : info.text;
	if (simd.map == 0 && fields.opcode == 0xE3)   // the count register follows the address size
		mnemonic = (fields.addrSize == 64) ? "JRCXZ" : (fields.addrSize == 32) ? "JECXZ" : "JCXZ";

	if (fields.hasLock) instructionText += "LOCK ";
	if (fields.repPrefix) instructionText += (fields.repPrefix == 0xF3) ? "REP " : isBranch ? "BND " : "REPNE ";
	if (hasNotrack) instructionText += "NOTRACK ";
	if (!segmentText.empty() && !segmentUsed) instructionText += segmentText.substr(0, 2) + " ";
	instructionText += mnemonic;
	instructionText += " \t";
	instructionText += operands;
}

	inline std::string& decodeLineString() override {
		return instructionText;
	}

	inline std::string& getMachineCode() override {
		return machineCode;
	}

};

} // namespace voidwalk
