#pragma once
#include "address_space.hpp"
#include "disassembler/disassembler.hpp"
#include <memory>
#include <string>

namespace tui {

// Wraps an already-loaded binary in a tui::Session, runs the FTXUI application to
// completion, and returns the process exit code. `status` is the loader's detection
// message, shown in the status bar with surrounding newlines stripped.
int start(int argc, char** argv, std::string status,
          std::shared_ptr<voidwalk::AddressSpace> space,
          std::shared_ptr<voidwalk::Disassembler> disassembler);

} // namespace tui
