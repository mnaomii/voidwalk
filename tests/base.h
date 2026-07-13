#pragma once
#include <memory>
#include <iostream>
#include "console.h"
#include "../main/disassembler/disassembler.h"



class Tests{
    public:

        inline void running(const char* name){
            std::cout << test_console::dim << "  (~)  Running " << name << "() ..\n" << test_console::reset;
        }
        inline void passed(const char* name){
            std::cout << test_console::green << "  (✓)  Passed " << test_console::reset
                      << name << "()" <<  " !\n" << test_console::reset;
        }

    inline static int argc;
    inline static char** argv;
    inline static std::shared_ptr<Disassembler> disassembler;

    Tests(){};
};
