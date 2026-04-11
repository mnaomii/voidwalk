#ifndef ELF_DISASSEMBLER_H
#define ELF_DISASSEMBLER_H
#include "../disassembler.h"

class ELF_Disassembler : public Disassembler {
private:
    uint32_t* __symtab;
    uint32_t* _dynsym;
    uint32_t* _strtab;
    uint32_t* _dynstr;
    uint32_t* _plt;
    uint32_t* _got;
    uint32_t* _rel;
    uint32_t* _eh_frame;

    void getSymtabHeader();
    void getDynsymHeader();
    void getStrtabHeader();
    void getDynstrHeader();
    void getPLTHeader();
    void getGOTHeader();
    void getRelHeader();
    void getEHFrameHeader();

    void getDataHeader() override;
    void getTextHeader() override;
    void getROnlyHeader() override;
    void getBSSHeader() override;


public:
    ELF_Disassembler(vector<uint8_t> data);
};
#endif