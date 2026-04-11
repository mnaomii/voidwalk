#ifndef PE_DISASSEMBLER_H
#define PE_DISASSEMBLER_H
#include "../disassembler.h"

class PE_Disassembler : public Disassembler {
private:
    uint64_t* __idata;
    uint64_t* _edata;
    uint64_t* _rsrc;
    uint64_t* _pdata;
    // uint64_t* _reloc - use for rebasing

    void setHeadersOffsets() override;
public:
    PE_Disassembler(vector<uint8_t> data);
};
#endif