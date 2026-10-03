//
// ELF section-parsing suite. Builds a minimal but real ELF (32- and 64-bit) with a
// section header table and a .shstrtab, then checks that ELF_Disassembler resolves the
// section names and populates the base section headers with the right vaddr/offset/size.
// A section that is absent from the table must leave its list empty.
//
#include "tests/framework/base.hpp"
#include "tests/framework/fixtures.hpp"
#include "disassembler/format/elf/elf_disassembler.hpp"

class ELF_Sections_Tests : public Tests {

    void checkElf(bool is64, uint16_t machine, const std::string& arch, const std::string& label) {
        const auto fx = fixtures::buildELF(is64, machine);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());
        ELF_Disassembler d(as, {});
        const Sections& s = d.getSections();

        expect_eq(d.getArchitecture(), arch, label + " architecture");
        // The fixture's section table has exactly one .text.
        if (expect_eq((long long)s.text.size(), 1, label + " exactly one .text")) {
            const Header& text = s.text.front();
            expect_eq((long long)text.getVaddr(),  (long long)fx.text.vaddr,  label + " .text vaddr");
            expect_eq((long long)text.getOffset(), (long long)fx.text.offset, label + " .text offset");
            expect_eq((long long)text.getSize(),   (long long)fx.text.size,   label + " .text size");
        }
        // .bss is not present in the fixture's section table: its list must stay empty,
        // proving the parser only records sections it actually finds.
        expect_eq((long long)s.bss.size(), 0, label + " absent .bss stays empty");
    }

    void runAll() {
        running("testELF32_x86");    checkElf(false, 0x03, "x86",    "ELF32 x86");
        running("testELF64_x86_64"); checkElf(true,  0x3E, "x86_64", "ELF64 x86_64");
    }

public:
    ELF_Sections_Tests() { runAll(); }
};


// Entry point for this suite. Declared in tests/suites.hpp; the checks run in
// ELF_Sections_Tests's constructor, and the shared Tests counters aggregate the results.
void run_elf_sections_tests() { ELF_Sections_Tests t; }
