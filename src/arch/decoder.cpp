#include "arch/arch.hpp"
#include "arch/decoder.hpp"

#include "arch/aarch64/aarch64_decoder.hpp"
#include "arch/arm32/arm32_decoder.hpp"
#include "arch/x86_64/x86_64_decoder.hpp"

namespace voidwalk {

// Returns the decoder for `a`, or nullptr for Arch::Unknown.
//
// Unknown yields nullptr rather than a throwing stub because "the architecture
// was not identified" and "the architecture is known but undecodable" are
// different failures: Disassembler::decodeLine reports the first, the stub
// decoders report the second.
std::unique_ptr<Decoder> makeDecoder(Arch a) {
    switch (a) {
        case Arch::X86:     return std::make_unique<X86Decoder>(false);
        case Arch::X86_64:  return std::make_unique<X86Decoder>(true);
        case Arch::ARM32:   return std::make_unique<Arm32Decoder>();
        case Arch::AArch64: return std::make_unique<AArch64Decoder>();
        default:            return nullptr;
    }
}

} // namespace voidwalk
