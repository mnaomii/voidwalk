#pragma once
#include <string>
#include "../../disassembler/disassembler.h"
#include <memory>

int UIstart(int argc, char** argv, std::string status, std::shared_ptr<Disassembler> disassembler);