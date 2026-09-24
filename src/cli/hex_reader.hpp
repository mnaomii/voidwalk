#pragma once
// basic hex reader


#include "address_space.hpp"

#include <iomanip>
#include <stdexcept>
#include <iostream>

namespace cli {

// The analysis core lives in namespace voidwalk.
using voidwalk::AddressSpace;


// Writes `filename` to stdout as a hex dump: an 8-digit file offset, then up to
// 16 space-separated bytes per line.
//
// Throws std::length_error if the file cannot be mapped (from AddressSpace) or is
// empty. Streams straight from the mapping - no buffering, so a large file prints
// progressively rather than all at once.
inline void outputHex(char* filename) {
    AddressSpace file{std::string(filename)};

    size_t size = file.size();
    if (size == 0) throw std::length_error("File is empty.\n");

    for (size_t i = 0; i < file.size(); ++i) {
        if (i % 16 == 0)
            std::cout << std::hex << std::setfill('0') << std:: setw(8) << i << "  ";
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(file.read_u8(i)) << ' ';
        if ((i + 1) % 16 == 0) std::cout << '\n';
    }
    std::cout << '\n';
}




} // namespace cli
