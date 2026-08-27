#pragma once
//
// Shared harness for the x86/x86-64 decoder suites (IA-32 and AMD64).
//
// The decoder's single-line entry point, Disassembler::decodeLine_x86_64(addr, vaddr,
// is64Bit), is protected. A tiny probe subclass with a no-op setHeadersOffsets() reaches
// it and decodes a hand-written instruction stream straight out of a temp file - no ELF/PE
// loader, no section parsing, no frontend. This is exactly the "core probe" the audits
// describe, wired up as a reusable fixture.
//
#include <string>
#include <vector>
#include <algorithm>

#include "../../../main/disassembler/disassembler.hpp"
#include "../../../main/address-space/address_space.hpp"
#include "../../fixtures.hpp"

namespace x86dec {

struct Decoded {
    std::string text;      // rendered assembly (decodeLineString)
    std::string machine;   // machine-code column (getMachineCode)
    uint64_t    length = 0;// bytes consumed = next offset, since decoding starts at 0
    bool        truncated = false; // byte-eater hit end-of-file (no forward progress)
    bool        threw = false;     // decode raised (a bug for the inputs we feed)
};

// Reaches the protected decoder and exposes the decoded Instruction's rendered strings.
class DecoderProbe : public Disassembler {
public:
    explicit DecoderProbe(AddressSpace& as) : Disassembler(as, {}) {}

    void setHeadersOffsets() override {}
    std::string getArchitecture() override { return "probe"; }
    uint64_t decodeLine(uint64_t address, uint64_t vaddr) override {
        return decodeLine_x86_64(address, vaddr, is64_);
    }

    Decoded run(bool is64Bit, uint64_t vaddr) {
        is64_ = is64Bit;
        Decoded d;
        try {
            const uint64_t next = decodeLine_x86_64(0, vaddr, is64Bit);
            d.length = next;                 // start offset is 0
            d.truncated = (next == 0);       // catch path returns initAddress (== 0)
            const auto& v = getDecodedInstructions();
            if (!d.truncated && !v.empty()) {
                d.text    = v.back()->decodeLineString();
                d.machine = v.back()->getMachineCode();
            }
        } catch (const std::exception& e) {
            d.threw = true;
            d.text  = std::string("<threw: ") + e.what() + ">";
        }
        return d;
    }

private:
    bool is64_ = false;
};

// Strip all spaces and tabs. The renderer separates mnemonic/operands with a mix of
// spaces and a tab ("MOV \tEBX, EAX"); comparing on the whitespace-free form makes the
// assertions robust to that spacing without losing any operand content.
inline std::string norm(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(),
                           [](char c){ return c == ' ' || c == '\t'; }),
            s.end());
    return s;
}

// Decode one instruction. `instr` is the exact instruction bytes; the fixture pads the
// file so the byte-eater never runs off the end.
inline Decoded decodeOne(const std::vector<uint8_t>& instr, bool is64Bit, uint64_t vaddr = 0x1000) {
    fixtures::TempBinary tmp(fixtures::code(instr));
    AddressSpace as(tmp.path());
    DecoderProbe probe(as);
    return probe.run(is64Bit, vaddr);
}

// Same, but with NO trailing padding - used to prove that a genuinely truncated
// instruction is reported as no-progress rather than read past the file.
inline Decoded decodeExact(const std::vector<uint8_t>& instr, bool is64Bit, uint64_t vaddr = 0x1000) {
    fixtures::TempBinary tmp(fixtures::code(instr, /*pad=*/false));
    AddressSpace as(tmp.path());
    DecoderProbe probe(as);
    return probe.run(is64Bit, vaddr);
}

} // namespace x86dec
