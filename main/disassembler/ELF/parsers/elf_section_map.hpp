#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include "../../disassembler.hpp"
#include "../../../address-space/address_space.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>



inline void setHeaders32bit(Sections& base, ELF_Sections& extra, AddressSpace& data) {


	uint64_t e_shoff = data.read_u32(0x20); // section header offset
	uint64_t e_shentsize = data.read_u16(0x2e); // section header entry size
	uint64_t e_shnum = data.read_u16(0x30); // section header nb. entries
	uint64_t e_shstrndx = data.read_u16(0x32); // index into section names

	uint32_t sh_name, sh_offset, sh_size, sh_addr;

	std::unordered_map<std::string, Header*> section_map = {
	{ ".text",    &base._text    },
	{ ".data",    &base._data    },
	{ ".rodata",  &base._ronly   },
	{ ".bss",     &base._bss     },
	{ ".symtab",  &extra._symtab  },
	{ ".dynsym",  &extra._dynsym  },
	{ ".strtab",  &extra._strtab  },
	{ ".dynstr",  &extra._dynstr  },
	{ ".plt",     &extra._plt     },
	{ ".got",     &extra._got     },
	{ ".eh_frame",&extra._eh_frame},
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
		if (it != section_map.end()) {
			it->second->setOffset(sh_offset);
			it->second->setSize(sh_size);
			it->second->setVaddr(sh_addr);
		}

	}


}

inline void setHeaders64bit(Sections& base, ELF_Sections& extra, AddressSpace& data) {

	uint64_t e_shoff =		data.read_u64(0x28); // section header offset
	uint64_t e_shentsize =	data.read_u16(0x3A); // size of one section header entry
	uint64_t e_shnum =		data.read_u16(0x3C); // how many entries
	uint64_t e_shstrndx =	data.read_u16(0x3E); // index of the section that holds section names

	if (e_shoff == 0 && e_shnum == 0) { // sstrip-ed binary



		return;
	}


	uint64_t sh_name; uint64_t sh_offset, sh_size, sh_addr;

	std::unordered_map<std::string, Header*> section_map = {
	{ ".text",    &base._text    },
	{ ".data",    &base._data    },
	{ ".rodata",  &base._ronly   },
	{ ".bss",     &base._bss     },
	{ ".symtab",  &extra._symtab  },
	{ ".dynsym",  &extra._dynsym  },
	{ ".strtab",  &extra._strtab  },
	{ ".dynstr",  &extra._dynstr  },
	{ ".plt",     &extra._plt     },
	{ ".got",     &extra._got     },
	{ ".eh_frame",&extra._eh_frame},
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
		if (it != section_map.end()) {
			it->second->setOffset(sh_offset);
			it->second->setSize(sh_size);
			it->second->setVaddr(sh_addr);
		}

	}


}

#endif