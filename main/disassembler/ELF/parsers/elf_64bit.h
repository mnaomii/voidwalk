#ifndef ELF_64BIT_H
#define ELF_64BIT_H

#include "../../disassembler.h"
#include "../../../address_space/address_space.h"
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <cstdint>


void parseHeaders64(header& _text, header& _data, header& _ronly, header& _bss,
	header& _symtab,
	header& _dynsym,
	header& _strtab,
	header& _dynstr,
	header& _plt,
	header& _got,
	header& _rel,
	header& _eh_frame, AddressSpace& data, uint64_t e_shoff, uint16_t e_shentsize, uint16_t e_shnum, uint16_t e_shstrndx) {

	uint32_t sh_name; uint64_t sh_offset, sh_size, sh_addr;

	std::unordered_map<std::string, header*> section_map = {
	{ ".text",    &_text    },
	{ ".data",    &_data    },
	{ ".rodata",  &_ronly   },
	{ ".bss",     &_bss     },
	{ ".symtab",  &_symtab  },
	{ ".dynsym",  &_dynsym  },
	{ ".strtab",  &_strtab  },
	{ ".dynstr",  &_dynstr  },
	{ ".plt",     &_plt     },
	{ ".got",     &_got     },
	{ ".eh_frame",&_eh_frame},
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
			it->second->offset = sh_offset;
			it->second->size = sh_size;
		}

	}

}

void setHeaders64bit(header& _text, header& _data, header& _ronly, header& _bss,
	header& _symtab,
	header& _dynsym,
	header& _strtab,
	header& _dynstr,
	header& _plt,
	header& _got,
	header& _rel,
	header& _eh_frame, AddressSpace& data) {

	uint64_t e_shoff = data.read_u64(0x28); // section header offset
	uint16_t e_shentsize = data.read_u16(0x3A); // size of one section header entry
	uint16_t e_shnum =	data.read_u16(0x3C); // how many entries
	uint16_t e_shstrndx =	data.read_u16(0x3E); // index of the section that holds section names


	parseHeaders64(_text, _data, _ronly, _bss, _symtab, _dynsym, _strtab, _dynstr, _plt, _got, _rel, _eh_frame, data, e_shoff, e_shentsize, e_shnum, e_shstrndx);


}

 void setHeaders64bitARM(header& _text, header& _data, header& _ronly, header& _bss,
	header& _symtab,
	header& _dynsym,
	header& _strtab,
	header& _dynstr,
	header& _plt,
	header& _got,
	header& _rel,
	header& _eh_frame, AddressSpace& data) {
	throw std::runtime_error("AArch64 support not implemented yet.");
}

#endif