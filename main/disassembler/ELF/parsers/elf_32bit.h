#pragma once

inline void setHeaders32bit(header __symtab,
	header _dynsym,
	header _strtab,
	header _dynstr,
	header _plt,
	header _got,
	header _rel,
	header _eh_frame, AddressSpace& data) {


	uint32_t e_shoff = data.read_u8(0x20);
	//size_t shentsize = *reinterpret_cast<size_t*>(data[0x]);


}