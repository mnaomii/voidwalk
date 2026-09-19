#pragma once
#include "disassembler/disassembler.hpp"
#include "disassembler/format/section.hpp"
#include "address_space.hpp"
#include "arch/instruction.hpp"



namespace voidwalk {

// ELF-only sections, beyond the four in Sections.
struct ELF_Sections {
    Header _symtab, _dynsym, _strtab, _dynstr, _plt, _got, _rel, _eh_frame;
};


class ELF_Disassembler : public Disassembler {
private:
    ELF_Sections extraSections;

    // Fills baseSections and extraSections from the ELF section header table.
    // Throws std::runtime_error if the architecture was not recognised.
    void setHeadersOffsets() override;


public:
    // Reads e_machine from `data`, selects the decoder, and parses the section
    // table. `outputs` receives each decoded line as the sweep emits it.
    ELF_Disassembler(AddressSpace& data , const std::vector<std::ostream*>& outputs);



};

} // namespace voidwalk
