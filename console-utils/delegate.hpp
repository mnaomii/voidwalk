#include "printToConsole.hpp"
#include "hexReader.hpp"

inline void console(int argc, char** argv) {
	if (argc <= 1) return;

	std::string command = argv[1];


	if (argc <= 2) throw std::invalid_argument("No file was provided\n");

	if (command == "--print")
		 printToConsole(argc, argv);
	else if (command == "--dump-hex")
		 outputHex(argv[2]);

}
