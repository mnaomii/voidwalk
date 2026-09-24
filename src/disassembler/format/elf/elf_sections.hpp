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
// .shstrtab section, and store the entries they recognise: appended to `base` (the
// four common sections - lists, since ELF allows a name to repeat) or assigned into
// `extra` (the ELF-only ones, at most one each in practice). A section the file
// does not have leaves its `base` list empty, or its `extra` Header all-zero.
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
// a push_back into a `base` list or an assignment into an `extra` field - so a
// single name -> Store map covers all of them.
using Store = std::function<void(const Header&)>;

// 32-bit ELF (ELFCLASS32). Offsets are the Elf32_Shdr layout.
inline void parseSections32(Sections& base, ELF_Sections& extra, AddressSpace& data) {


	uint64_t e_shoff = data.read_u32(0x20); // section header offset
	uint64_t e_shentsize = data.read_u16(0x2e); // section header entry size
	uint64_t e_shnum = data.read_u16(0x30); // section header nb. entries
	uint64_t e_shstrndx = data.read_u16(0x32); // index into section names

	uint32_t sh_name, sh_offset, sh_size, sh_addr;

	std::unordered_map<std::string, Store> section_map = {
	{ ".text",    [&](const Header& h) { base._text.push_back(h);  } },
	{ ".data",    [&](const Header& h) { base._data.push_back(h);  } },
	{ ".rodata",  [&](const Header& h) { base._ronly.push_back(h); } },
	{ ".bss",     [&](const Header& h) { base._bss.push_back(h);   } },
	{ ".symtab",  [&](const Header& h) { extra._symtab = h;   } },
	{ ".dynsym",  [&](const Header& h) { extra._dynsym = h;   } },
	{ ".strtab",  [&](const Header& h) { extra._strtab = h;   } },
	{ ".dynstr",  [&](const Header& h) { extra._dynstr = h;   } },
	{ ".plt",     [&](const Header& h) { extra._plt = h;      } },
	{ ".got",     [&](const Header& h) { extra._got = h;      } },
	{ ".eh_frame",[&](const Header& h) { extra._eh_frame = h; } },
	};

	uint64_t shstrtab_entry = e_shoff + e_shstrndx * e_shentsize;
	uint64_t shstrtab_offset = data.read_u32(shstrtab_entry + 0x10);

	for (uint16_t count = 0; count < e_shnum; ++count) // going through the information of all sections
	{
		sh_name = data.read_u32(e_shoff + e_shentsize * count);             // index in glossary
		sh_offset = data.read_u32(e_shoff + e_shentsize * count + 0x10); 	// offset of section
		sh_size = data.read_u32(e_shoff + e_shentsize * count + 0x14);    // size of said section
		sh_addr = data.read_u32(e_shoff + e_shentsize * count + 0x0c);


		std::string section_name = "";

		for (uint64_t i = 0; ; ++i) {
			char c = data.read_u8(shstrtab_offset + sh_name + i);
			if (c == '\0') break;
			section_name += c;
		}


		auto it = section_map.find(section_name);
		if (it != section_map.end())
			it->second(Header(sh_addr, sh_offset, sh_size)); // Header(vaddr, offset, size)

	}


}

// 64-bit ELF (ELFCLASS64). Offsets are the Elf64_Shdr layout; otherwise identical
// to parseSections32.
inline void parseSections64(Sections& base, ELF_Sections& extra, AddressSpace& data) {

	uint64_t e_shoff =		data.read_u64(0x28); // section header offset
	uint64_t e_shentsize =	data.read_u16(0x3A); // size of one section header entry
	uint64_t e_shnum =		data.read_u16(0x3C); // how many entries
	uint64_t e_shstrndx =	data.read_u16(0x3E); // index of the section that holds section names

	if (e_shoff == 0 && e_shnum == 0) { // sstrip-ed binary

		// logic will go here

		return;
	}


	uint64_t sh_name; uint64_t sh_offset, sh_size, sh_addr;

	std::unordered_map<std::string, Store> section_map = {
	{ ".text",    [&](const Header& h) { base._text.push_back(h);  } },
	{ ".data",    [&](const Header& h) { base._data.push_back(h);  } },
	{ ".rodata",  [&](const Header& h) { base._ronly.push_back(h); } },
	{ ".bss",     [&](const Header& h) { base._bss.push_back(h);   } },
	{ ".symtab",  [&](const Header& h) { extra._symtab = h;   } },
	{ ".dynsym",  [&](const Header& h) { extra._dynsym = h;   } },
	{ ".strtab",  [&](const Header& h) { extra._strtab = h;   } },
	{ ".dynstr",  [&](const Header& h) { extra._dynstr = h;   } },
	{ ".plt",     [&](const Header& h) { extra._plt = h;      } },
	{ ".got",     [&](const Header& h) { extra._got = h;      } },
	{ ".eh_frame",[&](const Header& h) { extra._eh_frame = h; } },
	};


	uint64_t shstrtab_entry = e_shoff + e_shstrndx * e_shentsize;
	uint64_t shstrtab_offset = data.read_u64(shstrtab_entry + 0x18);


	// parsing the section map
	for (uint16_t count = 0; count < e_shnum; ++count) // going through the information of all sections
	{

		sh_name = data.read_u32(e_shoff + e_shentsize * count);             // index in glossary
		sh_offset = data.read_u64(e_shoff + e_shentsize * count + 0x18); 	// offset of section
		sh_size = data.read_u64(e_shoff + e_shentsize * count + 0x20);    // size of said section
		sh_addr = data.read_u64(e_shoff + e_shentsize * count + 0x10);

		std::string section_name = "";

		for (uint64_t i = 0; ; ++i) {
			char c = data.read_u8(shstrtab_offset + sh_name + i);
			if (c == '\0') break;
			section_name += c;
		}


		auto it = section_map.find(section_name);
		if (it != section_map.end())
			it->second(Header(sh_addr, sh_offset, sh_size)); // Header(vaddr, offset, size)

	}


}

} // namespace voidwalk::elf

