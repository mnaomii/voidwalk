#ifndef PE_DISASSEMBLER_H
#define PE_DISASSEMBLER_H
#include "../disassembler.h"
#include "../../address_space/address_space.h"


class PE_Disassembler : public Disassembler {
private:
    header _idata;
    header _edata;
    header _rsrc;
    header _pdata;
    // uint64_t* _reloc - use for rebasing

    void setHeadersOffsets() override;
    std::string getArchitecture() override;
public:
    PE_Disassembler(AddressSpace& data);
};
#endif