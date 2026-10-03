#pragma once
#include "disassembler/disassembler.hpp"


namespace voidwalk {

// PE-only sections, beyond the four in Sections. Parsed and stored, but nothing
// reads them yet - idata is what an import table would be built from.
//
// Any section the file lacks stays default-constructed (all zero).
struct PE_Sections {
    Header idata,   // import directory
           edata,   // export directory
           rsrc,    // resources
           pdata;   // exception / unwind data
};

// Disassembler for PE containers.
//
// Reads the architecture from the COFF Machine field and the sections from the
// section table, both located through e_lfanew (the PE signature offset stored at
// 0x3C). Also records the optional header's ImageBase, which PE section headers
// need since they store RVAs rather than absolute addresses.
class PE_Disassembler : public Disassembler {
private:
    PE_Sections extraSections;

    // File offset of the PE signature, read from the DOS header at 0x3C. Every
    // other header offset in this format is relative to it.
    uint32_t peHeaderOffset;
    // uint64_t* _reloc - use for rebasing

    // Fills commonSections from the PE section table and sets imageBase.
    // Throws std::runtime_error if the architecture was not recognised.
    void setHeadersOffsets() override;
public:


    // Reads the COFF Machine field from `data`, selects the decoder, and parses
    // the section table. `outputs` receives each decoded line as it is emitted.
    PE_Disassembler(AddressSpace& data, const std::vector<std::ostream*>& outputs);


};

} // namespace voidwalk
