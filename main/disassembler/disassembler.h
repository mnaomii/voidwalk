#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <vector>
#include <string>
#include <memory>
#include "../address-space/address_space.h"
#include "miscellaneous/sections/base/header.h"
#include "mnemonic/instruction.h"


struct Sections {
    Header _text, _data, _ronly, _bss;
};

struct Registers { // emulating the current values of the registers.
    uint64_t eax, edx, ecx, ebx, esp, ebp, esi, edi, eip; // registers + eip - current instruction pointer;
    uint64_t ds, cs, fs, gs, ss, es; // segments

    uint8_t flags;
};


class Disassembler {
protected:



    std::vector<std::unique_ptr<Instruction>> decodedInstructions;
    // Virtual address each decodedInstructions[i] starts at, recorded by decode().
    // Parallel to decodedInstructions; Instruction itself carries no address.
    std::vector<uint64_t> instructionAddresses;
    std::vector<uint64_t> virtStack;
    Sections baseSections;
    uint64_t offset;
    uint16_t architecture;

    Registers registers;

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;

    uint64_t decodeLine_IA_32(uint64_t address, uint64_t vaddr);

public:
    Disassembler(AddressSpace& temp) : contents(temp), architecture(0x00), offset(0x00) {

        // emulated for the moment
        registers.eax = 0;
        registers.ecx = 0;
        registers.edx = 0;
        registers.ebp = 0; // offset into fake stack
        registers.esp = 0; // offset into fake stack
        registers.esi = 0;
        registers.edi = 0;
        registers.ebx = 0;
        registers.flags = 0;
        registers.eip = baseSections._text.getOffset();
        registers.cs = registers.eip;

    };
    void decodeCS(FILE* outputStream);
    virtual std::string getArchitecture()=0;
    virtual uint64_t decodeLine(uint64_t address, uint64_t vaddr)=0;
    virtual ~Disassembler() = default;

    void decode();

    // read-only views for the UI layers (TUI/GUI); they must not mutate core state
    const Registers& getRegisters() const { return registers; }
    const std::vector<uint64_t>& getVirtStack() const { return virtStack; }
    const std::vector<std::unique_ptr<Instruction>>& getDecodedInstructions() const { return decodedInstructions; }
    const std::vector<uint64_t>& getInstructionAddresses() const { return instructionAddresses; }
    const Sections& getSections() const { return baseSections; }
    AddressSpace& getAddressSpace() { return contents; }

};

#endif