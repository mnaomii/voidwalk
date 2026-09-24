#pragma once
#include "disassembler/disassembler.hpp"
#include "disassembler/format/detect.hpp"
#include "address_space.hpp"

#include <vector>
#include <memory>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

namespace cli {

// The analysis core lives in namespace voidwalk.
using voidwalk::AddressSpace;
using voidwalk::Disassembler;
using voidwalk::make_disassembler;


// Disassembles argv[2] and writes the listing to stdout plus every path in
// argv[3..argc), each opened truncating.
//
// An output path equal to the input is skipped with a warning on stderr rather
// than treated as fatal - writing the listing over the binary being read would
// corrupt the mapping mid-sweep.
//
// Throws std::invalid_argument when no file was given, and std::runtime_error
// when an output file cannot be opened. Failures from the open or the sweep
// itself (bad format, unimplemented architecture) are caught here and reported on
// stderr, so this returns normally after a failed decode.
inline void printToConsole(int argc, char** argv) {
    if (argc <= 2) throw std::invalid_argument("No file was provided.\n");

    std::string filePath(argv[2]);
    if (filePath.empty()) throw std::invalid_argument("File cannot be opened.\n");

    std::vector<std::unique_ptr<std::ofstream>> owned;   // hold streams alive
    std::vector<std::ostream*> streams{ &std::cout };

    for (int i = 3; i < argc; ++i) {

        if(strcmp(argv[i],filePath.c_str()) == 0)
        {
            std::cerr << "[voidwalk] Cannot print to " << argv[i] 
            << " : Cannot write to disassembly target.\n";
            
            std::cerr<< "[voidwalk] Skipping past faulty print target.\n";
            
            continue;
        }

        auto ofs = std::make_unique<std::ofstream>(argv[i]);
        if (!*ofs) throw std::runtime_error(std::string("Cannot open ") + argv[i]);

        streams.push_back(ofs.get());
        owned.push_back(std::move(ofs));
    }

    try {
        auto contents = std::make_shared<AddressSpace>(filePath);
        std::shared_ptr<Disassembler> disasm;
        make_disassembler(*contents, &disasm, streams);
        std::cout << std::endl;
        disasm->decode();
    }
    catch (std::exception& e) {
        std::cerr << e.what();
    }
}

} // namespace cli
