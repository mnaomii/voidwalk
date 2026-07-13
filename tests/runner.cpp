#include <iostream>
#include <memory>
#include "disassembler/IA-32/tests.h"
#include "disassembler/elf-sections.h"
#include "disassembler/pe-sections.h"
#include "../main/disassembler/disassembler.h"
#include "base.h"
#include "console.h"



void runTests(int ac, char** av, std::shared_ptr<Disassembler> disasm) {

    test_console::init();   // UTF-8 + ANSI colors, for this module's output only
    using namespace test_console;

    std::cout << cyan << "  (*)  " << reset << "Setting initial parameters...\n";
    Tests::argc = ac;
    Tests::argv = av;
    Tests::disassembler = disasm;

    std::cout << cyan << "  (*)  " << reset << "Running IA-32-tests .. \n" << dim << "  |\n" << reset;
    IA_32_Tests();
    std::cout << "  |\n" << cyan << "  (*)  " << reset << "Running ELF-Sections-tests .. \n" << dim << "  |\n" << reset;
    ELF_Sections_Tests();
    std::cout << "  |\n" << cyan << "  (*)  " << reset << "Running PE-Sections-tests .. \n" << dim << "  |\n" << reset;
    PE_Sections_Tests();


    std::cout << dim << "  |\n  └─- " << reset
              << green << "All tests passed.." << reset << "\n\n";
}
