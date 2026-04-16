#include "elf-disassembler.h"
#include "parsers/elf-32bit.h"
#include "parsers/elf-64bit.h"
#include "parsers/elf-64bit-ARM.h"
#include <stdexcept>

void ELF_Disassembler::setHeadersOffsets() {
	if (is32bit) {
		if(filetype==1)
			setHeaders32bit( 
			 _symtab,
			 _dynsym,
			 _strtab,
			 _dynstr,
			 _plt,
			 _got,
			 _rel,
			 _eh_frame, contents); 

		else
			throw runtime_error("Invalid architecture [32bit ARM]");
		
	}
	else {
		if (filetype == 1)
			setHeaders64bit(
				_symtab,
				_dynsym,
				_strtab,
				_dynstr,
				_plt,
				_got,
				_rel,
				_eh_frame, contents);
		else
			setHeaders64bitARM(
				_symtab,
				_dynsym,
				_strtab,
				_dynstr,
				_plt,
				_got,
				_rel,
				_eh_frame, contents);
	}

}


ELF_Disassembler::ELF_Disassembler(AddressSpace& data): Disassembler(data) {
	if (this->contents.read_u8(0x04) == 0x01) is32bit = true;
	this->architecture = this->contents.read_u16(0x12);
	if(! (this->architecture == 0x03 && this->architecture==0x3E && this->architecture==0xB7) ) throw exception("Architecture is not supported yet.");
	

	this->setHeadersOffsets();
}