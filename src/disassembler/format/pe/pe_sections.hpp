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
// section of a kind is appended to that kind's list in `common`, in table order - a
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
inline int classify(uint32_t characteristics) {
	if (characteristics & 0x20000000 || characteristics & 0x20) return CODE;          // EXECUTE or CNT_CODE
	if (characteristics & 0x80)                    return BSS;           // CNT_UNINIT_DATA
	if (characteristics & 0x40) return (characteristics & 0x80000000) ? DATA : RODATA; // INIT_DATA, WRITE?
	return NONE;
}

// PE32. `peHeaderOffset` is the file offset of the PE signature (from the DOS header at
// 0x3C). Writes the optional header's 32-bit ImageBase to `imageBase`.
inline void parseSections32(Sections& common, [[maybe_unused]] PE_Sections& extra, AddressSpace& data, uint32_t peHeaderOffset, uint64_t& imageBase) {

	uint16_t sectionCount = data.read_u16(peHeaderOffset + 6);          // NumberOfSections
	uint16_t optionalHeaderSize = data.read_u16(peHeaderOffset + 20);   // SizeOfOptionalHeader
	uint64_t sectionTableOffset = peHeaderOffset + 24 + optionalHeaderSize;
	imageBase = data.read_u32(peHeaderOffset + 52);



	std::unordered_map<int, std::vector<Header>*> sectionLists = {   // key is int, NOT the name
		{ CODE,   &common.text  },
		{ DATA,   &common.data  },
		{ RODATA, &common.readOnly },
		{ BSS,    &common.bss   },
	};

	for (int i = 0; i < sectionCount; ++i) {
		uint64_t entryOffset = sectionTableOffset + i * 40;
		uint32_t characteristics = data.read_u32(entryOffset + 36);

		auto it = sectionLists.find(classify(characteristics));   // <-- lookup by category
		if (it != sectionLists.end())
			it->second->push_back(Header(imageBase + data.read_u32(entryOffset + 12),   // vaddr
			                             data.read_u32(entryOffset + 20),               // offset
			                             data.read_u32(entryOffset + 16)));             // size
	}

}


// PE32+. Same as parseSections32 but reads the 64-bit ImageBase, which sits at a
// different optional-header offset.
inline void parseSections64(Sections& common, [[maybe_unused]] PE_Sections& extra, AddressSpace& data, uint32_t peHeaderOffset, uint64_t& imageBase) {

	uint16_t sectionCount = data.read_u16(peHeaderOffset + 6);          // NumberOfSections
	uint16_t optionalHeaderSize = data.read_u16(peHeaderOffset + 20);   // SizeOfOptionalHeader
	uint64_t sectionTableOffset = peHeaderOffset + 24 + optionalHeaderSize; // size of raw data, not virtual
	imageBase = data.read_u64(peHeaderOffset + 48);


	std::unordered_map<int, std::vector<Header>*> sectionLists = {
		{ CODE,   &common.text  },
		{ DATA,   &common.data  },
		{ RODATA, &common.readOnly },
		{ BSS,    &common.bss   },
	};

	for (int i = 0; i < sectionCount; ++i) {
		uint64_t entryOffset = sectionTableOffset + i * 40;
		uint32_t characteristics = data.read_u32(entryOffset + 36);

		auto it = sectionLists.find(classify(characteristics));
		if (it != sectionLists.end())
			it->second->push_back(Header(imageBase + data.read_u32(entryOffset + 12),   // vaddr
			                             data.read_u32(entryOffset + 20),               // offset
			                             data.read_u32(entryOffset + 16)));             // size
	}
}

} // namespace voidwalk::pe




