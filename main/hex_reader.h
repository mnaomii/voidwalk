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

inline void determine_filetype(vector<uint8_t>& contents, bool &is_elf, bool &is_pe){

    if(contents.size() >= 4 && contents[0] == 0x7F && contents[1] == 0x45 && 
        contents[2] == 0x4C && contents[3] == 0x46) is_elf=true;
    else if(contents.size() >=2 && contents[0] == 0x4D && contents[1] == 0x5A) {
        uint32_t pe_header_offset = *reinterpret_cast<uint32_t*>(&contents[0x3C]); // PE header offset pointer
        if( pe_header_offset + 4 <= contents.size() &&  contents[pe_header_offset] == 0x50 && contents[pe_header_offset + 1] == 0x45 && 
            contents[pe_header_offset + 2] == 0x00 && contents[pe_header_offset + 3] == 0x00) is_pe=true;
    }
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