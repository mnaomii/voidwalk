#pragma once
#include "arch/decoder.hpp"

namespace voidwalk {

// The x86/x86-64 length-and-operand decoder.
class X86Decoder : public Decoder {
public:
    // Constructs a decoder for AMD64 when `is64Bit`, otherwise for IA-32. The mode
    // is a property of the binary and fixed for the decoder's lifetime.
    explicit X86Decoder(bool is64Bit) : is64Bit_(is64Bit) {}

    // Decodes one instruction. See Decoder::decodeLine for the full contract.
    // Returns `address` unchanged when the instruction runs past end-of-file.
    uint64_t decodeLine(AddressSpace& contents,
                        uint64_t address,
                        uint64_t vaddr,
                        std::vector<std::unique_ptr<Instruction>>& decodedInstructions) override;

private:
    bool is64Bit_;
};

} // namespace voidwalk
