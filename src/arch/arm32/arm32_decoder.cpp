#include "arch/arm32/arm32_decoder.hpp"

#include <stdexcept>

namespace voidwalk {

// Always throws: no ARM32 decoder exists yet.
uint64_t Arm32Decoder::decodeLine(AddressSpace&, uint64_t, uint64_t,
                          std::vector<std::unique_ptr<Instruction>>&) {
    throw std::runtime_error("Not implemented yet.");
}

} // namespace voidwalk
