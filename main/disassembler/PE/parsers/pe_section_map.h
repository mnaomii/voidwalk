#ifndef PE_PARSER_H
#define PE_PARSER_H

#include <unordered_map>
#include <string>
#include <stdexcept>
#include "../../disassembler.h"

enum SectionKind { CODE, DATA, RODATA, BSS, NONE };

int classify(uint32_t c) {
	if (c & 0x20000000 || c & 0x20) return CODE;          // EXECUTE or CNT_CODE
	if (c & 0x80)                    return BSS;           // CNT_UNINIT_DATA
	if (c & 0x40) return (c & 0x80000000) ? DATA : RODATA; // INIT_DATA, WRITE?
	return NONE;
}

void setHeaders32bit(Sections& base, PE_Sections& extra, AddressSpace& data, uint32_t e_lfanew) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader;
	uint64_t ImageBase = data.read_u32(e_lfanew + 52);



	std::unordered_map<int, Header*> section_map = {   // key is int, NOT the name
		{ CODE,   &base._text  },
		{ DATA,   &base._data  },
		{ RODATA, &base._ronly },
		{ BSS,    &base._bss   },
	};

	for (int i = 0; i < NumberOfSections; ++i) {
		uint64_t b = SectionTable + i * 40;
		uint32_t chars = data.read_u32(b + 36);

		auto it = section_map.find(classify(chars));   // <-- lookup by category
		if (it != section_map.end()) {
			it->second->setVaddr(ImageBase + data.read_u32(b + 12));
			it->second->setSize(data.read_u32(b + 16));
			it->second->setOffset(data.read_u32(b + 20));
		}
	}

}


void setHeaders64bit(Sections& base, PE_Sections extra, AddressSpace& data, uint32_t e_lfanew) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 6);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 20);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader; // size of raw data, not virtual
	uint64_t ImageBase = data.read_u8(e_lfanew + 48);


	std::unordered_map<int, Header*> section_map = {   
		{ CODE,   &base._text  },
		{ DATA,   &base._data  },
		{ RODATA, &base._ronly },
		{ BSS,    &base._bss   },
	};

	for (int i = 0; i < NumberOfSections; ++i) {
		uint64_t b = SectionTable + i * 40;
		uint32_t chars = data.read_u32(b + 36);

		auto it = section_map.find(classify(chars));   
		if (it != section_map.end()) {
			it->second->setVaddr(ImageBase + data.read_u32(b + 12));
			it->second->setSize(data.read_u32(b + 16));
			it->second->setOffset(data.read_u32(b + 20));
		}
	}
}



#endif
