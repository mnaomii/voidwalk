#pragma once
#include "cli/print_to_console.hpp"
#include "cli/hex_reader.hpp"

#include <stdexcept>
#include <string>

namespace cli {



// Entry point for the non-interactive modes. Named start() to match gui::start /
// tui::start, and dispatched from main.cpp on argv[1].
//
//   --print    <binary> [outfile...]   disassemble to stdout and each outfile
//   --dump-hex <binary>                hex dump to stdout
//
// Throws std::invalid_argument for a missing or extra argument; the handlers
// throw for their own failures. main() reports both and exits 1.
inline void start(int argc, char** argv) {
	const std::string command = argv[1];
	if (argc <= 2) throw std::invalid_argument(command + " needs a <binary>");

	if (command == "--print")
		printToConsole(argc, argv);
	else if (argc > 3)
		throw std::invalid_argument("--dump-hex takes exactly one <binary>");
	else
		outputHex(argv[2]);
}

} // namespace cli
