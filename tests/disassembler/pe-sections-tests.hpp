#pragma once
//
// PE section-parsing suite. Unlike ELF, the PE parser buckets sections by their
// Characteristics flags through classify() (not by name), so the fixture carries four
// sections that land in CODE / DATA / RODATA / BSS respectively. We check every bucket
// for both PE32 and PE32+, including that the section vaddr is imageBase + RVA.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/disassembler/PE/pe_disassembler.hpp"

class PE_Sections_Tests : public Tests {

    void check(const char* which, const Header& h,
               const fixtures::SectionExpect& e, const std::string& label) {
        expect_eq((long long)h.getVaddr(),  (long long)e.vaddr,  label + " " + which + " vaddr");
        expect_eq((long long)h.getOffset(), (long long)e.offset, label + " " + which + " offset");
        expect_eq((long long)h.getSize(),   (long long)e.size,   label + " " + which + " size");
    }

    void checkPe(bool is64, const std::string& arch, const std::string& label) {
        const auto fx = fixtures::buildPE(is64);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());
        PE_Disassembler d(as, {});
        const Sections& s = d.getSections();

        expect_eq(d.getArchitecture(), arch, label + " architecture");
        check(".text (CODE)",   s._text,  fx.text,   label);
        check(".data (DATA)",   s._data,  fx.data,   label);
        check(".rdata (RODATA)",s._ronly, fx.rodata, label);
        check(".bss (BSS)",     s._bss,   fx.bss,    label);
    }

    void runAll() {
        running("testPE32_x86");    checkPe(false, "x86",    "PE32 x86");
        running("testPE64_x86_64"); checkPe(true,  "x86_64", "PE32+ x86_64");
    }

public:
    PE_Sections_Tests() { runAll(); }
};
