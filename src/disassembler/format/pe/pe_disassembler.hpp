#pragma once
#include "disassembler/disassembler.hpp"


namespace voidwalk {

// PE-only sections, beyond the four in Sections.
struct PE_Sections {
    Header _idata, _edata, _rsrc, _pdata;
};

class PE_Disassembler : public Disassembler {
private:
    PE_Sections extraSections;

    uint32_t e_lfanew;
    // uint64_t* _reloc - use for rebasing

    // Fills baseSections from the PE section table and sets imageBase.
    // Throws std::runtime_error if the architecture was not recognised.
    void setHeadersOffsets() override;
public:


    // Reads the COFF Machine field from `data`, selects the decoder, and parses
    // the section table. `outputs` receives each decoded line as it is emitted.
    PE_Disassembler(AddressSpace& data, const std::vector<std::ostream*>& outputs);


};

} // namespace voidwalk
