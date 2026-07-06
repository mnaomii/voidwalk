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


class Disassembler {
protected:

    Sections baseSections;
    uint64_t offset;
    uint16_t architecture;

    AddressSpace& contents;

    virtual void setHeadersOffsets()=0;


public:
    Disassembler(AddressSpace& temp) : contents(temp), architecture(0x00), offset(0x00) {};
    virtual std::string decodeLine() = 0;
    virtual std::string getArchitecture()=0;
    virtual ~Disassembler() = default;

};

#endif