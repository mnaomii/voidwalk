#pragma once
//
// AMD64 (long-mode, is64Bit == true) decoder suite.
//
// Ground truth is audit-amd64-2026-08-24.md's decodes plus direct table traces. Focus
// areas are the long-mode-only mechanics that WORK today: REX.W width, RIP-relative
// addressing, the SPL/BPL/SIL/DIL byte set, d64 default-64 promotion, and 64-bit invalid
// opcodes.
//
// REX.R / REX.B register extension to R8..R15 (testRexRegisterExtension) was carried
// here as expect_xfail while AUDIT.md B1 was open. B1 is fixed - registerOf() now bounds
// on the register index and the byte-register rule rather than on REX.W - so those cases
// are plain expectations again, kept as the regression guard for it. Same for the two
// E1 cases: ENDBR64 (testModernPrologue) and unknown-0F length (testUnknownOpcodeLength).
//
// Worth keeping in mind if any of them regress: the fix originally prescribed for B1 -
// widening the bound to "(r > 15) || (r > 7 && !is64bit)" - is NOT sufficient. The
// REX.B-without-REX.W case below is the one it still gets wrong.
//
#include "x86_64-base.hpp"
#include "../../base.hpp"

class AMD64_Tests : public Tests {
    static constexpr bool M64 = true;   // 64-bit mode

    void eq(const std::vector<uint8_t>& bytes, const std::string& want,
            uint64_t len, const std::string& label) {
        const auto d = x86dec::decodeOne(bytes, M64);
        expect_eq(x86dec::norm(d.text), x86dec::norm(want), label + " text");
        expect_eq((long long)d.length, (long long)len, label + " length");
    }
    // Mnemonic-prefix + length, for opcodes that also carry implicit operands.
    void startsWith(const std::vector<uint8_t>& bytes, const std::string& mne,
                    uint64_t len, const std::string& label) {
        const auto d = x86dec::decodeOne(bytes, M64);
        expect(x86dec::norm(d.text).rfind(mne, 0) == 0, label + " mnemonic '" + mne + "'");
        expect_eq((long long)d.length, (long long)len, label + " length");
    }

    // Operand width is driven by REX.W, not CPU mode: no REX keeps the 32-bit register,
    // REX.W widens it to 64.
    void testRexWidth() {
        eq({0x89, 0xc3},       "MOV EBX, EAX", 2, "89 (no REX) keeps 32-bit width");
        eq({0x48, 0x89, 0xc3}, "MOV RBX, RAX", 3, "48 89 REX.W widens to 64-bit");
    }

    // Memory operands: 64-bit base register, and a SIB base+index*scale.
    void testMemoryForms() {
        eq({0x48, 0x8b, 0x03},       "MOV RAX, [RBX]",        3, "48 8b non-SIB 64-bit base");
        eq({0x48, 0x8b, 0x04, 0xc8}, "MOV RAX, [RAX + RCX*8]",4, "48 8b SIB base+index*scale");
    }

    // RIP-relative addressing: mod==00, rm==101 in long mode is [RIP + disp32].
    void testRipRelative() {
        eq({0x48, 0x8b, 0x05, 0x10, 0x00, 0x00, 0x00},
           "MOV RAX, [RIP + 0x10]", 7, "48 8b 05 RIP-relative");
    }

    // Byte registers reached via REX use the SPL/BPL/SIL/DIL set (a bare REX flips
    // AH..BH to SPL..DIL even with no width bit set).
    void testByteRegisters() {
        eq({0x40, 0x88, 0xe6}, "MOV SIL, SPL", 3, "40 88 REX byte-register set");
    }

    // Mnemonic width selection: text64 under REX.W, text16 under 0x66, and REX.W winning
    // over a 0x66 that precedes it.
    void testMnemonicWidth() {
        eq({0x48, 0x98},       "CDQE", 2, "48 98 text64 (CDQE)");
        startsWith({0x48, 0xab},"STOSQ",2, "48 ab text64 (STOSQ)");
        eq({0x66, 0x98},       "CBW",  2, "66 98 text16 (CBW)");
        eq({0x66, 0x48, 0x98}, "CDQE", 3, "66 48 98 REX.W nullifies 0x66");
    }

    // +r register-in-opcode with a d64 default: 0x50 (PUSH) defaults to 64-bit operand
    // size in long mode even with no REX.W, so PUSH rAX is PUSH RAX.
    void testDefault64Width() {
        eq({0x50}, "PUSH RAX", 1, "50 PUSH +r promotes to 64-bit (d64)");
    }

    // An opcode that is legal in 32-bit but invalid in long mode (0x06 PUSH ES) must
    // render "(bad)" and advance exactly one byte so the sweep resyncs.
    void testInvalidOpcode() {
        const auto d = x86dec::decodeOne({0x06}, M64);
        expect_eq(x86dec::norm(d.text), "(bad)", "06 in long mode -> (bad)");
        expect_eq((long long)d.length, 1, "invalid opcode advances one byte");
    }

    // ---- REX.R / REX.B register extension to R8..R15 -------------------------
    // The defining feature of AMD64 and the single most common thing in any 64-bit
    // .text. registerOf() used to bound the register index on its `is64bit` parameter,
    // which callers pass as REX.W ("render a 64-bit-wide name") rather than "we are in
    // long mode", so every index above 7 raised - a runtime_error that decodeLine's
    // length_error handler does not catch, tearing down the whole sweep. Fixed; these
    // are the regression guard.
    //
    // The last case is the important one: REX.B with NO REX.W. It is what makes this a
    // parameter-meaning bug rather than an off-by-one, and it is the case a fix that
    // only widens the bound to 15 still gets wrong.
    void testRexRegisterExtension() {
        eq({0x4c, 0x89, 0xc0}, "MOV RAX, R8",   3, "4c 89 c0 REX.R -> R8");
        eq({0x49, 0x8b, 0x03}, "MOV RAX, [R11]",3, "49 8b 03 REX.B -> R11");
        eq({0x41, 0x55},       "PUSH R13",      2, "41 55 REX.B +r -> R13");
        eq({0x4d, 0x89, 0xcf}, "MOV R15, R9",   3, "4d 89 cf REX.R+B -> R15,R9");

        // REX.B without REX.W: 32-bit operation on an extended register. Pervasive in
        // compiler output, and the case that survives the bounds fix recorded in this
        // file's header comment.
        eq({0x41, 0x89, 0xf5}, "MOV R13D, ESI", 3,
           "41 89 f5 REX.B without REX.W -> R13D");
    }

    // ---- instructions every modern 64-bit binary opens with ------------------
    // endbr64 is the CET landing pad the compiler puts at the head of essentially every
    // function. It used to split into a bare "REP" plus a separate CLI, so both the text
    // and the length were wrong - and a wrong length in a linear sweep desyncs
    // everything after it. F3 is a mandatory prefix here, not a REP, and FA is a ModRM.
    void testModernPrologue() {
        eq({0xf3, 0x0f, 0x1e, 0xfa}, "ENDBR64", 4, "f3 0f 1e fa ENDBR64");
    }

    // ---- unimplemented opcodes must still be length-correct ------------------
    // A decoder is allowed not to know an instruction. It is not allowed to guess its
    // length: the sweep resyncs on length, so an unknown opcode that under-consumes
    // corrupts every later address. movaps is 0F 29 /r - four bytes with this ModRM,
    // whatever the mnemonic ends up being.
    void testUnknownOpcodeLength() {
        const auto d = x86dec::decodeOne({0x0f, 0x29, 0x44, 0x24, 0x50}, M64);
        expect(d.length == 5, "0f 29 44 24 50 (movaps) consumes 5 bytes, got "
                                  + std::to_string(d.length));
    }

    void runAll() {
        running("testRexWidth");            testRexWidth();
        running("testMemoryForms");         testMemoryForms();
        running("testRipRelative");         testRipRelative();
        running("testByteRegisters");       testByteRegisters();
        running("testMnemonicWidth");       testMnemonicWidth();
        running("testDefault64Width");      testDefault64Width();
        running("testInvalidOpcode");       testInvalidOpcode();
        running("testRexRegisterExtension");testRexRegisterExtension();
        running("testModernPrologue");      testModernPrologue();
        running("testUnknownOpcodeLength"); testUnknownOpcodeLength();
    }

public:
    AMD64_Tests() { runAll(); }
};
