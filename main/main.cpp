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
#include <exception>
#include <stdexcept>

static void printHelp(const char* exe) {
    std::cout <<
        "voidwalk - binary analysis tool for ELF and PE executables.\n"
        "\n"
        "Usage:\n"
        "  " << exe << " [--gui]                       open the GUI (default when no\n"
        "                                       arguments are given)\n"
        "  " << exe << " --tui <binary>                open the terminal UI on <binary>\n"
        "  " << exe << " --print <binary> [out...]     disassemble to stdout, and to each\n"
        "                                       additional file given\n"
        "  " << exe << " --dump-hex <binary>           hex dump of <binary>\n"
        "  " << exe << " --help                        this message\n"
        "\n"
        "The binary format (ELF or PE) and its architecture are detected from the\n"
        "file's magic bytes - there is no flag to select them.\n";
}

// Everything main() used to do, minus the exception handling. Kept as its own
// function so main() can be a thin top-level handler rather than one giant try
// block: a throw from any path below lands in exactly one place.
static int run(int argc, char** argv) {

    std::string mode = (argc > 1) ? argv[1] : "--gui";

    // argv[0] is whatever path the shell used to invoke us; the usage lines are
    // unreadable with an absolute one, so show just the program name.
    const char* exe = "voidwalk";
    if (argc > 0 && argv[0] && *argv[0]) {
        exe = argv[0];
        for (const char* c = argv[0]; *c; ++c)
            if (*c == '/' || *c == '\\') exe = c + 1;
        if (!*exe) exe = "voidwalk";
    }
    if (mode == "--help" || mode == "-h") {
        printHelp(exe);
        return 0;
    }

    if (mode != "--gui" && mode != "--tui")
    {
        // console() throws for a missing/unopenable file, and printToConsole()
        // throws for an output stream it cannot create. Both used to escape
        // main() and abort the process; report them and exit non-zero instead.
        try {
            console(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "voidwalk: " << e.what()
                      << "Try '" << exe << " --help' for usage.\n";
            return 1;
        }
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
    catch (const std::exception& e) {
        std::cerr << "voidwalk: cannot open " << argv[argc - 1] << " - " << e.what() << "\n";
        return 1;
    }

    std::string status = "";
    std::shared_ptr<Disassembler> disassembler;
    try {
         status = make_disassembler(*data, &disassembler);
    }
    catch (const std::exception& e) {
        // Was a fixed "Corrupt file." with a zero exit status, which reported
        // an unsupported architecture and a truncated header identically - and
        // told the shell the run had succeeded.
        std::cerr << "voidwalk: cannot parse " << argv[argc - 1] << " - " << e.what() << "\n";
        return 1;
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
#endif

    return 0;
}

int main(int argc, char** argv) {
    // Last line of defence. An exception that escapes main() is not an error
    // message - it is std::terminate and a SIGABRT, with no indication of what
    // went wrong. The decoder and both UI paths can still throw from places
    // run() does not wrap individually, so catch here rather than abort.
    try {
        return run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "voidwalk: unhandled error - " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "voidwalk: unhandled error of unknown type\n";
        return 1;
    }
}
