// basic hexreader

#ifndef HEX_READER_H
#define HEX_READER_H

#include <fstream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>


inline void outputHex(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0)
            std::cout << std::hex << std::setfill('0') << std:: setw(8) << i << "  ";
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]) << ' ';
        if ((i + 1) % 16 == 0) std::cout << '\n';
    }
    std::cout << '\n';
}



inline std::vector<uint8_t> readFile(char* filename){
    std::ifstream fin(filename, std::ios::binary | std::ios::ate);
    if(!fin.is_open()) throw std::runtime_error("File cannot be opened.");

    size_t size = fin.tellg();
    std::vector<uint8_t> contents(size);
    fin.seekg(0,std::ios::beg);

    if(!fin.read(reinterpret_cast<char*>(contents.data()),size)) throw std::runtime_error("Could not read file contents.");

    return contents;
}

#endif 