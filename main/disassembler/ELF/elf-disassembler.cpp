#include "elf-disassembler.h"
#include "parsers/elf_32bit.h"
#include "parsers/elf_64bit.h"
#include <stdexcept>


ELF_Disassembler::ELF_Disassembler(AddressSpace& data) : Disassembler(data) { // constructor
	if (this->contents.read_u8(0x04) == 0x01) is32bit = true;
	else is32bit = false;
	this->architecture = this->contents.read_u16(0x12);
	//						  x86						  x86_64
	// this->architecture == 0x03 ; this->architecture == 0x3E ;
	//						 ARM64						   ARM32
	//if(this->architecture == 0xB7 || this->architecture == 0x28) throw exception("Architecture is not supported yet.");

	this->initHeader(this->_symtab);
	this->initHeader(this->_dynsym);
	this->initHeader(this->_strtab);
	this->initHeader(this->_dynstr);
	this->initHeader(this->_plt);
	this->initHeader(this->_got);
	this->initHeader(this->_rel);
	this->initHeader(this->_eh_frame);

	this->setHeadersOffsets();
}

std::string ELF_Disassembler::getArchitecture() {
	switch (this->architecture) {
	case 0x03:
		return "x86";
	case 0x28:
		return "ARM32";
	case 0x3E:
		return "x86_64";
	case 0xB7:
		return "AArch64";
	default:
		return "Unknown";
	}
}

void ELF_Disassembler::setHeadersOffsets() {
	if (is32bit) {
		if (architecture == 0x03) {
			setHeaders32bit(this->_text, this->_data, this->_ronly, this->_bss,
				this->_symtab,
				this->_dynsym,
				this->_strtab,
				this->_dynstr,
				this->_plt,
				this->_got,
				this->_rel,
				this->_eh_frame, this->contents);
		}
		else if (architecture == 0x28) setHeaders32bitARM(this->_text, this->_data, this->_ronly, this->_bss,
			this->_symtab,
			this->_dynsym,
			this->_strtab,
			this->_dynstr,
			this->_plt,
			this->_got,
			this->_rel,
			this->_eh_frame, this->contents);
		else throw std::runtime_error("Unsupported 32-bit ELF architecture.");
	}
	else {
		if (architecture == 0x3E) setHeaders64bit(this->_text, this->_data, this->_ronly, this->_bss,
			this->_symtab,
			this->_dynsym,
			this->_strtab,
			this->_dynstr,
			this->_plt,
			this->_got,
			this->_rel,
			this->_eh_frame, this->contents);
		else if (architecture == 0xB7) setHeaders64bitARM(this->_text, this->_data, this->_ronly, this->_bss,
			this->_symtab,
			this->_dynsym,
			this->_strtab,
			this->_dynstr,
			this->_plt,
			this->_got,
			this->_rel,
			this->_eh_frame, this->contents);
		else throw std::runtime_error("Unsupported 64-bit ELF architecture.");
	}
}

