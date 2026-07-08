#include <iostream>
#include <memory>
#include "disassembler/IA-32/tests.h"
#include "../main/disassembler/disassembler.h"
#include "base.h"



void runTests(int ac, char** av, std::shared_ptr<Disassembler> disasm) {

    std::cout<< "  (*)  Setting initial parameters...\n";
    Tests::argc = ac;
    Tests::argv = av;
    Tests::disassembler = disasm;

    std::cout<< "  (*)  Running IA-32-tests .. \n  |\n";
    if (IA_32_Tests().finishedCorrectly())
        std::cout<< "  -- All tests passed..\n\n";
    else
        std::cout << "  -- IA-32 tests failed.\n\n";




    std::cout<<"Finished all tests!";

}