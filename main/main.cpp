  #include "address-space/address_space.hpp"
#include "disassembler/disassembler.hpp"
#include "miscellaneous/loader.hpp"


//#include "../tests/runner.h"

#ifdef VOIDWALK_WITH_GUI // defined when the Qt6 GUI is compiled into this binary
#include "GUI/runner/gui_main.h"
#endif

#ifdef VOIDWALK_WITH_TUI // defined when the FTXUI TUI is compiled into this binary
#include "TUI/runner/mainUI.h"
#endif


// console utils

#include "../console-utils/delegate.hpp"

#include <memory>
#include <iostream>
#include <string>

int main(int argc, char** argv) {

    std::string mode = (argc > 1) ? argv[1] : "--gui";

    if (mode != "--gui" && mode != "--tui")
    {
        console(argc, argv);
        return 0;
    }


    // The GUI needs no pre-opened file - it opens one itself via its file
    // dialog, so branch here before AddressSpace touches argv[argc - 1].
    if (mode == "--gui") {
#ifdef VOIDWALK_WITH_GUI
        return GUIstart(argc, argv);
#else
        std::cout << "GUI not built in this configuration (enable VOIDWALK_BUILD_GUI).\n";
        return 0;
#endif
    }

    // The TUI and the test harness both need the target file opened up front.
   
    std::shared_ptr<AddressSpace> data;
    try {
        data = std::make_shared<AddressSpace>(argv[argc - 1]);
    }
    catch (std::exception&) {
        std::cerr << "File could not be opened.\n";
        return 1;
    }

    std::string status = "";
    std::shared_ptr<Disassembler> disassembler;
    try {
         status = make_disassembler(*data, &disassembler);
    }
    catch (std::exception&) {
        std::cerr << "Corrupt file.";
        return 0;
    }
//    if (mode == "--run-tests") {      NEEDS OVERHAUL  
//        runTests(argc, argv, disassembler);
 //       return 0;
//    }

    // "--ui" or a bare <binary> argument: the TUI is the default interface
#ifdef VOIDWALK_WITH_TUI
    if( mode == "--tui")
    return UIstart(argc, argv, status, data, disassembler);
#else
    std::cout << "Analyzing file " << argv[argc - 1] << status
              << " Architecture -> " << disassembler->getArchitecture() << "\n\n"
              << "(TUI not built in this configuration - use the voidwalk-tui project.)\n";
    return 0;
#endif



}
