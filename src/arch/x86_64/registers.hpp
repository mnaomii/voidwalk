#pragma once
#include <cstdint>

namespace voidwalk {

// Emulated x86/x86-64 register file. Every field reads zero until a debugger
// engine exists to populate it.
struct Registers_x86_64 { // emulating the current values of the registers.
    uint64_t rax, rdx, rcx, rbx, rsp, rbp, rsi, rdi, rip; // registers + eip - current instruction pointer;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t ds, cs, fs, gs, ss, es; // segments

    uint8_t flags;
};

} // namespace voidwalk
