#pragma once
#include "arch/decoder.hpp"

namespace voidwalk {

// AArch64 decoder. Not implemented yet; the instruction tables it will need are the
// placeholders beside this file (aarch64_instruction.hpp, aarch64_mnemonic.hpp).
class AArch64Decoder : public Decoder {
public:
    // Always throws std::runtime_error("Not implemented yet.").
    uint64_t decodeLine(AddressSpace& contents,
                        uint64_t address,
                        uint64_t vaddr,
                        std::vector<std::unique_ptr<Instruction>>& decodedInstructions) override;
};

} // namespace voidwalk
