#pragma once
#include <string>
#include <iostream>
#include "console.hpp"


// Minimal, dependency-free check framework shared by every suite.
//
// Design goals (all four matter for CI):
//   * No reliance on assert(): a Release build defines NDEBUG and compiles asserts
//     out, so the whole suite would silently "pass". We tally failures ourselves.
//   * No process abort on the first failure: every check runs, the count is returned
//     from runTests(), and the CI wrapper turns a non-zero count into exit code 1.
//   * Known defects stay VISIBLE. A suite that deletes the cases it cannot pass
//     reports "all green" while the product is broken - which is exactly what
//     happened here: 141 checks passed on a decoder that could not finish a
//     hello-world, because the R8..R15 cases had been commented out. expect_xfail()
//     keeps such a case running and counted, without failing the build.
//   * Environment-dependent tests SKIP rather than fail, so a bare CI box that has
//     no system binaries to disassemble does not produce a red run.
//
// Each concept-suite derives from Tests and calls expect()/expect_eq()/expect_xfail()
// in its body; the counters are static so they aggregate across all suites in one run.
class Tests {
public:
    static inline int failures = 0;   // checks that did not hold
    static inline int checks   = 0;   // checks attempted
    static inline int xfailed  = 0;   // known-defect checks that failed, as expected
    static inline int xpassed  = 0;   // known-defect checks that unexpectedly PASSED
    static inline int skipped  = 0;   // checks not run (missing environment)

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

    // A check that is KNOWN to fail because of an open defect. `ref` names the finding
    // (e.g. "AUDIT.md B1") so the marker can be traced back to a written diagnosis.
    //
    //   cond == false -> xfail: expected, reported, does NOT fail the build.
    //   cond == true  -> XPASS: the defect looks fixed. That IS a failure, because the
    //                    marker is now lying and must be removed - otherwise a fixed bug
    //                    silently regresses back to "known broken" the next time someone
    //                    reads the suite.
    bool expect_xfail(bool cond, const std::string& what, const std::string& ref) {
        ++checks;
        if (!cond) {
            ++xfailed;
            std::cout << "        " << test_console::dim << "✗ xfail: " << what
                      << "  [known: " << ref << "]" << test_console::reset << '\n';
        } else {
            ++xpassed; ++failures;
            std::cout << "        " << test_console::red << "! XPASS: " << test_console::reset
                      << what << "\n           " << ref
                      << " appears FIXED - remove the expect_xfail marker.\n";
        }
        return cond;
    }

    // Same, for a string comparison whose expected value is not yet produced.
    bool expect_eq_xfail(const std::string& got, const std::string& want,
                         const std::string& what, const std::string& ref) {
        const bool ok = (got == want);
        if (!ok)
            std::cout << "           " << test_console::dim << "(want [" << want
                      << "], got [" << got << "])" << test_console::reset << '\n';
        return expect_xfail(ok, what, ref);
    }

    // Not run: the environment could not provide what the check needs (no system
    // binary to disassemble, no /proc, ...). Reported, never a failure.
    void skip(const std::string& what, const std::string& why) {
        ++skipped;
        std::cout << "        " << test_console::dim << "- skip: " << what
                  << "  (" << why << ")" << test_console::reset << '\n';
    }

    Tests() = default;
    virtual ~Tests() = default;
};
