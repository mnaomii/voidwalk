#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <vector>
#include <string>
#include <memory>
#include "../address-space/address_space.hpp"
#include "miscellaneous/sections/base/header.hpp"
#include "mnemonic/instruction.hpp"


struct Sections {
    Header _text, _data, _ronly, _bss;
};

struct Registers_x86_64 { // emulating the current values of the registers.
    uint64_t rax, rdx, rcx, rbx, rsp, rbp, rsi, rdi, rip; // registers + eip - current instruction pointer;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t ds, cs, fs, gs, ss, es; // segments

    uint8_t flags;
};


class Disassembler {

private:
    size_t instrDecodePos{};
    std::vector<std::ostream*> outputStreams;



protected:

    uint64_t imageBase{};

    std::vector<std::unique_ptr<Instruction>> decodedInstructions;
    // Virtual address each decodedInstructions[i] starts at, recorded by decode().
    // Parallel to decodedInstructions; Instruction itself carries no address.
    std::vector<uint64_t> instructionAddresses;
    std::vector<uint64_t> virtStack;
    Sections baseSections;
    uint64_t offset;
    uint16_t architecture;

    Registers_x86_64 registers{};

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;

    uint64_t decodeLine_x86_64(uint64_t address, uint64_t vaddr, bool is64Bit);

public:
    Disassembler(AddressSpace& temp, const std::vector<std::ostream*>& stream) : contents(temp), architecture(0x00), offset(0x00), outputStreams(stream) {

        // emulated for the moment
        registers.rip = baseSections._text.getOffset();
        registers.cs = registers.rip;


    };
    void emitDecodedLine();
    virtual std::string getArchitecture()=0;
    virtual uint64_t decodeLine(uint64_t address, uint64_t vaddr)=0;
    virtual ~Disassembler() = default;

    void decode();

    // read-only views for the UI layers (TUI/GUI); they must not mutate core state
    const Registers_x86_64& getRegisters() const { return registers; }
    const std::vector<uint64_t>& getVirtStack() const { return virtStack; }
    const std::vector<std::unique_ptr<Instruction>>& getDecodedInstructions() const { return decodedInstructions; }
    const std::vector<uint64_t>& getInstructionAddresses() const { return instructionAddresses; }
    const Sections& getSections() const { return baseSections; }
    AddressSpace& getAddressSpace() { return contents; }

};

#endif