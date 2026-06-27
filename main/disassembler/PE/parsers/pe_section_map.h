#ifndef PE_PARSER_H
#define PE_PARSER_H

#include <unordered_map>
#include <string>
#include <stdexcept>
#include "../../disassembler.h"

void setHeaders32bit(Sections& base, PE_Sections extra, AddressSpace& data, uint32_t e_lfanew) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader;

	std::unordered_map<std::string, Header*> section_map = {
	{ ".text",    &base._text    },
	{ ".data",    &base._data    },
	{ ".rodata",  &base._ronly   },
	{ ".bss",     &base._bss     },
	{ ".idata",  &extra._idata  },
	{ ".edata",  &extra._edata  },
	{ ".rsrc",  &extra._rsrc  },
	{ ".pdata",  &extra._pdata}
	};

	for (int i = 0; i < NumberOfSections; ++i) {
		std::string section_name = "";

		for (size_t index = 0; ; ++index) {
			char c = data.read_u8(SectionTable + index + i * 40);
			if (c == '\0') break;
			section_name += c;
		}

		auto it = section_map.find(section_name);
		if (it != section_map.end()) {
			it->second->setVaddr(data.read_u32(SectionTable + i * 40 + 12));
			it->second->setOffset(data.read_u32(SectionTable + i * 40 + 20));
			it->second->setSize( data.read_u32(SectionTable + i * 40 + 16));
		}
	}

}


void setHeaders64bit(Sections& base, PE_Sections extra, AddressSpace& data, uint32_t e_lfanew) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader;

	std::unordered_map<std::string, Header*> section_map = {
	{ ".text",    &base._text    },
	{ ".data",    &base._data    },
	{ ".rodata",  &base._ronly   },
	{ ".bss",     &base._bss     },
	{ ".idata",  &extra._idata  },
	{ ".edata",  &extra._edata  },
	{ ".rsrc",  &extra._rsrc  },
	{ ".pdata",  &extra._pdata}
	};


	for (int i = 0; i < NumberOfSections; ++i) {
		std::string section_name = "";

		for (size_t j = 0; j < 8; ++j) {
			char c = data.read_u8(SectionTable + i*40 + j);
			if (c == '\0') break;
			section_name += c;
		}

		auto it = section_map.find(section_name);
		if (it != section_map.end()) {
			it->second->setVaddr(data.read_u32(SectionTable + i * 40 + +12));
			it->second->setSize(data.read_u32(SectionTable+ i*40 + 16));
			it->second->setOffset(data.read_u32(SectionTable + i * 40 + 20));
		}
	}
}



#endif
