#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include "../../disassembler.h"
#include "../../../address-space/address_space.h"
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <cstdint>



void setHeaders32bit(Sections& base, ELF_Sections& extra, AddressSpace& data) {


	uint32_t e_shoff = data.read_u32(0x20);
	uint16_t e_shentsize = data.read_u16(0x2e);
	uint16_t e_shnum = data.read_u16(0x30);
	uint16_t e_shstrndx = data.read_u16(0x32);

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


	uint32_t shstrtab_entry = e_shoff + e_shstrndx * e_shentsize;
	uint32_t shstrtab_offset = data.read_u32(shstrtab_entry + 0x10);

	for (uint16_t count = 0; count < e_shnum; ++count) // going through the information of all sections
	{
		sh_name = data.read_u32(e_shoff + e_shentsize * count);             // index in glossary
		sh_offset = data.read_u32(e_shoff + e_shentsize * count + 0x10); 	// offset of section
		sh_size = data.read_u32(e_shoff + e_shentsize * count + 0x14);    // size of said section
		sh_addr = data.read_u32(e_shoff + e_shentsize * count + 0x0c);


		std::string section_name = "";

		for (uint32_t i = 0; ; ++i) {
			char c = data.read_u8(shstrtab_offset + sh_name + i);
			if (c == '\0') break;
			section_name += c;
		}


		auto it = section_map.find(section_name);
		if (it != section_map.end()) {
			it->second->setOffset(sh_offset);
			it->second->setSize(sh_size);
		}

	}


}

void setHeaders64bit(Sections& base, ELF_Sections& extra, AddressSpace& data) {

	uint64_t e_shoff = data.read_u64(0x28); // section header offset
	uint16_t e_shentsize = data.read_u16(0x3A); // size of one section header entry
	uint16_t e_shnum =	data.read_u16(0x3C); // how many entries
	uint16_t e_shstrndx =	data.read_u16(0x3E); // index of the section that holds section names


	uint32_t sh_name; uint64_t sh_offset, sh_size, sh_addr;

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

	for (uint16_t count = 0; count < e_shnum; ++count) // going through the information of all sections
	{

		sh_name = data.read_u32(e_shoff + e_shentsize * count);             // index in glossary
		sh_offset = data.read_u64(e_shoff + e_shentsize * count + 0x18); 	// offset of section
		sh_size = data.read_u64(e_shoff + e_shentsize * count + 0x20);    // size of said section
		sh_addr = data.read_u64(e_shoff + e_shentsize * count + 0x10);

		std::string section_name = "";

		for (uint32_t i = 0; ; ++i) {
			char c = data.read_u8(shstrtab_offset + sh_name + i);
			if (c == '\0') break;
			section_name += c;
		}


		auto it = section_map.find(section_name);
		if (it != section_map.end()) {
			it->second->setOffset(sh_offset);
			it->second->setSize(sh_size);
		}

	}


}

#endif