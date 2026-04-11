#include "hex_reader.h"
#include <stdexcept>


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