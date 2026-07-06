#pragma once
#include <memory>
#include "../main/disassembler/disassembler.h"



class Tests{
    public:

    bool fc;
    inline static int argc;
    inline static char** argv;
    inline static std::shared_ptr<Disassembler> disassembler;

    Tests(): fc(true){};
};
