#include "hex_reader.h"
#include <stdexcept>

 void determine_filetype(vector<uint8_t>& contents, bool& is_elf, bool& is_pe) {

    if (contents.size() >= 4 && contents[0] == 0x7F && contents[1] == 0x45 &&
        contents[2] == 0x4C && contents[3] == 0x46) is_elf = true;
    else if (contents.size() >= 2 && contents[0] == 0x4D && contents[1] == 0x5A) {
        uint32_t pe_header_offset = *reinterpret_cast<uint32_t*>(&contents[0x3C]); // PE header offset pointer
        if (pe_header_offset + 4 <= contents.size() && contents[pe_header_offset] == 0x50 && contents[pe_header_offset + 1] == 0x45 &&
            contents[pe_header_offset + 2] == 0x00 && contents[pe_header_offset + 3] == 0x00) is_pe = true;
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
    vector<uint8_t> data = read_file(argv[1]);

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