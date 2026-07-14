#ifndef PE_DISASSEMBLER_H
#define PE_DISASSEMBLER_H
#include "../disassembler.h"


struct PE_Sections {
    Header _idata, _edata, _rsrc, _pdata;
};

class PE_Disassembler : public Disassembler {
private:
    PE_Sections extraSections;

    uint32_t e_lfanew;
    // uint64_t* _reloc - use for rebasing

    void setHeadersOffsets() override;
public:
    std::string getArchitecture() override;

     uint64_t decodeLine(uint64_t address, uint64_t vaddr) ;


    PE_Disassembler(AddressSpace& data);


};
#endif