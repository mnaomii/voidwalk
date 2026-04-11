#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <vector>
using namespace std;

class Disassembler {
protected:

    vector<uint8_t> contents;
    uint32_t* _text;
    uint32_t* _data;
    uint32_t* _ronly; //strings, constants
    uint32_t* _bss;

    virtual void getDataHeader() = 0;
    virtual void getTextHeader() = 0;
    virtual void getROnlyHeader() = 0;
    virtual void getBSSHeader() = 0;

public:
    Disassembler(vector<uint8_t>& temp) : contents(temp) {};
    virtual ~Disassembler() = default;

};

#endif