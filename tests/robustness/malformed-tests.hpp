#pragma once
//
// Robustness suite - hostile and malformed input.
//
// This is the suite a binary-analysis tool most needs and the one the project did not
// have. Every other suite feeds the decoder input it was designed for. This one feeds
// it the input it will actually meet: truncated downloads, packed and hand-edited
// executables, headers that lie about their own file, and byte sequences that are legal
// x86 but that no compiler emits.
//
// The contract under test is narrow and absolute:
//
//     For ANY sequence of bytes, voidwalk either parses it or rejects it.
//     It never crashes, never reads out of bounds, never hangs.
//
// A tool that inspects untrusted files has no weaker version of this property available
// to it - a crash on a malformed header is a bug, and a memory-safety fault on one is a
// vulnerability, because the input is chosen by whoever wrote the sample.
//
// Each candidate runs in a forked child (see isolate.hpp) so a genuine fault is observed
// and reported as one failed check instead of taking the whole runner down.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../isolate.hpp"
#include "../../main/miscellaneous/loader.hpp"
#include "../../main/disassembler/ELF/elf_disassembler.hpp"

#include <memory>
#include <sstream>
#include <vector>

class Malformed_Tests : public Tests {

    // Load + full sweep, the exact path every frontend takes on "Open".
    static void loadAndDecode(const std::vector<uint8_t>& image) {
        fixtures::TempBinary tmp(image);
        AddressSpace as(tmp.path());
        std::shared_ptr<Disassembler> d;
        make_disassembler(as, &d);   // may legitimately throw: rejection is fine
        d->decode();
    }

    // Assert the contract. `ref` is set for inputs that are known to violate it today;
    // those are recorded as expected failures so the count stays visible without
    // painting CI red for an already-written-up defect.
    //
    // A `ref`'d case is SKIPPED in an uninstrumented build. Its failure mode is UB that
    // does not necessarily fault - reading one slot before a stack array usually just
    // returns the neighbouring bytes - so a plain build would report the defect as
    // passing and, worse, raise XPASS telling the reader to delete the marker for a bug
    // that is still there. Under ASan/UBSan the same input aborts and the check is
    // meaningful. Cases with no `ref` are ordinary regression guards and always run.
    void mustNotCrash(const std::vector<uint8_t>& image, const std::string& what,
                      const std::string& ref = {}) {
        if (!isolate::supported) { skip(what, "no crash isolation on this platform"); return; }
        if (!ref.empty() && !isolate::instrumented) {
            skip(what, "needs -DVOIDWALK_SANITIZE=ON to be conclusive");
            return;
        }
        const auto outcome = isolate::run([&]{ loadAndDecode(image); });
        const bool ok = (outcome != isolate::Outcome::Crashed);
        const std::string label = what + " -> " + isolate::describe(outcome);
        if (ref.empty()) expect(ok, label);
        else             expect_xfail(ok, label, ref);
    }

    // An ELF whose .text is exactly `body`, so a hostile *instruction stream* reaches
    // the byte-eater through the real loader rather than a probe subclass.
    static std::vector<uint8_t> elfWithText(const std::vector<uint8_t>& body) {
        return fixtures::buildELF(true, 0x3E, body).bytes;
    }

    // --- hostile instruction streams ----------------------------------------
    // Both of these are legal byte sequences that a linear sweep meets in padding,
    // jump tables and data-in-text. Neither is exotic.
    void testInstructionStreams() {
        // These two closed A1 and A2 between them. A1 was the out-of-bounds *write* of
        // the REX byte past instructionBytes[15], fixed by the `cnt < 15` guard on the
        // REX branch; that made the input fall through with positions[endOpcode] == 0
        // into A2's read of instructionBytes[-1], which the `cnt < 15` guard on the
        // whole opcode block now prevents. Both are regression guards, and they stay
        // separate because they diverge again the moment either guard regresses.
        //
        // They carry no `ref`, so they run in an uninstrumented build too - but the
        // verdict is only meaningful under ASan/UBSan, since reading one slot before a
        // stack array usually just returns neighbouring bytes without faulting.
        mustNotCrash(elfWithText(fixtures::streams::maxPrefixesThenRex()),
                     "15 legacy prefixes followed by a REX byte");

        mustNotCrash(elfWithText(fixtures::streams::maxPrefixesThenOpcode()),
                     "15 legacy prefixes followed by an opcode");

        // Fewer prefixes than the cap must stay clean - this is the control that
        // proves the two above are about the boundary, not about prefixes at all.
        mustNotCrash(elfWithText(fixtures::streams::prefixRun(0x48, 8)),
                     "8 prefixes followed by a REX byte (under the cap)");

        // A run of 0x0F escapes: every one starts a two-byte opcode that is not there.
        mustNotCrash(elfWithText(std::vector<uint8_t>(64, 0x0f)),
                     "64 consecutive 0x0F escape bytes");

        // 0xFF is a group opcode; /7 is an illegal extension.
        mustNotCrash(elfWithText(std::vector<uint8_t>(64, 0xff)),
                     "64 consecutive 0xFF (group 5) bytes");

        // Random-looking high bytes: the region of the map with the most holes.
        mustNotCrash(elfWithText(std::vector<uint8_t>(64, 0xd8)),
                     "64 consecutive 0xD8 (x87 escape) bytes");

        // .text that ends mid-instruction: the byte-eater must stop, not read on.
        mustNotCrash(elfWithText({0x48, 0x8b}),
                     ".text ending mid-instruction");
    }

    // --- headers that lie about the file ------------------------------------
    void testElfHeaderFields() {
        using namespace fixtures;

        // sh_offset large enough that `offset + sizeof(T)` wraps past the bounds check.
        {
            auto img = buildELF(true, 0x3E).bytes;
            const size_t f = elf64SectionField(img, 2 /*.shstrtab*/, elf64_sh::sh_offset);
            ByteBuf b; b.data = std::move(img); b.patch64(f, 0xFFFFFFFFFFFFFFFFull);
            // No xfail marker: readType() now subtracts instead of adding, so this is
            // held to the real standard. It is the regression test for that fix.
            mustNotCrash(b.data, "shstrtab sh_offset = 2^64-1 (bounds check overflow)");
        }

        // The same overflow reached through .text's own fields.
        {
            auto img = buildELF(true, 0x3E).bytes;
            const size_t f = elf64SectionField(img, 1 /*.text*/, elf64_sh::sh_offset);
            ByteBuf b; b.data = std::move(img); b.patch64(f, 0xFFFFFFFFFFFFFFF0ull);
            mustNotCrash(b.data, ".text sh_offset near 2^64");
        }

        // .text claims to be enormous: end = start + size overflows, and the
        // worst-case reserve() asks for an absurd allocation.
        {
            auto img = buildELF(true, 0x3E).bytes;
            const size_t f = elf64SectionField(img, 1, elf64_sh::sh_size);
            ByteBuf b; b.data = std::move(img); b.patch64(f, 0xFFFFFFFFFFFFFFFFull);
            mustNotCrash(b.data, ".text sh_size = 2^64-1 (sweep bound overflows)");
        }

        // A large but not absurd .text size, past the end of the file.
        {
            auto img = buildELF(true, 0x3E).bytes;
            const size_t f = elf64SectionField(img, 1, elf64_sh::sh_size);
            ByteBuf b; b.data = std::move(img); b.patch64(f, 0x10000000ull);
            mustNotCrash(b.data, ".text sh_size far past EOF");
        }

        // Section table pointing outside the file.
        mustNotCrash(elfWith64(true, 0x3E, elf64::e_shoff, 0xFFFFFFFF00000000ull),
                     "e_shoff outside the file");

        // More sections than exist - the loop runs e_shnum times regardless.
        mustNotCrash(elfWith16(true, 0x3E, elf64::e_shnum, 0xFFFF),
                     "e_shnum = 65535");

        // The string-table index is used unchecked to find the name blob.
        mustNotCrash(elfWith16(true, 0x3E, elf64::e_shstrndx, 0xFFFF),
                     "e_shstrndx = 65535 (also the SHN_XINDEX escape value)");

        // e_shentsize * count is computed in `int`; both at maximum overflows it.
        mustNotCrash(elfWith16(true, 0x3E, elf64::e_shentsize, 0xFFFF),
                     "e_shentsize = 65535 (signed overflow in header arithmetic)");

        // A zero entry size makes every section header alias section 0.
        mustNotCrash(elfWith16(true, 0x3E, elf64::e_shentsize, 0),
                     "e_shentsize = 0");

        // A machine the parser does not know must be rejected cleanly.
        mustNotCrash(elfWith16(true, 0x3E, elf64::e_machine, 0xF3),
                     "unknown e_machine (RISC-V)");
    }

    // --- truncation ----------------------------------------------------------
    // The most common corruption in the wild, and the one with the most read paths
    // to fall off: every header field past the cut is a read past EOF.
    void testTruncation() {
        const auto full = fixtures::buildELF(true, 0x3E).bytes;
        for (size_t n : {size_t(4), size_t(5), size_t(16), size_t(20), size_t(52),
                         size_t(64), full.size() / 2, full.size() - 1}) {
            if (n >= full.size()) continue;
            mustNotCrash(fixtures::truncated(full, n),
                         "ELF truncated to " + std::to_string(n) + " bytes");
        }

        const auto pe = fixtures::buildPE(true).bytes;
        for (size_t n : {size_t(2), size_t(0x3C), size_t(0x40), size_t(0x50), pe.size() / 2}) {
            if (n >= pe.size()) continue;
            mustNotCrash(fixtures::truncated(pe, n),
                         "PE truncated to " + std::to_string(n) + " bytes");
        }
    }

    // --- PE headers ----------------------------------------------------------
    void testPeHeaderFields() {
        using namespace fixtures;

        // NumberOfSections beyond what the file holds.
        {
            auto f = buildPE(true);
            ByteBuf b; b.data = std::move(f.bytes);
            b.patch16(0x40 + 6, 0xFFFF);
            mustNotCrash(b.data, "PE NumberOfSections = 65535");
        }

        // SizeOfOptionalHeader pushes the section table past EOF.
        {
            auto f = buildPE(true);
            ByteBuf b; b.data = std::move(f.bytes);
            b.patch16(0x40 + 20, 0xFFFF);
            mustNotCrash(b.data, "PE SizeOfOptionalHeader = 65535");
        }

        // e_lfanew pointing into the middle of nowhere. determine_filetype checks the
        // signature, so this must be REJECTED rather than parsed.
        {
            auto f = buildPE(true);
            ByteBuf b; b.data = std::move(f.bytes);
            b.patch32(0x3C, 0x7FFFFFFF);
            mustNotCrash(b.data, "PE e_lfanew = 0x7FFFFFFF");
        }

        // An unknown Machine currently leaves every section zeroed instead of
        // throwing; it must at least not fault.
        {
            auto f = buildPE(true);
            ByteBuf b; b.data = std::move(f.bytes);
            b.patch16(0x40 + 4, 0x5032);   // not a machine voidwalk knows
            mustNotCrash(b.data, "PE unknown Machine value");
        }
    }

    // --- degenerate files ----------------------------------------------------
    void testDegenerate() {
        mustNotCrash({}, "zero-byte file");
        mustNotCrash({0x7f}, "one-byte file");
        mustNotCrash({0x7f, 'E', 'L', 'F'}, "ELF magic and nothing else");
        mustNotCrash({'M', 'Z'}, "MZ magic and nothing else");
        mustNotCrash(std::vector<uint8_t>(4096, 0x00), "4 KiB of zeros");
        mustNotCrash(std::vector<uint8_t>(4096, 0xFF), "4 KiB of 0xFF");
    }

    // --- output path ---------------------------------------------------------
    // The CLI's --print mode streams each line as it decodes. That path keeps a
    // cursor into the instruction vectors which decode() does not reset, so a
    // second sweep on the same object indexes past the end.
    void testStreamingOutputIsRepeatable() {
        if (!isolate::supported) { skip("streaming decode twice", "no crash isolation"); return; }
        const auto outcome = isolate::run([]{
            const auto fx = fixtures::buildELF(true, 0x3E);
            fixtures::TempBinary tmp(fx.bytes);
            AddressSpace as(tmp.path());
            std::ostringstream sink;
            std::vector<std::ostream*> streams{ &sink };
            ELF_Disassembler d(as, streams);
            d.decode();
            d.decode();   // instrDecodePos is never reset
        });
        expect_xfail(outcome != isolate::Outcome::Crashed,
                     std::string("decode() twice with an output stream -> ")
                         + isolate::describe(outcome),
                     "AUDIT.md E5 (instrDecodePos not reset, disassembler.cpp:398)");
    }

    void runAll() {
        if (!isolate::supported)
            std::cout << "        " << test_console::dim
                      << "(no fork on this platform: a fault here aborts the runner)"
                      << test_console::reset << '\n';
        running("testInstructionStreams");         testInstructionStreams();
        running("testElfHeaderFields");            testElfHeaderFields();
        running("testTruncation");                 testTruncation();
        running("testPeHeaderFields");             testPeHeaderFields();
        running("testDegenerate");                 testDegenerate();
        running("testStreamingOutputIsRepeatable");testStreamingOutputIsRepeatable();
    }

public:
    Malformed_Tests() { runAll(); }
};
