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
// ARM32 / AArch64 decoders are not implemented, so no suites exist for them yet
// (tests/disassembler/ARM32-tests.hpp and AArch64-tests.hpp are deliberate stubs).

// Each suite runs its checks in its constructor; the shared Tests counters aggregate
// across all of them. No suite aborts on failure, so one run reports every problem.
int runTests() {
    test_console::init();   // UTF-8 + ANSI colors when the stream is a real terminal
    using namespace test_console;

    Tests::failures = 0;
    Tests::checks   = 0;

    std::cout << cyan << "\n== voidwalk test suite ==\n" << reset;

    auto section = [](const char* name) {
        std::cout << "\n" << test_console::cyan << "  (*) " << test_console::reset << name << "\n";
    };

    section("AddressSpace");  { AddressSpace_Tests  t; }
    section("Loader");        { Loader_Tests        t; }
    section("ELF sections");  { ELF_Sections_Tests  t; }
    section("PE sections");   { PE_Sections_Tests   t; }
    section("IA-32 decoder"); { IA_32_Tests         t; }
    section("AMD64 decoder"); { AMD64_Tests         t; }

    std::cout << "\n";
    if (Tests::failures == 0)
        std::cout << green << "  All " << Tests::checks << " checks passed." << reset << "\n\n";
    else
        std::cout << red << "  " << Tests::failures << " of " << Tests::checks
                  << " checks FAILED." << reset << "\n\n";

    return Tests::failures;
}
