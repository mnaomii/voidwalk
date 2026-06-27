#ifndef PE_DISASSEMBLER_H
#define PE_DISASSEMBLER_H
#include "../disassembler.h"
#include "../miscellaneous/sections/base/header.h"

#include "../../address-space/address_space.h"

struct PE_Sections {
    Header _idata, _edata, _rsrc, _pdata;
};

class PE_Disassembler : public Disassembler {
private:
    PE_Sections extraSections;

    uint32_t e_lfanew;
    // uint64_t* _reloc - use for rebasing

    void setHeadersOffsets() override;
    std::string getArchitecture() override;
public:
    PE_Disassembler(AddressSpace& data);
    std::string decodeLine() override;

};
#endif