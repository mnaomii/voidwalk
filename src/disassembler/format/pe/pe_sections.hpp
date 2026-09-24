#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>
#include "disassembler/disassembler.hpp"

// PE section-table readers. One per PE class (PE32 / PE32+); PE_Disassembler picks
// between them on is64Bit(arch).
//
// Unlike ELF, sections are matched by their Characteristics flags, not by name -
// PE section names are conventions (.text/.code/CODE) that a linker is free to
// ignore, while the flags are what the loader itself acts on. Consequence: every
// section of a kind is appended to that kind's list in `base`, in table order - a
// binary with several code sections contributes several entries to `_text`.
//
// Both also publish the optional header's ImageBase through `imageBase`, which is
// what turns a section's RVA into the virtual address stored in its Header.
//
// Reads are bounds-checked by AddressSpace (std::length_error past end-of-file);
// the field values are not validated.
namespace voidwalk::pe {

// Which of the four common sections a Characteristics word describes.
enum SectionKind { CODE, DATA, RODATA, BSS, NONE };

// Maps a section's Characteristics field to a SectionKind. Order matters:
// executable wins over everything, then uninitialised data, then initialised data
// split on the WRITE bit. NONE for anything else (debug info, relocations, ...).
inline int classify(uint32_t c) {
	if (c & 0x20000000 || c & 0x20) return CODE;          // EXECUTE or CNT_CODE
	if (c & 0x80)                    return BSS;           // CNT_UNINIT_DATA
	if (c & 0x40) return (c & 0x80000000) ? DATA : RODATA; // INIT_DATA, WRITE?
	return NONE;
}

// PE32. `e_lfanew` is the file offset of the PE signature (from the DOS header at
// 0x3C). Writes the optional header's 32-bit ImageBase to `imageBase`.
inline void parseSections32(Sections& base, PE_Sections& extra, AddressSpace& data, uint32_t e_lfanew, uint64_t& imageBase) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader;
	imageBase = data.read_u32(e_lfanew + 52);



	std::unordered_map<int, std::vector<Header>*> section_map = {   // key is int, NOT the name
		{ CODE,   &base._text  },
		{ DATA,   &base._data  },
		{ RODATA, &base._ronly },
		{ BSS,    &base._bss   },
	};

	for (int i = 0; i < NumberOfSections; ++i) {
		uint64_t b = SectionTable + i * 40;
		uint32_t chars = data.read_u32(b + 36);

		auto it = section_map.find(classify(chars));   // <-- lookup by category
		if (it != section_map.end())
			it->second->push_back(Header(imageBase + data.read_u32(b + 12),   // vaddr
			                             data.read_u32(b + 20),               // offset
			                             data.read_u32(b + 16)));             // size
	}

}


// PE32+. Same as parseSections32 but reads the 64-bit ImageBase, which sits at a
// different optional-header offset.
inline void parseSections64(Sections& base, PE_Sections& extra, AddressSpace& data, uint32_t e_lfanew, uint64_t& imageBase) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader; // size of raw data, not virtual
	imageBase = data.read_u64(e_lfanew + 48);


	std::unordered_map<int, std::vector<Header>*> section_map = {
		{ CODE,   &base._text  },
		{ DATA,   &base._data  },
		{ RODATA, &base._ronly },
		{ BSS,    &base._bss   },
	};

	for (int i = 0; i < NumberOfSections; ++i) {
		uint64_t b = SectionTable + i * 40;
		uint32_t chars = data.read_u32(b + 36);

		auto it = section_map.find(classify(chars));
		if (it != section_map.end())
			it->second->push_back(Header(imageBase + data.read_u32(b + 12),   // vaddr
			                             data.read_u32(b + 20),               // offset
			                             data.read_u32(b + 16)));             // size
	}
}

} // namespace voidwalk::pe




