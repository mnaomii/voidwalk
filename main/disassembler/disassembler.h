#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../address_space/address_space.h"

using namespace std;

struct header {
    uint64_t vaddr;
    uint64_t offset;
    uint64_t size;
};

class Disassembler {
protected:

    bool is32bit;
    uint16_t architecture;

    AddressSpace& contents;

    header _text;
    header _data;
    header _ronly; //strings, constants
    header _bss; // empty section, memory will be filled at runtime

    virtual void setHeadersOffsets()=0;

    void initHeader(header& x);

public:
    Disassembler(AddressSpace& temp);

    virtual std::string getArchitecture()=0;
    virtual ~Disassembler() = default;

};

#endif