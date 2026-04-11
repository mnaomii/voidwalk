#ifndef ELF_DISASSEMBLER_H
#define ELF_DISASSEMBLER_H
#include "../disassembler.h"

class ELF_Disassembler : public Disassembler {
private:
    uint64_t* __symtab;
    uint64_t* _dynsym;
    uint64_t* _strtab;
    uint64_t* _dynstr;
    uint64_t* _plt;
    uint64_t* _got;
    uint64_t* _rel;
    uint64_t* _eh_frame;

    void setHeadersOffsets() override;

public:
    ELF_Disassembler(vector<uint8_t> data);
};
#endif