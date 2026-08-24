#pragma once
#include "../../../main/disassembler/disassembler.hpp"
#include "../../../main/disassembler/mnemonic/x86_64/x86_64-instr.hpp"
#include "../../../main/disassembler/mnemonic/instruction.hpp"
#include <memory>
#include <iostream>
#include <cassert>
#include <string>
#include "../../base.hpp"
#include "../../console.hpp"

class IA_32_Tests: public Tests{
private:



    static void testLineDecoding_OpcImm(){
       /* IA_32 instruction;
        instruction.decode(Instruction::Prefix( {0,0,0,0} ), 0xb9, 0, 0, 0, 0x6);
        auto res = instruction.decodeLineString();
        std::erase(res, ' ');
        assert(res == "MOVeCX,0x6"); */
    }

    void printOutput(){
        /*FILE* tmpOut;
#ifdef _WIN32
        fopen_s(&tmpOut, "tmpOut.txt", "w");
#else
        tmpOut = fopen("tmpOut.txt", "w");
#endif

        disassembler->decode();
        disassembler->decode();
        fclose(tmpOut);

        */
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
