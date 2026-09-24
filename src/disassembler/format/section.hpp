#pragma once
#include <cstdint>

namespace voidwalk {

// One section of a loaded binary, reduced to the three numbers every container
// format agrees on. Both the ELF and PE readers translate their own header layout
// into this, so nothing above the format tier parses a section header.
//
// The two addresses are NOT interchangeable and mixing them is the most common bug
// at this seam:
//   offset  where the bytes are in the file        - what AddressSpace reads at
//   vaddr   where the loader would place them      - what the disassembly shows
// They differ by the image base plus per-section alignment padding, and coincide
// only by accident.
//
// A default-constructed Header (all zero) means "the parser did not find this
// section". Callers test offset == 0 && size == 0 rather than carrying a flag.
class Header {
private:
        uint64_t vaddr; // address during runtime
        uint64_t offset; // address on disk
        uint64_t size; // total size of section (bytes)

public:

    // An absent section: all three fields zero.
    Header() : vaddr(0), offset(0), size(0) {};

    // v = runtime virtual address, o = file offset, s = size in bytes.
    Header(uint64_t v, uint64_t o, uint64_t s) : vaddr(v), offset(o), size(s) {};

    void setVaddr(uint64_t value) { vaddr = value; }
    void setOffset(uint64_t value) { offset = value; }
    void setSize(uint64_t value) { size = value; }

    // Runtime address of the section's first byte.
    uint64_t getVaddr() const { return vaddr; }

    // File offset of the section's first byte - the value to hand AddressSpace.
    uint64_t getOffset() const { return offset; }

    // Section length in bytes, as the container header claims it. NOT validated
    // against the file size: a malformed binary can report a size far larger than
    // the file, so a caller sizing an allocation from this must clamp it first.
    uint64_t getSize() const { return size; }

    
};

} // namespace voidwalk
