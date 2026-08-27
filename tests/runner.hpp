#pragma once

// Runs every suite and returns the number of failed checks (0 == all passed).
// The standalone test binary (tests/test_main.cpp) turns a non-zero return into a
// non-zero process exit code, which is what a CI job keys off.
int runTests();
