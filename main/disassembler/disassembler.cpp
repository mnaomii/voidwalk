#include "disassembler.h"
#include "../address-space/address_space.h"
#include "miscellaneous/sections/base/header.h"
#include "mnemonic/IA-32/IA-32-instr.h"
#include "mnemonic/IA-32/IA-32-mnemonic.h"


using ADDRESSING = IA_32::ADDRESSING;
using SIZE = IA_32::SIZE;

void Disassembler::decode() {
	decodedInstructions.clear();
	instructionAddresses.clear();

	const uint64_t start = baseSections._text.getOffset();
	const uint64_t end = start + baseSections._text.getSize();

	uint64_t ptr = start;
	uint64_t vaddr = baseSections._text.getVaddr();

	while (ptr < end) {
		const uint64_t lineVaddr = vaddr;
		uint64_t next = decodeLine(ptr, vaddr);

		while (instructionAddresses.size() < decodedInstructions.size())
			instructionAddresses.push_back(lineVaddr);


		if (next <= ptr) break;

		vaddr += next - ptr;
		ptr = next;
	}
}

uint64_t Disassembler::decodeLine_IA_32(uint64_t address, uint64_t vaddr) {

	uint64_t field1 = 0, field2 = 0, field3 = 0, field4 = 0, field5 = 0, field6 = 0;
	uint64_t initAddress = address;

	field1 = contents.read_u8(address);
	if (!IA_32::isPrefix(field1)) // invalid prefix, just read opcode
	{
		field2 = field1;
		field1 = 0x00;
	}
	else { // must read opcode
		++address;

		field2 = contents.read_u8(address);
	}
	++address;

	field3 = 0x0;
	uint8_t mod = 0x0, reg_op = 0x0, rm = 0x0;
	if (IA_32::hasRMbyte(field2)) {
		field3 = contents.read_u8(address++); // getting ModR/M byte

		mod = ((field3 & 0b11000000) >> 6);
		reg_op = ((field3 & 0b00111000) >> 3);
		rm = (field3 & 0b00000111);

		if (mod != 0b11 && rm == 0b100)
			field4 = contents.read_u8(address++); // read sib;

		// A displacement is an offset from a register, so it is signed: disp8 0xFC is -4,
		// not +252. Widen it here - once it sits in an unsigned field the sign is gone
		// for good, and no cast downstream can bring it back.
		switch (mod) {
		case 0b01:
			field5 = static_cast<uint32_t>(static_cast<int8_t>(contents.read_u8(address++)));
			break;
		case 0b10:
			field5 = contents.read_u32(address);              // disp32: already the full width
			address += 4;
			break;
		case 0b00:
			if (rm == 0b101 || (rm == 0b100 && (field4 & 0b00000111) == 0b101)) {
				field5 = contents.read_u32(address);          // absolute address, no base register
				address += 4;
			}
			else field5 = 0x0;
			break;
		default:
			field5 = 0x0;
			break;
		}
	}


	// A group opcode (0x80-0x83, 0xC0/0xC1, 0xD0-0xD3, 0xF6/0xF7, 0xFE/0xFF) names its
	// instruction in ModRM.reg, and the 256-entry row it sits behind is only a placeholder with
	// empty operand fields - 0xF7's row claims no immediate at all, though F7 /0 is TEST Ev, Iz.
	// Resolve against the group table before deciding how many bytes this instruction eats:
	// read one too few and every instruction after this one starts on the wrong byte.
	const Instruction::OpcodeInfo info = IA_32::resolvedInfo(static_cast<uint32_t>(field2), reg_op);

	if (info.hasImmediateByte) { // set immediate field

#define cast(name) static_cast<uint8_t>(name)

		uint8_t immSize = 0;
		uint8_t immAddr = 0;
		field6 = 0x0;

		if (info.op1am == cast(ADDRESSING::I) || info.op1am == cast(ADDRESSING::J) || info.op1am == cast(ADDRESSING::A)) {
			immSize = info.op1s;
			immAddr = info.op1am;
		}
		else if (info.op2am == cast(ADDRESSING::I) || info.op2am == cast(ADDRESSING::J) || info.op2am == cast(ADDRESSING::A)) {
			immSize = info.op2s;
			immAddr = info.op2am;
		}
		else {
			immSize = info.op3s;
			immAddr = info.op3am;
		}

		const bool sixteenBit = (field1 == 0x66);
		const bool isRelative = (immAddr == static_cast<uint8_t>(ADDRESSING::J));

		uint32_t width = 0;   // bytes this immediate occupies in the stream
		switch (immSize) {
		case cast(SIZE::b): width = 1; break;
		case cast(SIZE::w): width = 2; break;
		case cast(SIZE::v):
		case cast(SIZE::z): width = sixteenBit ? 2 : 4; break;
		default:
			fprintf(stderr, "Immediate size not implemented yet (opcode %#x)\n", field2);
			break;
		}

		// A relative offset is signed - rel8 0xF0 is -16, a jump backwards. Every other
		// immediate (a value, a port, an absolute address) is just bits, so read it plain.
		int64_t value = 0;
		switch (width) {
		case 1:
			value = isRelative ? static_cast<int8_t>(contents.read_u8(address))
				: static_cast<int64_t>(contents.read_u8(address));
			break;
		case 2:
			value = isRelative ? static_cast<int16_t>(contents.read_u16(address))
				: static_cast<int64_t>(contents.read_u16(address));
			break;
		case 4:
			value = isRelative ? static_cast<int32_t>(contents.read_u32(address))
				: static_cast<int64_t>(contents.read_u32(address));
			break;
		}
		address += width;

		// rel is counted from the END of the instruction, so resolve the target only once
		// every byte of it has been consumed - address - initAddress is now its full length.
		field6 = isRelative
			? static_cast<uint32_t>(vaddr + (address - initAddress) + value)
			: static_cast<uint32_t>(value);
	}
	else field6 = 0x0;

#undef cast

	auto instruction = std::make_unique<IA_32>();
	instruction->decode(field1, field2, field3, field4, field5, field6);
	decodedInstructions.push_back(std::move(instruction));

	return address; // address where next instruction begins

}

uint64_t Disassembler::decodeLine(uint64_t address, uint64_t vaddr) {


	switch (this->architecture) {
	case 0x14c: {// x86 

		return decodeLine_IA_32(address, vaddr);
	}
	case 0x8664: {
		throw std::runtime_error("Not implemented yet.");
	}
	case 0xAA64: {
		throw std::runtime_error("Not implemented yet.");
	}
	case 0x1c0: {
		throw std::runtime_error("Not implemented yet.");

		//return ;
	}
	default: {
		throw std::runtime_error("Invalid architecture. Cannot parse.");
	}
	}

}

