#pragma once
// basic hex reader


#include "address_space.hpp"

#include <cstdio>
#include <iostream>

namespace cli {

// The analysis core lives in namespace voidwalk.
using voidwalk::AddressSpace;


// Writes `filename` to stdout as a hex dump: an 8-digit file offset, 16
// space-separated bytes and their ASCII per line, the last line padded to align.
//
// Throws std::length_error if the file cannot be mapped (from AddressSpace).
// Streams straight from the mapping, one formatted line at a time, so a large
// file prints progressively rather than all at once.
inline void outputHex(const char* filename) {
    AddressSpace file{std::string(filename)};
    static constexpr char kHex[] = "0123456789abcdef";

    const size_t size = file.size();
    for (size_t off = 0; off < size; off += 16) {
        char line[96];
        int n = std::snprintf(line, sizeof(line), "%08zx  ", off);
        char ascii[17]{};
        for (size_t i = 0; i < 16; ++i) {
            if (off + i < size) {
                const uint8_t b = file.read_u8(off + i);
                line[n++] = kHex[b >> 4];
                line[n++] = kHex[b & 15];
                ascii[i] = (b >= 0x20 && b < 0x7f) ? static_cast<char>(b) : '.';
            } else {
                line[n++] = ' ';
                line[n++] = ' ';
                ascii[i] = ' ';
            }
            line[n++] = ' ';
        }
        n += std::snprintf(line + n, sizeof(line) - n, " |%s|\n", ascii);
        std::cout.write(line, n);
    }
}

} // namespace cli
