#pragma once
#include "arch/decoder.hpp"

namespace voidwalk {

// ARM32 decoder. Not implemented yet; the instruction tables it will need are the
// placeholders beside this file (arm32_instruction.hpp, arm32_mnemonic.hpp).
class Arm32Decoder : public Decoder {
public:
    // Always throws std::runtime_error("Not implemented yet.").
    uint64_t decodeLine(AddressSpace& contents,
                        uint64_t address,
                        uint64_t vaddr,
                        std::vector<std::unique_ptr<Instruction>>& decodedInstructions) override;
};

} // namespace voidwalk
