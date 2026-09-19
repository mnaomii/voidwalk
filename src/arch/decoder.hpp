#pragma once
#include "address_space.hpp"
#include "arch/instruction.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace voidwalk {

// One architecture's instruction decoder. The seam between the arch tier and
// everything above it: no caller of this interface names an architecture.
class Decoder {
public:
    virtual ~Decoder() = default;

    // Decodes the single instruction at file offset `address`, whose runtime
    // address is `vaddr`, reading bytes from `contents`, and appends the result
    // to `decodedInstructions`.
    //
    // Returns the file offset at which the next instruction begins. A return
    // value <= `address` means no forward progress was made - a truncated or
    // unreadable instruction - and callers must stop sweeping rather than retry.
    // Implementations report that case by return value, not by throwing.
    //
    // Throws std::runtime_error if the architecture has no decoder implemented.
    virtual uint64_t decodeLine(AddressSpace& contents,
                                uint64_t address,
                                uint64_t vaddr,
                                std::vector<std::unique_ptr<Instruction>>& decodedInstructions) = 0;
};

} // namespace voidwalk
