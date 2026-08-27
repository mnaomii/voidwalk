// Standalone entry point for the test binary (voidwalk-tests). Kept separate from the
// main voidwalk executable so tests build and run with no TUI/GUI dependency - which is
// exactly what the CI job compiles. Exit code is 0 on success, 1 if any check failed.
#include "runner.hpp"

int main() {
    return runTests() == 0 ? 0 : 1;
}
