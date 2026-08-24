#include <iostream>
#include <memory>
#include "disassembler/x86_64-tests/IA-32-tests.hpp"
#include "disassembler/elf-sections-tests.hpp"
#include "disassembler/pe-sections-tests.hpp"
#include "../main/disassembler/disassembler.hpp"
#include "base.hpp"
#include "console.hpp"



void runTests() {

    test_console::init();   // UTF-8 + ANSI colors, for this module's output only
    using namespace test_console;

    std::cout << cyan << "  (*)  " << reset << "Setting initial parameters...\n";


    std::cout << cyan << "  (*)  " << reset << "Running IA-32-tests .. \n" << dim << "  |\n" << reset;
    IA_32_Tests();
    std::cout << "  |\n" << cyan << "  (*)  " << reset << "Running ELF-Sections-tests .. \n" << dim << "  |\n" << reset;
    ELF_Sections_Tests();
    std::cout << "  |\n" << cyan << "  (*)  " << reset << "Running PE-Sections-tests .. \n" << dim << "  |\n" << reset;
    PE_Sections_Tests();


    std::cout << dim << "  |\n  └─- " << reset
              << green << "All tests passed.." << reset << "\n\n";


    std::cout << "Press [Enter] to exit.. " << std::endl;
    std::cin.get();
}
