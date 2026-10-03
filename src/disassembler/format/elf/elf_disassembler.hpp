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
    Header symtab,   // static symbol table
           dynsym,   // dynamic symbol table
           strtab,   // string table for symtab
           dynstr,   // string table for dynsym
           plt,      // procedure linkage table (import thunks)
           got,      // global offset table
           rel,      // never populated: no parser writes this field
           ehFrame;  // unwind information
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

    // Fills commonSections and extraSections from the ELF section header table.
    // Throws std::runtime_error if the architecture was not recognised.
    void setHeadersOffsets() override;


public:
    // Reads e_machine from `data`, selects the decoder, and parses the section
    // table. `outputs` receives each decoded line as the sweep emits it.
    ELF_Disassembler(AddressSpace& data , const std::vector<std::ostream*>& outputs);



};

} // namespace voidwalk
