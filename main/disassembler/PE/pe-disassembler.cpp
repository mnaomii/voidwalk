#include "pe-disassembler.h"

void PE_Disassembler::getIDATAHeader() {

}
void PE_Disassembler::getEDATAHeader() {

}
void PE_Disassembler::getRSRCHeader() {

}
void PE_Disassembler::getPDATAHeader() {

}

void PE_Disassembler::getDataHeader() {

}
void PE_Disassembler::getTextHeader() {

}
void PE_Disassembler::getROnlyHeader() {

}
void PE_Disassembler::getBSSHeader() {

}

PE_Disassembler::PE_Disassembler(vector<uint8_t> data) : Disassembler(data) {
	this->getIDATAHeader();
	this->getEDATAHeader();
	this->getRSRCHeader();
	this->getPDATAHeader();
}

