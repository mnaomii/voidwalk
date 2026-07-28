#pragma once
#include "../../../main/disassembler/disassembler.h"
#include "../../../main/disassembler/mnemonic/IA-32/IA-32-instr.h"
#include "../../../main/disassembler/mnemonic/instruction.h"
#include <memory>
#include <iostream>
#include <cassert>
#include <string>
#include "../../base.h"
#include "../../console.h"

class IA_32_Tests: public Tests{
private:



    static void testLineDecoding_OpcImm(){
        IA_32 instruction;
        instruction.decode(Instruction::Prefix( {0,0,0,0} ), 0xb9, 0, 0, 0, 0x6);
        auto res = instruction.decodeLineString();
        std::erase(res, ' ');
        assert(res == "MOVeCX,0x6");
    }

    void printOutput(){
        FILE* tmpOut;
#ifdef _WIN32
        fopen_s(&tmpOut, "tmpOut.txt", "w");
#else
        tmpOut = fopen("tmpOut.txt", "w");
#endif

        disassembler->decodeCS(stdout);
        disassembler->decodeCS(tmpOut);
        fclose(tmpOut);
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
