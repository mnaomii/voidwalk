#pragma once
//
// ELF section-parsing suite. Builds a minimal but real ELF (32- and 64-bit) with a
// section header table and a .shstrtab, then checks that ELF_Disassembler resolves the
// section names and populates the base section headers with the right vaddr/offset/size.
// A section that is absent from the table must stay zeroed.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/disassembler/ELF/elf_disassembler.hpp"

class ELF_Sections_Tests : public Tests {

    void checkElf(bool is64, uint16_t machine, const std::string& arch, const std::string& label) {
        const auto fx = fixtures::buildELF(is64, machine);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());
        ELF_Disassembler d(as, {});
        const Sections& s = d.getSections();

        expect_eq(d.getArchitecture(), arch, label + " architecture");
        expect_eq((long long)s._text.getVaddr(),  (long long)fx.text.vaddr,  label + " .text vaddr");
        expect_eq((long long)s._text.getOffset(), (long long)fx.text.offset, label + " .text offset");
        expect_eq((long long)s._text.getSize(),   (long long)fx.text.size,   label + " .text size");
        // .bss is not present in the fixture's section table: it must stay default-zero,
        // proving the parser only writes sections it actually finds.
        expect_eq((long long)s._bss.getOffset(), 0, label + " absent .bss stays zero");
        expect_eq((long long)s._bss.getSize(),   0, label + " absent .bss size zero");
    }

    void runAll() {
        running("testELF32_x86");    checkElf(false, 0x03, "x86",    "ELF32 x86");
        running("testELF64_x86_64"); checkElf(true,  0x3E, "x86_64", "ELF64 x86_64");
    }

public:
    ELF_Sections_Tests() { runAll(); }
};
