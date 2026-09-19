#pragma once
#include "cli/print_to_console.hpp"
#include "cli/hex_reader.hpp"

namespace cli {



// Entry point for the non-interactive modes (--print, --dump-hex).
// Named start() to match gui::start / tui::start.
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

