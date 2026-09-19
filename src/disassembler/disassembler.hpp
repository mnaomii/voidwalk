#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <stop_token>
#include "address_space.hpp"
#include "disassembler/format/section.hpp"
#include "arch/arch.hpp"
#include "arch/decoder.hpp"
#include "arch/instruction.hpp"
#include "arch/x86_64/registers.hpp"


namespace voidwalk {

// The four sections every container has in common. Format-specific extras live in
// the ELF_Sections / PE_Sections structs beside their own reader.
struct Sections {
    //std::vector<Header> _text, _data, _ronly, _bss;
    Header _text, _data, _ronly, _bss;
};


// Locates .text in a container and sweeps it, one instruction at a time.
//
// Subclasses supply the container format (ELF, PE). The instruction set is reached
// only through `decoder`, so this class names no architecture.
class Disassembler {

private:
    size_t instrDecodePos{};
    std::vector<std::ostream*> outputStreams;



protected:

    uint64_t imageBase{};

    std::vector<std::unique_ptr<Instruction>> decodedInstructions;
    std::vector<uint64_t> instructionAddresses;
    std::vector<uint64_t> virtStack;
    Sections baseSections;
    uint64_t offset;

    // Set by setArch() from the subclass constructor, before setHeadersOffsets().
    Arch arch;
    std::unique_ptr<Decoder> decoder;

    Registers_x86_64 registers{};

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;

    // Records the architecture the subclass read from the container header and
    // builds the matching decoder. Leaves `decoder` null for Arch::Unknown.
    void setArch(Arch a) { arch = a; decoder = makeDecoder(a); }

    std::atomic<size_t> readyCount{0};

public:
    Disassembler(AddressSpace& temp, const std::vector<std::ostream*>& stream) : contents(temp), arch(Arch::Unknown), offset(0x00), outputStreams(stream) {

    };
    void emitDecodedLine();

    // Returns the architecture's display name, e.g. "x86_64".
    std::string getArchitecture() const { return archName(arch); }

    // Returns the architecture this binary targets.
    Arch architecture() const { return arch; }

    // Decodes the instruction at file offset `address` (runtime address `vaddr`)
    // and appends it. Returns the offset of the next instruction; a value
    // <= `address` means no forward progress. Throws std::runtime_error if the
    // architecture was not recognised.
    uint64_t decodeLine(uint64_t address, uint64_t vaddr);

    virtual ~Disassembler() = default;

    void decode(std::stop_token stopToken = {});

    const Registers_x86_64& getRegisters() const { return registers; }
    const std::vector<uint64_t>& getVirtStack() const { return virtStack; }
    const std::vector<std::unique_ptr<Instruction>>& getDecodedInstructions() const { return decodedInstructions; }
    const std::vector<uint64_t>& getInstructionAddresses() const { return instructionAddresses; }
    const Sections& getSections() const { return baseSections; }
    AddressSpace& getAddressSpace() { return contents; }

    size_t readyInstructions() const { return readyCount.load(std::memory_order_acquire); }

};

} // namespace voidwalk
