#include "address-space/address_space.h"
#include "disassembler/disassembler.h"
#include "miscellaneous/loader.h"

#include "../tests/runner.h"

//#include "GUI/runner/mainGUI.cpp"
#ifdef VOIDWALK_WITH_TUI // defined by the voidwalk-tui project; the plain voidwalk project builds without FTXUI
#include "TUI/runner/mainUI.h"
#endif

#include <memory>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "Usage: voidwalk [--ui | --gui | --run-tests] <binary>\n";
        return 0;
    }

    std::shared_ptr<AddressSpace> data;
    try {
        data = std::make_shared<AddressSpace>(argv[argc - 1]);
    }
    catch (std::runtime_error&) {
        std::cerr << "File could not be opened.\n";
        return 1;
    }

    std::shared_ptr<Disassembler> disassembler;
    std::string status = make_disassembler(*data, &disassembler);

    std::string mode = argv[1];
    if (mode == "--gui") {
        //GUIstart(argc, argv); // GUI lives in the voidwalk-gui (Qt) project - placeholder here
        std::cout << "GUI not wired into this executable yet.\n";
        return 0;
    }
    if (mode == "--run-tests") {
        runTests(argc, argv, disassembler);
        return 0;
    }

    // "--ui" or a bare <binary> argument: the TUI is the default interface
#ifdef VOIDWALK_WITH_TUI
    return UIstart(argc, argv, status, data, disassembler);
#else
    std::cout << "Analyzing file " << argv[argc - 1] << status
              << " Architecture -> " << disassembler->getArchitecture() << "\n\n"
              << "(TUI not built in this configuration - use the voidwalk-tui project.)\n";
    return 0;
#endif
}
