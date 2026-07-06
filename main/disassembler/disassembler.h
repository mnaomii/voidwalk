#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../address-space/address_space.h"
#include "miscellaneous/sections/base/header.h"


struct Sections {
    Header _text, _data, _ronly, _bss;
};

struct Registers { // emulating the current values of the registers.
    uint64_t eax, edx, ecx, ebx, esp, ebp, esi, edi, eip; // registers + eip - current instruction pointer;
    uint64_t ds, cs, fs, gs, ss, es; // segments
};


class Disassembler {
protected:

    Sections baseSections;
    uint64_t offset;
    uint16_t architecture;

    Registers registers;

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;


public:
    Disassembler(AddressSpace& temp) : contents(temp), architecture(0x00), offset(0x00) {};
    virtual std::string decodeCS(FILE* outputStream) = 0;
    virtual std::string getArchitecture()=0;
    virtual ~Disassembler() = default;

};

#endif