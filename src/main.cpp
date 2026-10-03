#include "address_space.hpp"
#include "disassembler/disassembler.hpp"
#include "disassembler/format/detect.hpp"


//#include "../tests/runner.h"

#ifdef VOIDWALK_WITH_GUI // defined when the Qt6 GUI is compiled into this binary
#include "gui/gui_main.hpp"
#endif

#ifdef VOIDWALK_WITH_TUI // defined when the FTXUI TUI is compiled into this binary
#include "tui/tui_main.hpp"
#endif


// console utils

#include "cli/console.hpp"

#include <memory>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

// The analysis core lives in namespace voidwalk; main() is the one place that
// stitches core and frontends together, so it names what it needs explicitly.
using voidwalk::AddressSpace;
using voidwalk::Disassembler;
using voidwalk::make_disassembler;

// An exception's message as one clean line: the core's messages carry their own
// "[voidwalk] : " prefix and a trailing newline, which would double up with ours.
static std::string message(const std::exception& e) {
    std::string m = e.what();
    if (m.rfind("[voidwalk] : ", 0) == 0) m.erase(0, 13);
    while (!m.empty() && (m.back() == '\n' || m.back() == ' ')) m.pop_back();
    return m;
}

static void printHelp(const char* exe) {
    std::cout <<
        "voidwalk - binary analysis tool for ELF and PE executables.\n"
        "\n"
        "Usage:\n"
        "  " << exe << " [--gui]                       open the GUI (default when no\n"
        "                                       arguments are given)\n"
        "  " << exe << " [--tui] <binary>              open the terminal UI on <binary>\n"
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

    // A usage error says how to get help; any other failure just says what failed.
    // Both exit 1.
    try {
        if (mode == "--print" || mode == "--dump-hex") {
            cli::start(argc, argv);
            return 0;
        }
        // "--tui <binary>", or a bare "<binary>": exactly one path either way.
        if (mode != "--gui") {
            const bool bare = mode.empty() || mode[0] != '-';
            if (!bare && mode != "--tui")
                throw std::invalid_argument("unknown option '" + mode + "'");
            if (argc != (bare ? 2 : 3))
                throw std::invalid_argument(bare ? "expected a single <binary>"
                                                 : "--tui takes exactly one <binary>");
        }
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "voidwalk: " << message(e) << "\nTry '" << exe << " --help' for usage.\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "voidwalk: " << message(e) << "\n";
        return 1;
    }


    // The GUI needs no pre-opened file - it opens one itself via its file
    // dialog, so branch here before AddressSpace touches argv[argc - 1].
    if (mode == "--gui") {
#ifdef VOIDWALK_WITH_GUI
        return gui::start(argc, argv);
#else
        std::cout << "GUI not built in this configuration (enable VOIDWALK_BUILD_GUI).\n";
        return 0;
#endif
    }

    // The TUI needs the target file opened up front.

    std::shared_ptr<AddressSpace> data;
    try {
        data = std::make_shared<AddressSpace>(argv[argc - 1]);
    }
    catch (const std::exception& e) {
        std::cerr << "voidwalk: cannot open " << argv[argc - 1] << " - " << message(e) << "\n";
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
        std::cerr << "voidwalk: cannot parse " << argv[argc - 1] << " - " << message(e) << "\n";
        return 1;
    }
    // "--tui <binary>" or a bare <binary>: the TUI is the default interface
#ifdef VOIDWALK_WITH_TUI
    return tui::start(argc, argv, status, data, disassembler);
#else
    std::cout << "Analyzing file " << argv[argc - 1] << status
              << " Architecture -> " << disassembler->getArchitecture() << "\n\n"
              << "(TUI not built in this configuration (enable VOIDWALK_BUILD_TUI).)\n";
    return 0;
#endif
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
        std::cerr << "voidwalk: unhandled error - " << message(e) << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "voidwalk: unhandled error of unknown type\n";
        return 1;
    }
}
