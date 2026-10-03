#pragma once
#include "disassembler/disassembler.hpp"
#include "address_space.hpp"
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>



// ELF section-header-table readers. One per ELF class; ELF_Disassembler picks
// between them on is64Bit(arch).
//
// Both walk the e_shnum entries at e_shoff, resolve each sh_name through the
// .shstrtab section, and store the entries they recognise: appended to `common` (the
// four common sections - lists, since ELF allows a name to repeat) or assigned into
// `extra` (the ELF-only ones, at most one each in practice). A section the file
// does not have leaves its `common` list empty, or its `extra` Header all-zero.
//
// Section NAMES are the key, so a stripped or non-standard binary simply yields
// fewer sections rather than an error.
//
// Every field read goes through AddressSpace's bounds check, so a header pointing
// outside the file throws std::length_error rather than reading wild memory. The
// values themselves are NOT validated: sh_size is copied verbatim and can exceed
// the file (see Header::getSize).
namespace voidwalk::elf {

// What to do with a recognised section's Header. One type for both destinations -
// a push_back into a `common` list or an assignment into an `extra` field - so a
// single name -> Store map covers all of them.
using Store = std::function<void(const Header&)>;

// 32-bit ELF (ELFCLASS32). Offsets are the Elf32_Shdr layout.
inline void parseSections32(Sections& common, ELF_Sections& extra, AddressSpace& data) {


	uint64_t sectionTableOffset = data.read_u32(0x20); // e_shoff: section header offset
	uint64_t sectionEntrySize = data.read_u16(0x2e);   // e_shentsize: section header entry size
	uint64_t sectionCount = data.read_u16(0x30);       // e_shnum: section header nb. entries
	uint64_t namesSectionIndex = data.read_u16(0x32);  // e_shstrndx: index into section names

	uint32_t nameOffset, sectionOffset, sectionSize, sectionVaddr;

	std::unordered_map<std::string, Store> sectionHandlers = {
	{ ".text",    [&](const Header& header) { common.text.push_back(header);  } },
	{ ".data",    [&](const Header& header) { common.data.push_back(header);  } },
	{ ".rodata",  [&](const Header& header) { common.readOnly.push_back(header); } },
	{ ".bss",     [&](const Header& header) { common.bss.push_back(header);   } },
	{ ".symtab",  [&](const Header& header) { extra.symtab = header;   } },
	{ ".dynsym",  [&](const Header& header) { extra.dynsym = header;   } },
	{ ".strtab",  [&](const Header& header) { extra.strtab = header;   } },
	{ ".dynstr",  [&](const Header& header) { extra.dynstr = header;   } },
	{ ".plt",     [&](const Header& header) { extra.plt = header;      } },
	{ ".got",     [&](const Header& header) { extra.got = header;      } },
	{ ".eh_frame",[&](const Header& header) { extra.ehFrame = header; } },
	};

	uint64_t namesSectionEntry = sectionTableOffset + namesSectionIndex * sectionEntrySize;
	uint64_t namesOffset = data.read_u32(namesSectionEntry + 0x10);

	for (uint16_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) // going through the information of all sections
	{
		nameOffset = data.read_u32(sectionTableOffset + sectionEntrySize * sectionIndex);            // sh_name: index in glossary
		sectionOffset = data.read_u32(sectionTableOffset + sectionEntrySize * sectionIndex + 0x10);  // sh_offset: offset of section
		sectionSize = data.read_u32(sectionTableOffset + sectionEntrySize * sectionIndex + 0x14);    // sh_size: size of said section
		sectionVaddr = data.read_u32(sectionTableOffset + sectionEntrySize * sectionIndex + 0x0c);   // sh_addr


		std::string sectionName = "";

		for (uint64_t i = 0; ; ++i) {
			char nameChar = data.read_u8(namesOffset + nameOffset + i);
			if (nameChar == '\0') break;
			sectionName += nameChar;
		}


		auto it = sectionHandlers.find(sectionName);
		if (it != sectionHandlers.end())
			it->second(Header(sectionVaddr, sectionOffset, sectionSize)); // Header(vaddr, offset, size)

	}


}

// 64-bit ELF (ELFCLASS64). Offsets are the Elf64_Shdr layout; otherwise identical
// to parseSections32.
inline void parseSections64(Sections& common, ELF_Sections& extra, AddressSpace& data) {

	uint64_t sectionTableOffset = data.read_u64(0x28); // e_shoff: section header offset
	uint64_t sectionEntrySize = data.read_u16(0x3A);   // e_shentsize: size of one section header entry
	uint64_t sectionCount = data.read_u16(0x3C);       // e_shnum: how many entries
	uint64_t namesSectionIndex = data.read_u16(0x3E);  // e_shstrndx: index of the section that holds section names

	if (sectionTableOffset == 0 && sectionCount == 0) { // sstrip-ed binary

		// logic will go here

		return;
	}


	uint64_t nameOffset; uint64_t sectionOffset, sectionSize, sectionVaddr;

	std::unordered_map<std::string, Store> sectionHandlers = {
	{ ".text",    [&](const Header& header) { common.text.push_back(header);  } },
	{ ".data",    [&](const Header& header) { common.data.push_back(header);  } },
	{ ".rodata",  [&](const Header& header) { common.readOnly.push_back(header); } },
	{ ".bss",     [&](const Header& header) { common.bss.push_back(header);   } },
	{ ".symtab",  [&](const Header& header) { extra.symtab = header;   } },
	{ ".dynsym",  [&](const Header& header) { extra.dynsym = header;   } },
	{ ".strtab",  [&](const Header& header) { extra.strtab = header;   } },
	{ ".dynstr",  [&](const Header& header) { extra.dynstr = header;   } },
	{ ".plt",     [&](const Header& header) { extra.plt = header;      } },
	{ ".got",     [&](const Header& header) { extra.got = header;      } },
	{ ".eh_frame",[&](const Header& header) { extra.ehFrame = header; } },
	};


	uint64_t namesSectionEntry = sectionTableOffset + namesSectionIndex * sectionEntrySize;
	uint64_t namesOffset = data.read_u64(namesSectionEntry + 0x18);


	// parsing the section map
	for (uint16_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) // going through the information of all sections
	{

		nameOffset = data.read_u32(sectionTableOffset + sectionEntrySize * sectionIndex);            // sh_name: index in glossary
		sectionOffset = data.read_u64(sectionTableOffset + sectionEntrySize * sectionIndex + 0x18);  // sh_offset: offset of section
		sectionSize = data.read_u64(sectionTableOffset + sectionEntrySize * sectionIndex + 0x20);    // sh_size: size of said section
		sectionVaddr = data.read_u64(sectionTableOffset + sectionEntrySize * sectionIndex + 0x10);   // sh_addr

		std::string sectionName = "";

		for (uint64_t i = 0; ; ++i) {
			char nameChar = data.read_u8(namesOffset + nameOffset + i);
			if (nameChar == '\0') break;
			sectionName += nameChar;
		}


		auto it = sectionHandlers.find(sectionName);
		if (it != sectionHandlers.end())
			it->second(Header(sectionVaddr, sectionOffset, sectionSize)); // Header(vaddr, offset, size)

	}


}

} // namespace voidwalk::elf

