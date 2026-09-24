#pragma once
#include "cli/print_to_console.hpp"
#include "cli/hex_reader.hpp"

namespace cli {



// Entry point for the non-interactive modes. Named start() to match gui::start /
// tui::start, and dispatched from main.cpp on argv[1].
//
//   --print    <binary> [outfile...]   disassemble to stdout and each outfile
//   --dump-hex <binary>                hex dump to stdout
//
// Does nothing when argv[1] is neither. Throws std::invalid_argument when a mode
// was named with no file after it; the handlers throw for their own failures.
inline void start(int argc, char** argv) {
	if (argc <= 1) return;

	std::string command = argv[1];


	if (argc <= 2) throw std::invalid_argument("No file was provided\n");

	if (command == "--print")
		 printToConsole(argc, argv);
	else if (command == "--dump-hex")
		 outputHex(argv[2]);

}

} // namespace cli

