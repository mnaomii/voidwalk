#pragma once
#include "address_space.hpp"
#include "disassembler/disassembler.hpp"
#include "disassembler/format/elf/elf_disassembler.hpp"
#include "disassembler/format/pe/pe_disassembler.hpp"

#include <memory>
#include <stdexcept>
#include <string>

// File-type detection + disassembler factory, shared by main.cpp and the UI layers
// (the TUI "Open" action loads new binaries at runtime through these).

namespace voidwalk {

// Identifies the container format from the file's magic bytes: \x7fELF for ELF,
// or MZ plus a PE\0\0 signature at the offset stored at 0x3C for PE.
//
// Sets at most one of the two flags to true and NEVER sets either to false, so
// callers must initialise both before calling. Neither being set means the file is
// something else - this reports that by leaving them alone rather than throwing.
//
// Reads nothing past a bounds check, so a file too short to hold the magic simply
// matches nothing.
inline void determine_filetype(AddressSpace& contents, bool& is_elf, bool& is_pe) {

        if (contents.size() >= 4 && contents.read_u8(0) == 0x7F && contents.read_u8(1) == 0x45 &&
            contents.read_u8(2) == 0x4C && contents.read_u8(3) == 0x46) is_elf = true;
        else if (contents.size() >= 2 && contents.read_u8(0) == 0x4D && contents.read_u8(1) == 0x5A) { // ms-dos compat line
            uint32_t pe_header_offset = contents.read_u32(0x3C); // PE header offset pointer
            if (pe_header_offset <= contents.size() - 4 &&
                contents.read_u32(pe_header_offset) == 0x00004550) is_pe = true;
        }

}

// Constructs the disassembler matching `data`'s format into `*d`, and returns the
// detection message the frontends put in their status bar.
//
// `outputs` is forwarded to the Disassembler: every decoded line is written to
// each stream as the sweep emits it. The GUI and TUI pass none (they read the
// stores instead); the CLI passes stdout plus any files named on the command line.
//
// The constructed Disassembler holds a reference to `data`, which must therefore
// outlive it - the Session lifetime rule exists to enforce exactly this.
//
// Throws std::runtime_error if the file is neither ELF nor PE, and whatever the
// format reader throws for a header it cannot parse.
inline std::string make_disassembler(AddressSpace& data, std::shared_ptr<Disassembler>* d, std::vector<std::ostream*> outputs = {}) {
    bool is_elf = false, is_pe = false;

    determine_filetype(data, is_elf, is_pe);

    //if (outputs.empty()) outputs = std::vector<std::ostream*>{ &std::cout };

    if (is_elf) {

        *d = std::make_shared<ELF_Disassembler>(data, outputs);
        return  "\nELF Binary detected..\n";
    }
    if (is_pe) {

        *d = std::make_shared<PE_Disassembler>(data, outputs);
        return  "\nPE Binary detected..\n";
    }
    throw std::runtime_error("Not an ELF or PE binary.");
}

} // namespace voidwalk


