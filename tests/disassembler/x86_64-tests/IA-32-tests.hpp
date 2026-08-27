#pragma once
//
// IA-32 (32-bit, is64Bit == false) decoder suite.
//
// Every expected string here is a decode the objdump audit (audit-objdump-2026-08-11.md
// and its update banners) verified on-disk, or a direct trace of the current tables.
// Comparisons are whitespace-insensitive (see norm()); we assert both the rendered line
// and the instruction *length*, because in a linear sweep a wrong length desyncs every
// later address.
//
#include "x86_64-base.hpp"
#include "../../base.hpp"

class IA_32_Tests : public Tests {
    static constexpr bool M32 = false;   // 32-bit mode

    void eq(const std::vector<uint8_t>& bytes, const std::string& want,
            uint64_t len, const std::string& label) {
        const auto d = x86dec::decodeOne(bytes, M32);
        expect_eq(x86dec::norm(d.text), x86dec::norm(want), label + " text");
        expect_eq((long long)d.length, (long long)len, label + " length");
    }
    void len(const std::vector<uint8_t>& bytes, uint64_t n, const std::string& label) {
        const auto d = x86dec::decodeOne(bytes, M32);
        expect_eq((long long)d.length, (long long)n, label + " length");
    }
    // For instructions that also carry implicit operands (string ops render "[EDI], AX"
    // etc.): assert the mnemonic prefix and the length, which is what selects text16/etc.
    void startsWith(const std::vector<uint8_t>& bytes, const std::string& mne,
                    uint64_t n, const std::string& label) {
        const auto d = x86dec::decodeOne(bytes, M32);
        expect(x86dec::norm(d.text).rfind(mne, 0) == 0, label + " mnemonic '" + mne + "'");
        expect_eq((long long)d.length, (long long)n, label + " length");
    }

    // No-operand, single-byte opcodes: the simplest possible sweep units.
    void testTrivial() {
        eq({0x90}, "NOP",   1, "0x90 NOP");
        eq({0xc3}, "RET",   1, "0xc3 RET");
        eq({0xcc}, "INT3",  1, "0xcc INT3");
        eq({0xc9}, "LEAVE", 1, "0xc9 LEAVE");
        eq({0xf4}, "HLT",   1, "0xf4 HLT");
    }

    // ModRM register-direct form (mod == 11): both operand orders (Ev,Gv and Gv,Ev).
    void testModRMRegDirect() {
        eq({0x89, 0xc3}, "MOV EBX, EAX", 2, "0x89 MOV Ev,Gv");
        eq({0x8b, 0xc3}, "MOV EAX, EBX", 2, "0x8b MOV Gv,Ev");
    }

    // Group-1/2 immediates: 0x83's imm8 sign-extends to the operand width (SIZE::bs),
    // while shift counts (0xC1) and the byte form (0x80) stay unsigned.
    void testImmediates() {
        eq({0x83, 0xe4, 0xf0}, "AND ESP, 0xfffffff0", 3, "0x83 /4 sign-extended imm8");
        eq({0x80, 0xc3, 0xff}, "ADD BL, 0xff",         3, "0x80 /0 unsigned imm8 (byte form)");
        eq({0xc1, 0xe0, 0xff}, "SHL EAX, 0xff",        3, "0xc1 /4 unsigned shift count");
        eq({0x6a, 0xff},       "PUSH 0xffffffff",      2, "0x6a PUSH sign-extended imm8");
        eq({0xcd, 0x80},       "INT 0x80",             2, "0xcd INT imm8");
    }

    // Operand-size prefix 0x66: the mnemonic flips to its 16-bit name (text16) and the
    // sign-extended immediate widens to 16 bits, not 32.
    void testOpsizePrefix() {
        startsWith({0x66, 0xab},     "STOSW",         2, "66 ab STOSW (text16 flip)");
        eq({0x66, 0x98},             "CBW",           2, "66 98 CBW");
        eq({0x66, 0x99},             "CWD",           2, "66 99 CWD");
        eq({0x66, 0x83, 0xc0, 0xff}, "ADD AX, 0xffff",4, "66 83 /0 imm8 -> imm16 sign-extend");
    }

    // Address-size prefix 0x67: 16-bit ModRM addressing forms ([bx+si] etc.). These used
    // to desync; the byte-eater now gates SIB/disp on hasAddrSize so lengths are right.
    void testAddrsizePrefix() {
        eq({0x66, 0x67, 0x8b, 0x00},             "MOV AX, [BX + SI]", 4, "66 67 8b /0 [bx+si]");
        eq({0x66, 0x67, 0x8b, 0x06, 0x34, 0x12}, "MOV AX, [0x1234]",  6, "66 67 8b /6 disp16 absolute");
    }

    // Two-byte 0F map: no-ModRM system ops, ModRM reg/mem ops, and a rel32 Jcc.
    void testTwoByte() {
        eq({0x0f, 0x05},       "SYSCALL",      2, "0f 05 SYSCALL");
        eq({0x0f, 0xa2},       "CPUID",        2, "0f a2 CPUID");
        eq({0x0f, 0xb6, 0xc3}, "MOVZX EAX, BL",3, "0f b6 MOVZX Gv,Eb");
        eq({0x0f, 0xaf, 0xc3}, "IMUL EAX, EBX",3, "0f af IMUL Gv,Ev");
        // rel32 target depends on vaddr; assert mnemonic + length only.
        startsWith({0x0f, 0x84, 0x00, 0x00, 0x00, 0x00}, "JZ", 6, "0f 84 JZ rel32");
    }

    // x87 FPU escapes D8-DF: rendering has corner cases, but LENGTH must always be right
    // (the ModRM/SIB/disp are consumed) or the sweep loses alignment over FPU code.
    void testX87Length() {
        len({0xd9, 0xe0},                         2, "d9 e0 (FCHS, reg form)");
        len({0xd8, 0xc1},                         2, "d8 c1 (FADD ST,ST(1))");
        len({0xdd, 0x00},                         2, "dd 00 (mem form, no disp)");
        len({0xd9, 0x05, 0x17, 0x10, 0x40, 0x00}, 6, "d9 05 (mem form, disp32 absolute)");
    }

    // Byte-eater robustness: an instruction truncated at end-of-file must report
    // no forward progress, never read past the file.
    void testTruncation() {
        const auto d = x86dec::decodeExact({0x68}, M32);   // PUSH imm32, but no imm bytes
        expect(d.truncated, "truncated PUSH imm32 makes no progress");
        expect_eq((long long)d.length, 0, "truncated instruction length == 0");
    }

    void runAll() {
        running("testTrivial");        testTrivial();
        running("testModRMRegDirect"); testModRMRegDirect();
        running("testImmediates");     testImmediates();
        running("testOpsizePrefix");   testOpsizePrefix();
        running("testAddrsizePrefix"); testAddrsizePrefix();
        running("testTwoByte");        testTwoByte();
        running("testX87Length");      testX87Length();
        running("testTruncation");     testTruncation();
    }

public:
    IA_32_Tests() { runAll(); }
};
