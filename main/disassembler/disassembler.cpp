#include "disassembler.h"

void Disassembler::initHeader(header& x) {
	x.offset = 0;
	x.size = 0;
	x.vaddr = 0;
}

Disassembler::Disassembler(AddressSpace& temp) : //filetype(0),
    contents(temp), is32bit(false), architecture(0) {
	this->initHeader(this->_ronly);
	this->initHeader(this->_text);
	this->initHeader(this->_data);
	this->initHeader(this->_bss);

}