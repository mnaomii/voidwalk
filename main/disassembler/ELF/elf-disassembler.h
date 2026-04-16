#ifndef ELF_DISASSEMBLER_H
#define ELF_DISASSEMBLER_H
#include "../disassembler.h"
#include "../../address_space/address_space.h"


class ELF_Disassembler : public Disassembler {
private:
    header _symtab;
    header _dynsym;
    header _strtab;
    header _dynstr;
    header _plt;
    header _got;
    header _rel;
    header _eh_frame;

    void setHeadersOffsets() override;

public:
    ELF_Disassembler(AddressSpace& data);
};
#endif