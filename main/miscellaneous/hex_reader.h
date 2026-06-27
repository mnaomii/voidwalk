// basic hexreader

#ifndef HEX_READER_H
#define HEX_READER_H

#include <fstream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

inline void output(vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0)
            cout << std::hex << setfill('0') << setw(8) << i << "  ";
        cout << std::hex << setfill('0') << setw(2) << static_cast<int>(data[i]) << ' ';
        if ((i + 1) % 16 == 0) cout << '\n';
    }
    cout << '\n';
}



inline vector<uint8_t> read_file(const char filename[]){
    ifstream fin(filename, ios::binary | ios::ate);
    if(!fin.is_open()) throw runtime_error("File cannot be opened.");

    size_t size = fin.tellg();
    vector<uint8_t> contents(size);
    fin.seekg(0,ios::beg);

    if(!fin.read(reinterpret_cast<char*>(contents.data()),size)) throw runtime_error("Could not read file contents.");
    
    return contents;
}

#endif 