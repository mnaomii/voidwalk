#include "arch/aarch64/aarch64_decoder.hpp"

#include <stdexcept>

namespace voidwalk {

// Always throws: no AArch64 decoder exists yet.
uint64_t AArch64Decoder::decodeLine(AddressSpace&, uint64_t, uint64_t,
                          std::vector<std::unique_ptr<Instruction>>&) {
    throw std::runtime_error("Not implemented yet.");
}

} // namespace voidwalk
