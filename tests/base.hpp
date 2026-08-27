#pragma once
#include <string>
#include <iostream>
#include "console.hpp"


// Minimal, dependency-free check framework shared by every suite.
//
// Design goals (both matter for CI):
//   * No reliance on assert(): a Release build defines NDEBUG and compiles asserts
//     out, so the whole suite would silently "pass". We tally failures ourselves.
//   * No process abort on the first failure: every check runs, the count is returned
//     from runTests(), and the CI wrapper turns a non-zero count into exit code 1.
//
// Each concept-suite derives from Tests and calls expect()/expect_eq() in its body;
// the counters are static so they aggregate across all suites in one run.
class Tests {
public:
    static inline int failures = 0;   // checks that did not hold
    static inline int checks   = 0;   // checks attempted

    // Section banners, kept from the original harness so the run reads nicely.
    void running(const char* name) {
        std::cout << test_console::dim << "  (~)  " << name << "()\n" << test_console::reset;
    }
    void passed(const char* name) {
        std::cout << test_console::green << "  (✓)  " << test_console::reset
                  << name << "()\n";
    }

    // Core assertion. Returns the condition so callers can short-circuit follow-ups.
    bool expect(bool cond, const std::string& what) {
        ++checks;
        if (cond) {
            std::cout << "        " << test_console::green << "✓ " << test_console::reset
                      << what << '\n';
        } else {
            ++failures;
            std::cout << "        " << test_console::red << "✗ FAIL: " << test_console::reset
                      << what << '\n';
        }
        return cond;
    }

    // String equality with both values echoed on failure - the common case for the
    // decoder (rendered line) and the parsers (hex-formatted field values).
    bool expect_eq(const std::string& got, const std::string& want, const std::string& what) {
        const bool ok = (got == want);
        ++checks;
        if (ok) {
            std::cout << "        " << test_console::green << "✓ " << test_console::reset
                      << what << '\n';
        } else {
            ++failures;
            std::cout << "        " << test_console::red << "✗ FAIL: " << test_console::reset
                      << what << "\n           expected [" << want << "]\n"
                      << "           got      [" << got << "]\n";
        }
        return ok;
    }

    // Integer equality, same reporting shape.
    bool expect_eq(long long got, long long want, const std::string& what) {
        const bool ok = (got == want);
        ++checks;
        if (ok) {
            std::cout << "        " << test_console::green << "✓ " << test_console::reset
                      << what << '\n';
        } else {
            ++failures;
            std::cout << "        " << test_console::red << "✗ FAIL: " << test_console::reset
                      << what << "\n           expected [" << want << "]  got [" << got << "]\n";
        }
        return ok;
    }

    Tests() = default;
    virtual ~Tests() = default;
};
