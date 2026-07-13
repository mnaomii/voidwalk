#include "../base.h"
#include "../console.h"


class PE_Sections_Tests : public Tests {


private:
    void testOffsetsPE() {
    }



    void runAll() {

        running("testOffsetsPE");
        testOffsetsPE();
        passed("testOffsetsPE");

        std::cout << test_console::dim << "  |\n" << test_console::reset;
    }


public:

    PE_Sections_Tests() {
        runAll();
    }

};
