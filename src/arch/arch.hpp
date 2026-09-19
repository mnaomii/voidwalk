#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace voidwalk {

class Decoder;

// The instruction set a binary targets, independent of its container format.
// Each format reader translates its own machine numbering into this enum once.
enum class Arch {
    Unknown,
    X86,        // IA-32
    X86_64,     // AMD64
    ARM32,
    AArch64,
};

// Returns the display name of `a`: "x86", "x86_64", "ARM32", "AArch64", or
// "Unknown". These strings appear in CLI/TUI/GUI output and in test expectations.
inline std::string archName(Arch a) {
    switch (a) {
        case Arch::X86:     return "x86";
        case Arch::ARM32:   return "ARM32";
        case Arch::X86_64:  return "x86_64";
        case Arch::AArch64: return "AArch64";
        default:            return "Unknown";
    }
}

// Returns true when `a` uses the 64-bit variant of its container's section
// headers. Both format readers branch on this to pick a section parser.
inline bool is64Bit(Arch a) {
    return a == Arch::X86_64 || a == Arch::AArch64;
}

// Returns the decoder for `a`, or nullptr when no decoder exists for it.
// Defined in src/arch/decoder.cpp.
std::unique_ptr<Decoder> makeDecoder(Arch a);

} // namespace voidwalk
