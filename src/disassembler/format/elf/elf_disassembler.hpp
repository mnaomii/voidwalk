#pragma once
#include "disassembler/disassembler.hpp"
#include "disassembler/format/section.hpp"
#include "address_space.hpp"
#include "arch/instruction.hpp"



namespace voidwalk {

// ELF-only sections, beyond the four in Sections. Parsed and stored, but nothing
// reads them yet - they are the raw material for the symbol and import tables the
// sidebar currently derives from the disassembly text instead.
//
// Any section the file lacks stays default-constructed (all zero).
struct ELF_Sections {
    Header _symtab,   // static symbol table
           _dynsym,   // dynamic symbol table
           _strtab,   // string table for _symtab
           _dynstr,   // string table for _dynsym
           _plt,      // procedure linkage table (import thunks)
           _got,      // global offset table
           _rel,      // never populated: no parser writes this field
           _eh_frame; // unwind information
};


// Disassembler for ELF containers.
//
// Reads the architecture from e_machine and the sections from the section header
// table, both keyed off the ELF class byte at offset 0x04. Everything after that -
// the sweep, the decoding - is the base class's, so this type exists only to
// answer "where is .text and what machine is it".
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
