#include "disassembler/format/elf/elf_disassembler.hpp"
#include "disassembler/format/elf/elf_sections.hpp"
#include <stdexcept>

namespace voidwalk {

namespace {

// Maps a ELF e_machine value to the architecture it denotes, or Arch::Unknown.
Arch archOfElfMachine(uint16_t e_machine) {
	switch (e_machine) {
		case 0x03: return Arch::X86;
		case 0x28: return Arch::ARM32;
		case 0x3E: return Arch::X86_64;
		case 0xB7: return Arch::AArch64;
		default:   return Arch::Unknown;
	}
}

} // namespace

ELF_Disassembler::ELF_Disassembler(AddressSpace& data, const std::vector<std::ostream*>& outputs) : Disassembler(data, outputs) { // constructor
	this->setArch(archOfElfMachine(this->contents.read_u16(0x12)));


	this->setHeadersOffsets();
}

void ELF_Disassembler::setHeadersOffsets() {
	if (this->arch == Arch::Unknown)
		throw std::runtime_error("Unsupported 64-bit ELF architecture.");

	if (voidwalk::is64Bit(this->arch))
		elf::parseSections64(this->baseSections, this->extraSections, this->contents);
	else
		elf::parseSections32(this->baseSections, this->extraSections, this->contents);
}

} // namespace voidwalk
