#pragma once
//
// Loader suite - magic-byte detection (determine_filetype) and the make_disassembler
// factory. Covers ELF and PE recognition, the architecture the built disassembler
// reports, and the rejection paths (unrecognised format, "MZ" with no PE signature,
// too-small file) that must raise rather than mis-detect.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/miscellaneous/loader.hpp"

class Loader_Tests : public Tests {

    template <class F>
    bool raises(F&& f) {
        try { f(); return false; }
        catch (const std::exception&) { return true; }
    }

    void testDetectElf() {
        const auto fx = fixtures::buildELF(false, 0x03);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());

        bool is_elf = false, is_pe = false;
        determine_filetype(as, is_elf, is_pe);
        expect(is_elf && !is_pe, "ELF magic detected as ELF only");

        std::shared_ptr<Disassembler> d;
        const std::string status = make_disassembler(as, &d);
        expect(status.find("ELF") != std::string::npos, "make_disassembler reports ELF");
        expect(d != nullptr && d->getArchitecture() == "x86", "ELF disassembler is x86");
    }

    void testDetectPe() {
        const auto fx = fixtures::buildPE(false);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());

        bool is_elf = false, is_pe = false;
        determine_filetype(as, is_elf, is_pe);
        expect(is_pe && !is_elf, "PE magic + signature detected as PE only");

        std::shared_ptr<Disassembler> d;
        const std::string status = make_disassembler(as, &d);
        expect(status.find("PE") != std::string::npos, "make_disassembler reports PE");
        expect(d != nullptr && d->getArchitecture() == "x86", "PE disassembler is x86");
    }

    void testRejectUnknown() {
        // 64 bytes of zero - neither ELF nor PE.
        fixtures::TempBinary tmp(std::vector<uint8_t>(64, 0x00));
        AddressSpace as(tmp.path());

        bool is_elf = false, is_pe = false;
        determine_filetype(as, is_elf, is_pe);
        expect(!is_elf && !is_pe, "zero-filled file matches no format");

        std::shared_ptr<Disassembler> d;
        expect(raises([&]{ make_disassembler(as, &d); }), "unknown format is rejected");
    }

    void testRejectMzWithoutPe() {
        // "MZ" header, e_lfanew -> 0x40, but no "PE\0\0" signature there.
        fixtures::ByteBuf b;
        b.u8('M'); b.u8('Z');
        b.pad_to(0x3C);
        b.u32(0x40);            // e_lfanew
        b.pad_to(0x40);
        b.u32(0xDEADBEEF);      // where the PE signature should be, but isn't
        b.pad_to(0x80);
        fixtures::TempBinary tmp(b.data);
        AddressSpace as(tmp.path());

        bool is_elf = false, is_pe = false;
        determine_filetype(as, is_elf, is_pe);
        expect(!is_pe, "MZ without PE signature is not a PE");

        std::shared_ptr<Disassembler> d;
        expect(raises([&]{ make_disassembler(as, &d); }), "MZ-only stub is rejected");
    }

    void testRejectTooSmall() {
        fixtures::TempBinary tmp(std::vector<uint8_t>{ 0x7f });   // 1 byte: not enough for any magic
        AddressSpace as(tmp.path());

        std::shared_ptr<Disassembler> d;
        expect(raises([&]{ make_disassembler(as, &d); }), "1-byte file is rejected");
    }

    void runAll() {
        running("testDetectElf");         testDetectElf();
        running("testDetectPe");          testDetectPe();
        running("testRejectUnknown");     testRejectUnknown();
        running("testRejectMzWithoutPe"); testRejectMzWithoutPe();
        running("testRejectTooSmall");    testRejectTooSmall();
    }

public:
    Loader_Tests() { runAll(); }
};
