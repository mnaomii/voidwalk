#pragma once
#include "disassembler/disassembler.hpp"
#include "disassembler/format/detect.hpp"
#include "address_space.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace cli {

// The analysis core lives in namespace voidwalk.
using voidwalk::AddressSpace;
using voidwalk::Disassembler;
using voidwalk::make_disassembler;


// Disassembles argv[2] and writes the listing to stdout plus every path in
// argv[3..argc), each opened truncating.
//
// The input is mapped before any output is opened, and an output that is the
// input under another name (./name, a symlink, a hard link) is skipped with a
// warning on stderr: opening it would truncate the binary being read.
//
// Throws for every failure - an unreadable input, an unknown format, an output
// that cannot be created - so main() reports it and exits 1.
inline void printToConsole(int argc, char** argv) {
    const std::string filePath(argv[2]);
    AddressSpace contents(filePath);

    std::vector<std::unique_ptr<std::ofstream>> owned;   // hold streams alive
    std::vector<std::ostream*> streams{ &std::cout };

    for (int i = 3; i < argc; ++i) {
        std::error_code ec;   // a path that does not exist yet is simply not the input
        if (std::filesystem::equivalent(filePath, argv[i], ec)) {
            std::cerr << "voidwalk: skipping " << argv[i] << ": it is the binary being disassembled\n";
            continue;
        }

        auto ofs = std::make_unique<std::ofstream>(argv[i]);
        if (!*ofs) throw std::runtime_error(std::string("cannot open ") + argv[i] + " for writing");

        streams.push_back(ofs.get());
        owned.push_back(std::move(ofs));
    }

    std::shared_ptr<Disassembler> disasm;
    make_disassembler(contents, &disasm, streams);
    disasm->decode();
}

} // namespace cli
