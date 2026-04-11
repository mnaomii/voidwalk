#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
#include <string>
using namespace std;

class Disassembler {
protected:
    bool is32bit;
    string filetype;
    vector<uint8_t> contents;
    uint64_t* _text;
    uint64_t* _data;
    uint64_t* _ronly; //strings, constants
    uint64_t* _bss;

    virtual void setHeadersOffsets()=0;

public:
    Disassembler(vector<uint8_t> temp) : contents(temp) {};
    virtual ~Disassembler() = default;

};

#endif