#include "address-space/address_space.h"
#include "disassembler/disassembler.h"
#include "disassembler/ELF/elf_disassembler.h"
#include "disassembler/PE/pe_disassembler.h"

//#include "GUI/runner/mainGUI.cpp"
#include "UI/runner/mainUI.h"

#include <memory>
#include <iostream>
#include <string>

void determine_filetype(AddressSpace& contents, bool& is_elf, bool& is_pe) {

    if (contents.size() >= 4 && contents.read_u8(0) == 0x7F && contents.read_u8(1) == 0x45 &&
        contents.read_u8(2) == 0x4C && contents.read_u8(3) == 0x46) is_elf = true;
    else if (contents.size() >= 2 && contents.read_u8(0) == 0x4D && contents.read_u8(1) == 0x5A) { // ms-dos compat line
        uint32_t pe_header_offset = contents.read_u32(0x3C); // PE header offset pointer
        if (pe_header_offset <= contents.size() - 4 &&
            contents.read_u32(pe_header_offset) == 0x00004550) is_pe = true;
    }
}

std::string make_disassembler(AddressSpace& data, std::unique_ptr<Disassembler> d) {
    bool is_elf = false, is_pe = false;
    determine_filetype(data, is_elf, is_pe);

    if (is_elf) {
        
        d = std::make_unique<ELF_Disassembler>(data);
        return  "\nELF Binary detected..\n";
    }
    if (is_pe) {
        
        d = std::make_unique<PE_Disassembler>(data);
        return  "\nPE Binary detected..\n";
    }
    throw std::runtime_error("Not an ELF or PE binary.");
}

int main(int argc, char** argv) {
    std::unique_ptr<Disassembler> disassembler;
    if (argc <= 1) std::cout << "No interface selected.\n";
    else {
        if (std::string(argv[1]).find("-gui"))
            //GUIstart(argc, argv);
            return 0;
        else
            UIstart(argc, argv);
    }
	return 0;
}