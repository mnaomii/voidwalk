// Standalone entry point for the test binary (voidwalk-tests). Kept separate from the
// main voidwalk executable so tests build and run with no TUI/GUI dependency - which is
// exactly what the CI job compiles.
//
// Exit codes, which is the whole contract a CI job cares about:
//   0  every check passed (open xfails and environment skips do not fail the run)
//   1  at least one check failed, OR a known-defect marker is stale (XPASS), OR the
//      run could not complete
//
// Note the `!= 0 ? 1` rather than returning the failure count directly: an exit status
// is taken mod 256, so returning 256 failures would report success.
#include "runner.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        const int failures = runTests();
        std::cout.flush();
        return failures != 0 ? 1 : 0;
    }
    catch (const std::exception& e) {
        // A throw that escapes runTests() is a broken run, not a passing one. Without
        // this it reaches std::terminate and the job dies on SIGABRT (exit 134) with a
        // truncated log and no indication of which suite was mid-flight.
        std::cout.flush();
        std::cerr << "\nFATAL: test run aborted - " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cout.flush();
        std::cerr << "\nFATAL: test run aborted - exception of unknown type\n";
        return 1;
    }
}
