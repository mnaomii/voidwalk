#ifndef PE_32BIT_H
#define PE_32BIT_H

#include <unordered_map>
#include <string>
#include "../../disassembler.h"


void setHeaders32bit(header& _text, header& _data, header& _ronly, header& _bss,
	header& _idata, header& _edata, header& _rsrc, header& _pdata, AddressSpace& data, uint32_t e_lfanew) {

	uint16_t NumberOfSections = data.read_u16(e_lfanew + 2);
	uint16_t SizeOfOptionalHeader = data.read_u16(e_lfanew + 16);
	uint64_t SectionTable = e_lfanew + 24 + SizeOfOptionalHeader;

	std::string name;

		std::unordered_map<std::string, header*> section_map = {
	{ ".text",    &_text    },
	{ ".data",    &_data    },
	{ ".rodata",  &_ronly   },
	{ ".bss",     &_bss     },
	{ ".idata",  &_idata  },
	{ ".edata",  &_edata  },
	{ ".rsrc",  &_rsrc  },
	{ ".pdata",  &_pdata}
		};

		for (int i = 0; i < NumberOfSections; ++i) {

			

			auto it = section_map.find(section_name);
			if (it != section_map.end()) {
				it->second->offset = sh_offset;
				it->second->size = sh_size;
			}
	}


}

void setHeaders32bitARM(header& _text, header& _data, header& _ronly, header& _bss,
	header& _idata, header& _edata, header& _rsrc, header& _pdata, AddressSpace& data, uint32_t e_lfanew) {

}

void setHeaders64bit(header& _text, header& _data, header& _ronly, header& _bss,
	header& _idata, header& _edata, header& _rsrc, header& _pdata, AddressSpace& data, uint32_t e_lfanew) {

}

void setHeaders64bitARM(header& _text, header& _data, header& _ronly, header& _bss,
	header& _idata, header& _edata, header& _rsrc, header& _pdata, AddressSpace& data, uint32_t e_lfanew) {

}


#endif