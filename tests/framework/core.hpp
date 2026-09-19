#pragma once
//
// Pulls in the analysis core and imports namespace voidwalk for every suite.
//
// This is the only place in the project that imports the namespace wholesale.
// voidwalk-tests is a closed set of translation units that is never installed, so
// nothing can collide; product code uses explicit `using voidwalk::X;` instead.
//
// Suites get this through tests/framework/base.hpp and should not include the core
// headers individually.
//
#include "address_space.hpp"
#include "disassembler/disassembler.hpp"
#include "disassembler/format/detect.hpp"
#include "disassembler/format/section.hpp"
#include "disassembler/format/elf/elf_disassembler.hpp"
#include "disassembler/format/pe/pe_disassembler.hpp"

using namespace voidwalk;
