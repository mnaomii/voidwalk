#ifndef ELF_DISASSEMBLER_H
#define ELF_DISASSEMBLER_H
#include "../disassembler.h"
#include "../miscellaneous/sections/base/header.h"
#include "../../address-space/address_space.h"
#include "../mnemonic/instruction.h"

#pragma once


struct ELF_Sections {
    Header _symtab, _dynsym, _strtab, _dynstr, _plt, _got, _rel, _eh_frame;
};


class ELF_Disassembler : public Disassembler {
private:
    ELF_Sections extraSections;

    void setHeadersOffsets() override;


public:
    std::string getArchitecture() override;
    ELF_Disassembler(AddressSpace& data);
     uint64_t decodeLine(uint64_t address, uint64_t vaddr) ;



};
#endif