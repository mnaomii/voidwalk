#include "pe-disassembler.h"
#include "parsers/pe_32bit.h"
#include "parsers/pe_64bit.h"

void PE_Disassembler::setHeadersOffsets() {
	switch (this->architecture) {
	case 0x14c: // 32bit
		setHeaders32bit(this->_text, this->_data, this->_ronly, this->_bss,
			this->_idata, this->_edata, this->_rsrc, this->_pdata, this->contents, this->e_lfanew); break;

	case 0x8664: // 64bit
		setHeaders64bit(this->_text, this->_data, this->_ronly, this->_bss,
			this->_idata, this->_edata, this->_rsrc, this->_pdata, this->contents, this->e_lfanew); break;

	case 0xAA64: // AARCH64
		setHeaders64bitARM(this->_text, this->_data, this->_ronly, this->_bss,
			this->_idata, this->_edata, this->_rsrc, this->_pdata, this->contents, this->e_lfanew); break;

	case 0x1c0: // ARM32
		setHeaders32bitARM(this->_text, this->_data, this->_ronly, this->_bss,
			this->_idata, this->_edata, this->_rsrc, this->_pdata, this->contents, this->e_lfanew); break;
	}
}

std::string PE_Disassembler::getArchitecture() {
	switch (this->architecture) {
	case 0x14c:
		return "x86";

	case 0x8664:
		return "x86_64";

	case 0xAA64:
		return "AArch64";

	case 0x1c0:
		return "ARM32";

	}
}

PE_Disassembler::PE_Disassembler(AddressSpace& data) : Disassembler(data) {
	
	this->e_lfanew = data.read_u32(0x3C);

	this->architecture = data.read_u32(e_lfanew + 4);


	this->initHeader(this->_idata);
	this->initHeader(this->_edata);
	this->initHeader(this->_rsrc);
	this->initHeader(this->_pdata);
	this->setHeadersOffsets();
}

