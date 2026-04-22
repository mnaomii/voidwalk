#include "hex_reader.h"
#include "address_space/address_space.h"
#include "disassembler/disassembler.h"
#include "disassembler/ELF/elf-disassembler.h"
#include "disassembler/PE/pe-disassembler.h"
#include <stdexcept>
#include <string>

 void determine_filetype(AddressSpace& contents, bool& is_elf, bool& is_pe) {

    if (contents.size() >= 4 && contents.read_u8(0) == 0x7F && contents.read_u8(1) == 0x45 &&
        contents.read_u8(2) == 0x4C && contents.read_u8(3) == 0x46) is_elf = true;
    else if (contents.size() >= 2 && contents.read_u8(0) == 0x4D && contents.read_u8(1) == 0x5A) { // ms-dos compat line
        uint32_t pe_header_offset = contents.read_u32(0x3C); // PE header offset pointer
        if (pe_header_offset <= contents.size() -4 &&
            contents.read_u32(pe_header_offset) == 0x00004550) is_pe = true;
    }
}

 std::unique_ptr<Disassembler> make_disassembler(AddressSpace& data) {
     bool is_elf = false, is_pe = false;
     determine_filetype(data, is_elf, is_pe);

     if (is_elf) {
         std::cout << "\nELF Binary detected..\n";
         return std::make_unique<ELF_Disassembler>(data);
     }
     if (is_pe) {
         std::cout << "\nPE Binary detected..\n";
         return std::make_unique<PE_Disassembler>(data);
     }
     throw std::runtime_error("Not an ELF or PE binary.");
 }


 int main(int argc, char** argv) {
     std::string filename;

     if (argc > 2) {
         std::cout << "Too many arguments provided.\n";
         return 1;
     }

     if (argc == 2) {
         filename = argv[1];
     }
     else {
         std::cout << "At least one argument is required.\n"
             << "Give a path to an executable: >> ";
         while (true) {
             std::getline(std::cin,filename);
             std::ifstream f(filename);
             if (f.is_open()) break;
             std::cerr << "Invalid path/executable. Try again.\n"
                 << "Executable path: >> ";
         }
     }

     try {
         AddressSpace data(filename);
         auto d = make_disassembler(data);
         std::cout << "Detected architecture : " << d->getArchitecture() << "\n";
         // d->disassemble();
     }
     catch (std::runtime_error& e) {
         std::cout << e.what() << "\n";
         return 1;
     }
     return 0;
 }