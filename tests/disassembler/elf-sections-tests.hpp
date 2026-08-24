#include "../base.hpp"
#include "../console.hpp"


class ELF_Sections_Tests : public Tests {


private:


    void testOffsetsELF() {
    }




    void runAll() {

        running("testOffsetsELF");
        testOffsetsELF();
        passed("testOffsetsELF");


        std::cout << test_console::dim << "  |\n" << test_console::reset;
    }


public:

    ELF_Sections_Tests() {
        runAll();
    }

};
