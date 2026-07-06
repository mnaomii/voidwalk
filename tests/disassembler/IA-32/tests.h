#include "../../../main/disassembler/disassembler.h"
#include <memory>
#include <iostream>
#include "../../base.h"

class IA_32_Tests: public Tests{
    public:

    
    bool printOutput(){
        disassembler->decodeCS(stdout);
        return true;
    }

    IA_32_Tests() {
        fc = printOutput();
    }

    bool finishedCorrectly(){
        return fc;
    }

};