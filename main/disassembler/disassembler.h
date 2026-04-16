#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../../address_space/address_space.h"

using namespace std;

struct header {
    uint64_t* offset;
    size_t size;
};

class Disassembler {
protected:
    bool is32bit;
    uint16_t architecture;
    uint16_t filetype; // 1 - object file , 2 - executable, 3 - sharedlib (ELF)
                       // ..  (PE)
    AddressSpace& contents;

    header _text;
    header _data;
    header _ronly; //strings, constants
    header _bss; // empty section, memory will be filled at runtime

    virtual void setHeadersOffsets()=0;

public:
    Disassembler(AddressSpace& temp) : filetype(0), contents(temp) {}
    virtual ~Disassembler() = default;

};

#endif