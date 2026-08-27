#pragma once
//
// AMD64 (long-mode, is64Bit == true) decoder suite.
//
// Ground truth is audit-amd64-2026-08-24.md's decodes plus direct table traces. Focus
// areas are the long-mode-only mechanics that WORK today: REX.W width, RIP-relative
// addressing, the SPL/BPL/SIL/DIL byte set, d64 default-64 promotion, and 64-bit invalid
// opcodes.
//
// NOTE (deliberately not tested here): REX.R / REX.B register *extension* to R8..R15 is
// currently broken - x86_64-mnemonic.hpp registerOf() throws for any index > 7 in 64-bit
// mode ("(r > 15 && is64bit) || r > 7"), so 4c 89 c0 (->R8), 49 8b 03 (->R11), 41 55
// (->R13) raise instead of rendering. The amd64 audit lists that extension as resolved,
// but it was code-inspection-only; add R8..R15 cases once the bounds check is fixed to
// "(r > 15) || (r > 7 && !is64bit)".
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

    void runAll() {
        running("testRexWidth");       testRexWidth();
        running("testMemoryForms");    testMemoryForms();
        running("testRipRelative");    testRipRelative();
        running("testByteRegisters");  testByteRegisters();
        running("testMnemonicWidth");  testMnemonicWidth();
        running("testDefault64Width"); testDefault64Width();
        running("testInvalidOpcode");  testInvalidOpcode();
    }

public:
    AMD64_Tests() { runAll(); }
};
