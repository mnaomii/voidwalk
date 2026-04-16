#include "hex_reader.h"
#include "address_space/address_space.h"
#include <stdexcept>
#include <string>

 void determine_filetype(AddressSpace& contents, bool& is_elf, bool& is_pe) {

    if (contents.size() >= 4 && contents.read_u8(0) == 0x7F && contents.read_u8(1) == 0x45 &&
        contents.read_u8(2) == 0x4C && contents.read_u8(3) == 0x46) is_elf = true;
    else if (contents.size() >= 2 && contents.read_u8(0) == 0x4D && contents.read_u8(1) == 0x5A) {
        uint32_t pe_header_offset = contents.read_u32(0x3C); // PE header offset pointer
        if (pe_header_offset + 4 <= static_cast<uint32_t>(contents.size()) && contents.read_u32(pe_header_offset) == 0x50 && contents.read_u32(pe_header_offset + 1) == 0x45 &&
            contents.read_u32(pe_header_offset + 2) == 0x00 && contents.read_u32(pe_header_offset + 3) == 0x00) is_pe = true;
    }
}


int main(int argc, char** argv){

    if(argc != 2 ){ 
        bool condition = (argc == 1) ? true : false;
        switch(condition){
            case true:
                cout<<"At least one argument is required. "; break;
            default:
                cout<<"Too many arguments provided."; break;
        }
        return 1;
    }

    try{
    bool is_elf = false, is_pe = false;
    AddressSpace data(argv[1]);

    determine_filetype(data,is_elf,is_pe);
    if(!(is_elf || is_pe )) throw runtime_error("File is not a compatible executable. [ELF/PE] ");
    else{
        if(is_elf) cout<<"Provided file is an ELF-binary.\n";
        else if(is_pe) cout<<"Provided file is a PE-binary.\n";
        else{throw runtime_error("File is not an executable.\n");}
    }
   // output(data);

    } catch(runtime_error& e) {cout<<e.what()<<"\n"; return 1;}

    return 0;
}