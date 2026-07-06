#include <cstdint>
#pragma once

class Header {
private:
        uint64_t vaddr; // address during runtime
        uint64_t offset; // address on disk
        uint64_t size; // total size of section (bytes)

public:

    Header() : vaddr(0), offset(0), size(0) {};
    Header(uint64_t v, uint64_t o, uint64_t s) : vaddr(v), offset(o), size(s) {};

    void setVaddr(uint64_t value) { vaddr = value; }
    void setOffset(uint64_t value) { offset = value; }
    void setSize(uint64_t value) { size = value; }

    uint64_t getVaddr() { return vaddr; }
    uint64_t getOffset() { return offset; }
    uint64_t getSize() { return size; }

    
};