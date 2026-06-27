#include "../../../address_space/address_space.h"
#include "../elf_disassembler.h"
#include <string>
#include <cstdint>

// FORMAT:

// prefix : 0-3 bytes
// opcode : 1-3 bytes
// Mod R/M : 0-1 bytes : Mod + Reg/Opcode + R/M (little endian)
// SIB : 0-1 bytes : Scale + Index + Base (little endian)
// Displacement : 0-4 bytes
// Immediate : 0-4 bytes

std::string decodeLine(ELF_Disassembler base, uint64_t offset) {
	
}