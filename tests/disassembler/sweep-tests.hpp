#pragma once
//
// Linear-sweep integrity suite.
//
// The per-instruction suites (IA-32 / AMD64) check decodeLine in isolation: one byte
// string in, one rendered line out. That is necessary but it is not what a disassembler
// is judged on. What a user actually consumes is the SWEEP - thousands of consecutive
// decodes whose addresses must line up - and a sweep can be wrong in ways no single
// decode is:
//
//   * a length that is one byte short desyncs every later address (the per-instruction
//     suites assert lengths precisely because of this, but they cannot see the knock-on)
//   * an instruction can overlap or leave a hole in .text
//   * the machine-code column can disagree with the bytes actually on disk, which makes
//     the whole listing untrustworthy even where the mnemonics are right
//   * decode() is documented as re-runnable (the GUI/TUI call it again on re-open) and
//     has to actually reset its state
//
// These are whole-run invariants, so they are checked over a whole synthetic .text
// rather than against a table of expected strings. An invariant suite also keeps
// working when the decoder improves: adding SSE support changes what the listing says,
// but never whether the addresses are monotonic.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/disassembler/ELF/elf_disassembler.hpp"

#include <sstream>
#include <string>
#include <vector>

class Sweep_Tests : public Tests {

    // A representative 64-bit stream: the compiler-emitted shapes a sweep meets in
    // real .text - reg-reg moves, RIP-relative loads, SIB addressing, immediates,
    // a relative call, push/pop, and a ret to close the block.
    static std::vector<uint8_t> sampleText64() {
        return {
            0x31, 0xed,                                     // XOR EBP, EBP
            0x55,                                           // PUSH RBP
            0x48, 0x89, 0xe5,                               // MOV RBP, RSP
            0x48, 0x83, 0xec, 0x20,                         // SUB RSP, 0x20
            0x48, 0x8d, 0x05, 0x10, 0x00, 0x00, 0x00,       // LEA RAX, [RIP+0x10]
            0x48, 0x8b, 0x04, 0xc8,                         // MOV RAX, [RAX+RCX*8]
            0xb8, 0x2a, 0x00, 0x00, 0x00,                   // MOV EAX, 0x2a
            0x83, 0xf8, 0x01,                               // CMP EAX, 1
            0x74, 0x02,                                     // JE +2
            0x89, 0xc3,                                     // MOV EBX, EAX
            0xe8, 0x05, 0x00, 0x00, 0x00,                   // CALL rel32
            0x5d,                                           // POP RBP
            0xc3,                                           // RET
        };
    }

    // Parse the "48 89 e5 " machine-code column back into bytes.
    static std::vector<uint8_t> parseMachine(const std::string& s) {
        std::vector<uint8_t> out;
        std::istringstream is(s);
        std::string tok;
        while (is >> tok) {
            try { out.push_back(static_cast<uint8_t>(std::stoul(tok, nullptr, 16))); }
            catch (...) { return out; }
        }
        return out;
    }

    struct Swept {
        std::vector<uint64_t> addrs;
        std::vector<std::string> text, machine;
        uint64_t textOff = 0, textVaddr = 0, textSize = 0;
        size_t ready = 0;
    };

    // Run a full decode over an ELF whose .text is exactly `body`.
    static Swept sweep(const std::vector<uint8_t>& body, bool is64, uint16_t machine) {
        const auto fx = fixtures::buildELF(is64, machine, body);
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());
        ELF_Disassembler d(as, {});
        d.decode();

        Swept s;
        s.textOff   = d.getSections()._text.getOffset();
        s.textVaddr = d.getSections()._text.getVaddr();
        s.textSize  = d.getSections()._text.getSize();
        s.ready     = d.readyInstructions();
        s.addrs     = d.getInstructionAddresses();
        for (const auto& i : d.getDecodedInstructions()) {
            s.text.push_back(i->decodeLineString());
            s.machine.push_back(i->getMachineCode());
        }
        return s;
    }

    // --- invariant 1: the two parallel vectors stay the same length -----------
    // Readers index both with one count (readyInstructions), so a mismatch is an
    // out-of-bounds read in the GUI and TUI, not just a bookkeeping slip.
    void testParallelVectors() {
        const auto s = sweep(sampleText64(), true, 0x3E);
        expect(!s.text.empty(), "sweep produced instructions");
        expect_eq((long long)s.addrs.size(), (long long)s.text.size(),
                  "addresses and instructions are the same length");
        expect_eq((long long)s.ready, (long long)s.text.size(),
                  "readyInstructions() == final instruction count");
    }

    // --- invariant 2: addresses are strictly increasing -----------------------
    // A sweep that stalls or goes backwards would loop forever or render nonsense.
    void testMonotonicAddresses() {
        const auto s = sweep(sampleText64(), true, 0x3E);
        bool monotonic = true;
        size_t badAt = 0;
        for (size_t i = 1; i < s.addrs.size(); ++i)
            if (s.addrs[i] <= s.addrs[i - 1]) { monotonic = false; badAt = i; break; }
        expect(monotonic, monotonic ? "instruction addresses strictly increase"
                                    : "instruction addresses strictly increase (broke at index "
                                      + std::to_string(badAt) + ")");
    }

    // --- invariant 3: every instruction is 1..15 bytes ------------------------
    // 15 is the architectural maximum; 0 would mean the sweep made no progress.
    void testInstructionLengths() {
        const auto s = sweep(sampleText64(), true, 0x3E);
        bool sane = true;
        for (size_t i = 1; i < s.addrs.size(); ++i) {
            const uint64_t len = s.addrs[i] - s.addrs[i - 1];
            if (len == 0 || len > 15) { sane = false; break; }
        }
        expect(sane, "every instruction length is within 1..15 bytes");
    }

    // --- invariant 4: the listing tiles .text with no gaps or overlaps --------
    // Consecutive addresses must differ by exactly the machine-code byte count of
    // the earlier instruction. This is the check that catches a decoder consuming
    // fewer bytes than it rendered - the desync that makes every later line wrong.
    void testNoGapsOrOverlaps() {
        const auto s = sweep(sampleText64(), true, 0x3E);
        size_t mismatches = 0;
        size_t firstBad = 0;
        for (size_t i = 1; i < s.addrs.size(); ++i) {
            const uint64_t stride = s.addrs[i] - s.addrs[i - 1];
            const uint64_t rendered = parseMachine(s.machine[i - 1]).size();
            if (stride != rendered) {
                if (!mismatches) firstBad = i - 1;
                ++mismatches;
            }
        }
        expect(mismatches == 0,
               mismatches == 0
                   ? "listing tiles .text exactly (stride == rendered byte count)"
                   : std::to_string(mismatches) + " instruction(s) render a byte count that "
                     "disagrees with the sweep stride, first at index "
                     + std::to_string(firstBad) + " [" + s.text[firstBad] + "]");
    }

    // --- invariant 5: the machine-code column matches the file ---------------
    // The listing's bytes must be the bytes actually on disk at that address. If
    // they are not, the operand values were reconstructed wrongly even when the
    // mnemonic looks right - and a user patching from this listing would corrupt
    // the target.
    void testMachineCodeMatchesFile() {
        const auto body = sampleText64();
        const auto s = sweep(body, true, 0x3E);
        size_t mismatches = 0;
        size_t firstBad = 0;
        for (size_t i = 0; i < s.machine.size(); ++i) {
            const auto rendered = parseMachine(s.machine[i]);
            const uint64_t fileOff = s.addrs[i] - s.textVaddr;     // .text-relative
            if (fileOff + rendered.size() > body.size()) continue;  // tail, not a mismatch
            for (size_t k = 0; k < rendered.size(); ++k)
                if (rendered[k] != body[static_cast<size_t>(fileOff) + k]) {
                    if (!mismatches) firstBad = i;
                    ++mismatches;
                    break;
                }
        }
        expect(mismatches == 0,
               mismatches == 0
                   ? "machine-code column reproduces the on-disk bytes"
                   : std::to_string(mismatches) + " instruction(s) render bytes that differ "
                     "from the file, first at index " + std::to_string(firstBad)
                     + " [" + s.text[firstBad] + "]");
    }

    // --- invariant 6: the sweep stays inside .text ---------------------------
    // Running past the end would disassemble .rodata (or off the file) and present
    // it as code.
    void testStaysInsideText() {
        const auto s = sweep(sampleText64(), true, 0x3E);
        bool inside = true;
        for (uint64_t a : s.addrs)
            if (a < s.textVaddr || a >= s.textVaddr + s.textSize) { inside = false; break; }
        expect(inside, "every decoded address lies inside .text");
    }

    // --- invariant 7: decode() is re-runnable --------------------------------
    // Both frontends call decode() again when the user opens another binary, and
    // the header documents it as restartable. A second run must reproduce the
    // first exactly - not append to it, not double-count.
    void testDecodeIsRepeatable() {
        const auto fx = fixtures::buildELF(true, 0x3E, sampleText64());
        fixtures::TempBinary tmp(fx.bytes);
        AddressSpace as(tmp.path());
        ELF_Disassembler d(as, {});

        d.decode();
        const size_t firstCount = d.getDecodedInstructions().size();
        std::vector<uint64_t> firstAddrs = d.getInstructionAddresses();

        d.decode();
        const size_t secondCount = d.getDecodedInstructions().size();

        expect_eq((long long)secondCount, (long long)firstCount,
                  "second decode() yields the same instruction count");
        expect(d.getInstructionAddresses() == firstAddrs,
               "second decode() yields identical addresses");
        expect_eq((long long)d.readyInstructions(), (long long)secondCount,
                  "readyInstructions() is reset and republished by the second decode()");
    }

    // --- invariant 8: an empty .text is handled, not guessed at --------------
    void testEmptyText() {
        const auto s = sweep({}, true, 0x3E);
        expect_eq((long long)s.text.size(), 0, "empty .text decodes to zero instructions");
        expect_eq((long long)s.ready, 0, "empty .text publishes a ready count of zero");
    }

    // --- invariant 9: the same properties hold in 32-bit mode ---------------
    // IA-32 is the mode the project reports as complete, so its sweep is the one
    // that should be cleanest.
    void testSweep32() {
        const std::vector<uint8_t> body = {
            0x55,                               // PUSH EBP
            0x89, 0xe5,                         // MOV EBP, ESP
            0x83, 0xec, 0x10,                   // SUB ESP, 0x10
            0xb8, 0x01, 0x00, 0x00, 0x00,       // MOV EAX, 1
            0x8b, 0x45, 0x08,                   // MOV EAX, [EBP+8]
            0x89, 0xc3,                         // MOV EBX, EAX
            0xc9,                               // LEAVE
            0xc3,                               // RET
        };
        const auto s = sweep(body, false, 0x03);
        expect_eq((long long)s.text.size(), 8, "IA-32 sweep decodes all 8 instructions");

        size_t mismatches = 0;
        for (size_t i = 1; i < s.addrs.size(); ++i)
            if (s.addrs[i] - s.addrs[i - 1] != parseMachine(s.machine[i - 1]).size())
                ++mismatches;
        expect(mismatches == 0, "IA-32 listing tiles .text exactly");

        // Coverage stated as bytes rather than a hand-counted instruction total: the
        // sweep must consume the whole section, so the last instruction's address plus
        // its own length has to land exactly on the end of .text. This is the form that
        // does not need updating every time the fixture changes.
        if (!s.addrs.empty()) {
            const uint64_t consumed = (s.addrs.back() - s.textVaddr)
                                    + parseMachine(s.machine.back()).size();
            expect_eq((long long)consumed, (long long)body.size(),
                      "IA-32 sweep consumes every byte of .text");
        }
    }

    void runAll() {
        running("testParallelVectors");       testParallelVectors();
        running("testMonotonicAddresses");    testMonotonicAddresses();
        running("testInstructionLengths");    testInstructionLengths();
        running("testNoGapsOrOverlaps");      testNoGapsOrOverlaps();
        running("testMachineCodeMatchesFile");testMachineCodeMatchesFile();
        running("testStaysInsideText");       testStaysInsideText();
        running("testDecodeIsRepeatable");    testDecodeIsRepeatable();
        running("testEmptyText");             testEmptyText();
        running("testSweep32");               testSweep32();
    }

public:
    Sweep_Tests() { runAll(); }
};
