#include "mainUI.h"

#include "../model/session.h"
#include "../ui/ui.h"

#include <string>
#include <utility>

int UIstart(int argc, char** argv, std::string status,
            std::shared_ptr<AddressSpace> space,
            std::shared_ptr<Disassembler> disassembler) {
	std::string filePath = (argc > 0) ? argv[argc - 1] : std::string();

	tui::Session session(std::move(space), std::move(disassembler), std::move(filePath));

	// The loader's detection message is wrapped in newlines (e.g. "\nELF Binary
	// detected..\n"); strip them so it fits the one-line status bar.
	std::string::size_type begin = status.find_first_not_of("\r\n");
	if (begin == std::string::npos) {
		status.clear();
	} else {
		std::string::size_type end = status.find_last_not_of("\r\n");
		status = status.substr(begin, end - begin + 1);
	}
	session.setStatus(status);

	tui::UI ui(std::move(session));
	return ui.start();
}
