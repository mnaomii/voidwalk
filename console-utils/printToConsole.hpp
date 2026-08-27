#include "../main/disassembler/disassembler.hpp"
#include "../main/miscellaneous/loader.hpp"
#include "../main/address-space/address_space.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>

inline void printToConsole(int argc, char** argv) {
    if (argc <= 2) throw std::invalid_argument("No file was provided.\n");

    std::string filePath(argv[2]);
    if (filePath.empty()) throw std::invalid_argument("File cannot be opened.\n");

    std::vector<std::unique_ptr<std::ofstream>> owned;   // hold streams alive
    std::vector<std::ostream*> streams{ &std::cout };

    for (int i = 3; i < argc; ++i) {
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
    catch (std::exception&) {
        std::cerr << "Corrupt file";
    }
}