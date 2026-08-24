#include "printToConsole.hpp"
#include "hexReader.hpp"

inline void console(int argc, char** argv) {
	if (argc <= 1) return;

	std::string command = argv[1];
	
	if (command == "--print")
		if (argc <= 2) throw std::invalid_argument("No file was provided\n");
		else printToConsole(argc, argv);
	else if (command == "--dump-hex")
		if (argc <= 2) throw std::invalid_argument("No file was provided\n");
		else outputHex(readFile(argv[2]));

}
