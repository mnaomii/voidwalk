#ifndef PE_DISASSEMBLER_H
#define PE_DISASSEMBLER_H
#include "../disassembler.h"

class PE_Disassembler : public Disassembler {
private:
    uint32_t* __idata;
    uint32_t* _edata;
    uint32_t* _rsrc;
    uint32_t* _pdata;
    // uint32_t* _reloc - use for rebasing

    void getIDATAHeader();
    void getEDATAHeader();
    void getRSRCHeader();
    void getPDATAHeader();

    void getDataHeader() override;
    void getTextHeader() override;
    void getROnlyHeader() override;
    void getBSSHeader() override;
public:
    PE_Disassembler(vector<uint8_t> data);
};
#endif