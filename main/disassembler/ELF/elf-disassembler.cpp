#include "elf-disassembler.h"


void ELF_Disassembler::getSymtabHeader() {

}
void ELF_Disassembler::getDynsymHeader() {

}
void ELF_Disassembler::getStrtabHeader() {

}
void ELF_Disassembler::getDynstrHeader() {

}
void ELF_Disassembler::getPLTHeader() {

}
void ELF_Disassembler::getGOTHeader() {

}
void ELF_Disassembler::getRelHeader() {

}
void ELF_Disassembler::getEHFrameHeader() {

}

void ELF_Disassembler::getDataHeader()  {

}
void ELF_Disassembler::getTextHeader()  {

}
void ELF_Disassembler::getROnlyHeader()  {

}
void ELF_Disassembler::getBSSHeader()  {

}

ELF_Disassembler::ELF_Disassembler(vector<uint8_t> data): Disassembler(data) {
	this->getSymtabHeader();
	this->getSymtabHeader();
	this->getDynsymHeader();
	this->getStrtabHeader();
	this->getDynstrHeader();
	this->getPLTHeader();
	this->getGOTHeader();
	this->getRelHeader();
	this->getEHFrameHeader();
}