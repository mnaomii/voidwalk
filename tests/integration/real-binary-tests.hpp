#pragma once
//
// End-to-end suite - sweep a real, compiler-produced executable.
//
// This is the suite whose absence let the project report "All 141 checks passed" while
// the decoder could not get past the second instruction of a hello-world. Every other
// suite feeds hand-written byte strings, and hand-written byte strings only ever contain
// what the author already knew to write. Real .text does not: it contains R8..R15, CET
// landing pads, SSE spills, jump tables and alignment padding, in the proportions a
// compiler actually emits them.
//
// The assertions here are deliberately statistical rather than exact. Asserting a
// specific instruction at a specific address in /bin/ls would break on every libc
// rebuild and would be testing the host, not voidwalk. What does not change between
// hosts is the shape of the result:
//
//   * a sweep of a real .text yields thousands of instructions, not one
//   * it covers essentially all of .text, because a linear sweep has nowhere else to go
//   * the average instruction is a few bytes long
//
// A decoder that aborts early fails all three by an enormous margin, which is exactly
// the signal that was missing.
//
// The host may legitimately have no binary to offer (a scratch CI container). The suite
// then SKIPS: a missing sample is not a defect in voidwalk.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/miscellaneous/loader.hpp"

#include <memory>
#include <string>

class RealBinary_Tests : public Tests {

    void sweepSystemBinary() {
        const std::string path = fixtures::findSystemBinary();
        if (path.empty()) {
            skip("sweep a real system binary", "no suitable executable found on this host");
            return;
        }
        std::cout << "        " << test_console::dim << "target: " << path
                  << test_console::reset << '\n';

        std::unique_ptr<AddressSpace> as;
        std::shared_ptr<Disassembler> d;
        try {
            as = std::make_unique<AddressSpace>(path);
            make_disassembler(*as, &d);
        } catch (const std::exception& e) {
            expect(false, std::string("loading ") + path + " failed: " + e.what());
            return;
        }

        const std::string arch = d->getArchitecture();
        expect(arch != "Unknown", "architecture is recognised (" + arch + ")");

        const Header& text = d->getSections()._text;
        if (!expect(text.getSize() > 0, ".text was located and is non-empty"))
            return;

        // A sweep of real code must not raise. decodeLine throws for an unimplemented
        // architecture, which is a legitimate outcome - but on the host's own native
        // binary the decoder is supposed to be the implemented one.
        std::string threw;
        try { d->decode(); }
        catch (const std::exception& e) { threw = e.what(); }

        const size_t count = d->getDecodedInstructions().size();
        const uint64_t size = text.getSize();

        // Everything below is only meaningful for the architectures with a decoder.
        const bool haveDecoder = (arch == "x86" || arch == "x86_64");
        if (!haveDecoder) {
            skip("sweep metrics", arch + " has no decoder yet");
            return;
        }

        // These were split by bitness while AUDIT.md B1 was open: 64-bit targets tripped
        // REX register extension and were tracked as known failures, 32-bit ones were
        // held to the real standard. B1 is fixed, so both are held to it now.
        auto check = [&](bool cond, const std::string& what) { expect(cond, what); };

        check(threw.empty(),
              threw.empty() ? "sweep of real .text completes without raising"
                            : "sweep of real .text raised: " + threw);

        // A real .text averages a few bytes per instruction, so the count should be a
        // large fraction of the byte count. 1 instruction per 16 bytes is a floor no
        // working decoder can miss and no broken one can reach.
        const uint64_t floorCount = size / 16;
        check(count >= floorCount,
              "decoded " + std::to_string(count) + " instructions from "
                  + std::to_string(size) + " bytes of .text (floor "
                  + std::to_string(floorCount) + ")");

        // Coverage: how much of .text the sweep actually walked.
        if (count > 0) {
            const auto& addrs = d->getInstructionAddresses();
            const uint64_t first = addrs.front();
            const uint64_t last  = addrs.back();
            const uint64_t walked = last - first;
            const double pct = size ? (100.0 * double(walked) / double(size)) : 0.0;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.1f%%", pct);
            check(pct >= 90.0,
                  std::string("sweep covered ") + buf + " of .text");

            // Average instruction length has to be architecturally possible. On its own
            // this is a weak signal - it stays plausible when only a handful of
            // instructions decode - which is why the count and coverage checks above
            // carry the actual verdict.
            const double avg = count ? double(walked) / double(count) : 0.0;
            std::snprintf(buf, sizeof(buf), "%.2f", avg);
            expect(avg >= 1.0 && avg <= 15.0,
                   std::string("average instruction length is ") + buf + " bytes");
        }
    }

    // Format detection on a real file, independent of whether decoding works. This
    // part passes today and is worth keeping separate so a decoder regression does not
    // hide a loader regression.
    void testDetectRealFormat() {
        const std::string path = fixtures::findSystemBinary();
        if (path.empty()) { skip("detect a real binary's format", "no executable found"); return; }

        AddressSpace as(path);
        bool is_elf = false, is_pe = false;
        determine_filetype(as, is_elf, is_pe);
        expect(is_elf != is_pe, "a real executable matches exactly one format");
        expect(as.size() > 0, "a real executable maps with a non-zero size");
    }

    void runAll() {
        running("testDetectRealFormat"); testDetectRealFormat();
        running("sweepSystemBinary");    sweepSystemBinary();
    }

public:
    RealBinary_Tests() { runAll(); }
};
