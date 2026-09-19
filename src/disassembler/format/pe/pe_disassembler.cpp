#include "disassembler/format/pe/pe_disassembler.hpp"
#include "address_space.hpp"
#include "disassembler/format/pe/pe_sections.hpp"
#include <stdexcept>

namespace voidwalk {

namespace {

// Maps a PE COFF Machine value to the architecture it denotes, or Arch::Unknown.
Arch archOfPeMachine(uint16_t machine) {
	switch (machine) {
		case 0x14c:  return Arch::X86;
		case 0x8664: return Arch::X86_64;
		case 0xAA64: return Arch::AArch64;
		case 0x1c0:  return Arch::ARM32;
		default:     return Arch::Unknown;
	}
}

} // namespace

PE_Disassembler::PE_Disassembler(AddressSpace& data, const std::vector<std::ostream*>& outputs) : Disassembler(data, outputs) {

	this->e_lfanew = contents.read_u32(0x3C);

	// The COFF Machine field is 16 bits; read as u32 and narrowed, as before.
	this->setArch(archOfPeMachine(static_cast<uint16_t>(contents.read_u32(e_lfanew + 4))));

	this->setHeadersOffsets();
}

void PE_Disassembler::setHeadersOffsets() {
	if (this->arch == Arch::Unknown)
		throw std::runtime_error("Architecture not recognized.\n");

	if (voidwalk::is64Bit(this->arch))
		pe::parseSections64(this->baseSections, this->extraSections, this->contents, this->e_lfanew, imageBase);
	else
		pe::parseSections32(this->baseSections, this->extraSections, this->contents, this->e_lfanew, imageBase);
}

} // namespace voidwalk
