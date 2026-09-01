// basic hex reader

#ifndef HEX_READER_H
#define HEX_READER_H

#include "../main/address-space/address_space.hpp"

#include <iomanip>
#include <stdexcept>
#include <iostream>

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



#endif 