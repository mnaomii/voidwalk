#pragma once
#include "../../address-space/address_space.h"
#include "../../disassembler/disassembler.h"
#include <memory>
#include <string>

// TUI entry point: wraps the already-loaded binary in a tui::Session and
// runs the FTXUI application. `status` is the loader's detection message.
int UIstart(int argc, char** argv, std::string status,
            std::shared_ptr<AddressSpace> space,
            std::shared_ptr<Disassembler> disassembler);
