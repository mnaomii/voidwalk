#pragma once
//
// Shared harness for the x86/x86-64 decoder suites (IA-32 and AMD64).
//
// It decodes a hand-written instruction stream straight out of a temp file - no
// ELF/PE loader, no section parsing, no frontend. This is the "core probe" the
// audits describe, wired up as a reusable fixture.
//
// It used to have to be a probe *subclass* of Disassembler, with a no-op
// setHeadersOffsets(), because the only way in was the protected member
// Disassembler::decodeLine_x86_64(). Now that the decoder is its own class behind
// the Decoder interface, the fixture just constructs an X86Decoder and calls it -
// the suites test the decoder without dragging a container parser in at all.
//
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "arch/instruction.hpp"
#include "arch/x86_64/x86_64_decoder.hpp"
#include "address_space.hpp"
#include "tests/framework/core.hpp"
#include "tests/framework/fixtures.hpp"

namespace x86dec {

struct Decoded {
    std::string text;      // rendered assembly (decodeLineString)
    std::string machine;   // machine-code column (getMachineCode)
    uint64_t    length = 0;// bytes consumed = next offset, since decoding starts at 0
    bool        truncated = false; // byte-eater hit end-of-file (no forward progress)
    bool        threw = false;     // decode raised (a bug for the inputs we feed)
};

// Decodes one instruction with the real X86Decoder and exposes the rendered strings.
class DecoderProbe {
public:
    explicit DecoderProbe(AddressSpace& as) : as_(as) {}

    Decoded run(bool is64Bit, uint64_t vaddr) {
        X86Decoder decoder(is64Bit);
        std::vector<std::unique_ptr<Instruction>> decoded;
        Decoded d;
        try {
            const uint64_t next = decoder.decodeLine(as_, 0, vaddr, decoded);
            d.length = next;                 // start offset is 0
            d.truncated = (next == 0);       // catch path returns initAddress (== 0)
            if (!d.truncated && !decoded.empty()) {
                d.text    = decoded.back()->decodeLineString();
                d.machine = decoded.back()->getMachineCode();
            }
        } catch (const std::exception& e) {
            d.threw = true;
            d.text  = std::string("<threw: ") + e.what() + ">";
        }
        return d;
    }

private:
    AddressSpace& as_;
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
