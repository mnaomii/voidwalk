#pragma once

inline void setHeaders64bit(header __symtab,
	header _dynsym,
	header _strtab,
	header _dynstr,
	header _plt,
	header _got,
	header _rel,
	header _eh_frame, AddressSpace& data) {

	uint64_t e_shoff = data.read_u8(0x28); // section header offset
	uint16_t shentsize = data.read_u8(0x3A);
	uint16_t shnum =	data.read_u8(0x3C);
	uint16_t shstrndx =	data.read_u8(0x3E);

}

#pragma once

inline void setHeaders64bitARM(header __symtab,
	header _dynsym,
	header _strtab,
	header _dynstr,
	header _plt,
	header _got,
	header _rel,
	header _eh_frame, AddressSpace& data) {

}