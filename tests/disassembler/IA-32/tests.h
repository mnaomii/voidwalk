#include "../../../main/disassembler/disassembler.h"
#include "../../../main/disassembler/mnemonic/IA-32/IA-32-instr.h"
#include <memory>
#include <iostream>
#include <assert.h>
#include <string>
#include "../../base.h"
#include "../../console.h"

class IA_32_Tests: public Tests{
private:



    void testLineDecoding_OpcImm(){
        IA_32 instruction;
        instruction.decode(0,0xb9,0,0,0,0x6);
        auto res = instruction.decodeLineString();
        std::erase(res, ' ');
        assert(res == "MOVeCX,0x6");
    }

    void printOutput(){
        disassembler->decodeCS(stdout);
    }

    void runAll(){

        running("testLineDecoding_OpcImm");
        testLineDecoding_OpcImm();
        passed("testLineDecoding_OpcImm");

        std::cout << test_console::dim << "  |\n" << test_console::reset;
        printOutput();
    }



public:

    IA_32_Tests()  {
        runAll();
    }

};
