#include <iostream>

#include "runner.hpp"
#include "base.hpp"
#include "console.hpp"

#include "address_space/address-space-tests.hpp"
#include "disassembler/loader-tests.hpp"
#include "disassembler/elf-sections-tests.hpp"
#include "disassembler/pe-sections-tests.hpp"
#include "disassembler/x86_64-tests/IA-32-tests.hpp"
#include "disassembler/x86_64-tests/AMD64-tests.hpp"
#include "disassembler/sweep-tests.hpp"
#include "robustness/malformed-tests.hpp"
#include "concurrency/async-decode-tests.hpp"
#include "integration/real-binary-tests.hpp"
// ARM32 / AArch64 decoders are not implemented, so no suites exist for them yet
// (tests/disassembler/ARM32-tests.hpp and AArch64-tests.hpp are deliberate stubs).

// The suites are ordered by widening scope, so a failure is read top-down: a broken
// AddressSpace explains a broken loader, which explains a broken sweep. Reading the
// output in that order points at the deepest layer that actually broke.
//
//   unit         AddressSpace, loader, section parsers, per-instruction decode
//   sweep        whole-.text invariants (monotonic, no gaps, bytes match the file)
//   robustness   malformed and hostile input - must reject, never crash
//   concurrency  the lock-free decode/render handoff the GUI and TUI depend on
//   integration  a real compiler-produced binary, end to end
//
// Each suite runs its checks in its constructor; the shared Tests counters aggregate
// across all of them. No suite aborts on failure, so one run reports every problem.
int runTests() {
    test_console::init();   // UTF-8 + ANSI colors when the stream is a real terminal
    using namespace test_console;

    Tests::failures = 0;
    Tests::checks   = 0;
    Tests::xfailed  = 0;
    Tests::xpassed  = 0;
    Tests::skipped  = 0;

    std::cout << cyan << "\n== voidwalk test suite ==\n" << reset;

    auto section = [](const char* name) {
        std::cout << "\n" << test_console::cyan << "  (*) " << test_console::reset << name << "\n";
    };

    // --- unit -------------------------------------------------------------
    section("AddressSpace");    { AddressSpace_Tests  t; }
    section("Loader");          { Loader_Tests        t; }
    section("ELF sections");    { ELF_Sections_Tests  t; }
    section("PE sections");     { PE_Sections_Tests   t; }
    section("IA-32 decoder");   { IA_32_Tests         t; }
    section("AMD64 decoder");   { AMD64_Tests         t; }

    // --- whole-sweep invariants -------------------------------------------
    section("Sweep integrity"); { Sweep_Tests         t; }

    // --- hostile input ------------------------------------------------------
    section("Robustness");      { Malformed_Tests     t; }

    // --- threading contract -------------------------------------------------
    section("Async decode");    { AsyncDecode_Tests   t; }

    // --- end to end ---------------------------------------------------------
    section("Real binaries");   { RealBinary_Tests    t; }

    // --- summary ------------------------------------------------------------
    // The headline number is real failures. The known-defect count is printed next to
    // it and never folded into "passed": a suite that says "all green" while carrying
    // open defects is how the R8..R15 breakage stayed invisible through 141 checks.
    const int genuinePasses = Tests::checks - Tests::failures - Tests::xfailed;

    std::cout << "\n";
    if (Tests::failures == 0)
        std::cout << green << "  " << genuinePasses << " of " << Tests::checks
                  << " checks passed." << reset << "\n";
    else
        std::cout << red << "  " << Tests::failures << " of " << Tests::checks
                  << " checks FAILED." << reset << "\n";

    if (Tests::xfailed > 0)
        std::cout << "  " << Tests::xfailed
                  << " known defect(s) still open (xfail) - see AUDIT.md.\n";
    if (Tests::xpassed > 0)
        std::cout << red << "  " << Tests::xpassed
                  << " known defect(s) now PASS - remove their expect_xfail markers."
                  << reset << "\n";
    if (Tests::skipped > 0)
        std::cout << "  " << Tests::skipped << " check(s) skipped (environment).\n";
    std::cout << "\n";

    return Tests::failures;
}
