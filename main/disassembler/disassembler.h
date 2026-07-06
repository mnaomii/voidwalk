#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <vector>
#include <string>
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



    std::vector<Instruction> decodedInstructions;
    std::vector<uint64_t> virtStack;
    Sections baseSections;
    uint64_t offset;
    uint16_t architecture;

    Registers registers;

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;


public:
    Disassembler(AddressSpace& temp) : contents(temp), architecture(0x00), offset(0x00), decodedInstructions({}) {

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
    virtual std::string decodeCS(FILE* outputStream) = 0;
    virtual std::string getArchitecture()=0;
    virtual size_t decodeLine(uint64_t address) = 0;
    virtual ~Disassembler() = default;

};

#endif