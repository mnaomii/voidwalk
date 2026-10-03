#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <stop_token>
#include "address_space.hpp"
#include "chunk_store.hpp"
#include "disassembler/format/section.hpp"
#include "arch/arch.hpp"
#include "arch/decoder.hpp"
#include "arch/instruction.hpp"
#include "arch/x86_64/registers.hpp"


namespace voidwalk {

// The four sections every container has in common. Format-specific extras live in
// the ELF_Sections / PE_Sections structs beside their own reader.
struct Sections {
    std::vector<Header> text, data, readOnly, bss;
};


// Locates .text in a container and sweeps it, one instruction at a time.
//
// Subclasses supply the container format (ELF, PE). The instruction set is reached
// only through `decoder`, so this class names no architecture.
class Disassembler {

private:
    size_t nextEmitIndex{};
    std::vector<std::ostream*> outputStreams;


protected:

    uint64_t imageBase{};
    uint64_t entryAddress{};   // entry point vaddr, set by the subclass from its header

    InstructionStore decodedInstructions;
    ChunkStore<uint64_t> instructionAddresses;
    //std::vector<uint64_t> virtStack;
    Sections commonSections;
    uint64_t offset;

    // Set by setArch() from the subclass constructor, before setHeadersOffsets().
    Arch arch;
    std::unique_ptr<Decoder> decoder;

    Registers_x86_64 registers{};

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;

    // Records the architecture the subclass read from the container header and
    // builds the matching decoder. Leaves `decoder` null for Arch::Unknown.
    void setArch(Arch targetArch) { arch = targetArch; decoder = makeDecoder(targetArch); }

    std::atomic<size_t> readyCount{0};

public:
    Disassembler(AddressSpace& addressSpace, const std::vector<std::ostream*>& streams)
        : outputStreams(streams), offset(0x00), arch(Arch::Unknown), contents(addressSpace) {};

    // Prints the last decoded line to all the specified streams.
    void emitDecodedLine(bool showVaddr = true) ;

    // Returns the architecture's display name, e.g. "x86_64".
    std::string getArchitecture() const { return archName(arch); }

    // Returns the architecture this binary targets.
    Arch architecture() const { return arch; }

    // Virtual address execution starts at (ELF e_entry, PE ImageBase + AddressOfEntryPoint).
    uint64_t entryPoint() const { return entryAddress; }

    // Decodes the instruction at file offset `fileOffset` (runtime address `vaddr`)
    // and appends it. Returns the offset of the next instruction; a value
    // <= `fileOffset` means no forward progress. Throws std::runtime_error if the
    // architecture was not recognised.
    uint64_t decodeLine(uint64_t fileOffset, uint64_t vaddr);

    virtual ~Disassembler() = default;

    void decode(std::stop_token stopToken = {});

    void compact() {
        decodedInstructions.shrinkToFit();
        instructionAddresses.shrinkToFit();
    }

    const Registers_x86_64& getRegisters() const { return registers; }
    //const std::vector<uint64_t>& getVirtStack() const { return virtStack; }
    // Both are chunked, not contiguous: the decode worker appends to them while the
    // UI reads them, and a reallocation under a live reader is the one thing that
    // cannot be allowed (see chunk_store.hpp). Index them only in [0, readyInstructions()).
    const InstructionStore& getDecodedInstructions() const { return decodedInstructions; }
    const ChunkStore<uint64_t>& getInstructionAddresses() const { return instructionAddresses; }
    const Sections& getSections() const { return commonSections; }
    AddressSpace& getAddressSpace() { return contents; }

    size_t readyInstructions() const { return readyCount.load(std::memory_order_acquire); }

};

} // namespace voidwalk
