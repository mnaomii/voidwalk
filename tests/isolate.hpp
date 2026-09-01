#pragma once
//
// Crash isolation for the robustness suite.
//
// A disassembler is pointed at files it does not trust - that is the whole job. So
// "does not crash on a malformed binary" is a functional requirement, and a test for
// it must be able to OBSERVE a crash rather than die with the runner. Running the
// candidate in a forked child gives us that: the child segfaults, the parent reads
// the wait status and reports a failed check.
//
// This is the same shape a fuzzing harness uses, and it is why the robustness suite
// can assert on the three memory-safety findings without taking the whole run down.
//
// POSIX: fork(). Windows: no cheap fork, so the body runs in-process - a crash there
// takes the runner down, which is a loud (if less informative) failure. The suite
// reports which mode it is in so a Windows red run is not mistaken for a hang.
//
// IMPORTANT - this suite is only authoritative under sanitizers.
//
// "Did not crash" catches only the memory errors that actually fault. Reading
// instructionBytes[-1] reads adjacent stack and returns happily; a plain build calls
// that a pass. The build that gives these checks teeth is:
//
//     -fsanitize=address,undefined -fno-sanitize-recover=all
//
// -fno-sanitize-recover matters as much as the sanitizers themselves: UBSan's default
// is to PRINT a diagnostic and carry on, so an out-of-bounds index is reported to the
// log and the check still passes. With it, any detected UB aborts, the child dies on a
// signal, and the check fails as it should.
//
#include <string>
#include <exception>

#ifndef _WIN32
  #include <sys/wait.h>
  #include <unistd.h>
  #include <csignal>
  #include <cstdlib>
#endif

namespace isolate {

// Is this build instrumented? A memory-safety verdict is only trustworthy when it is.
//
// Without instrumentation a check like "15 prefixes then an opcode" reads
// instructionBytes[-1], gets whatever is next to it on the stack, and returns
// normally - so the check PASSES and the suite reports a buffer underflow as fixed.
// Suites use this to skip such a case rather than answer it wrongly: "unknown" is a
// far more useful result than a confident false negative.
#if defined(__SANITIZE_ADDRESS__)
inline constexpr bool instrumented = true;
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(undefined_behavior_sanitizer)
inline constexpr bool instrumented = true;
#  else
inline constexpr bool instrumented = false;
#  endif
#else
inline constexpr bool instrumented = false;
#endif

enum class Outcome {
    Ok,        // returned normally
    Threw,     // raised a C++ exception - a controlled rejection, which is fine
    Crashed,   // died on a signal (SIGSEGV/SIGBUS/SIGABRT) - a real defect
    Unsupported// could not isolate (no fork); body was not run
};

inline const char* describe(Outcome o) {
    switch (o) {
        case Outcome::Ok:      return "returned";
        case Outcome::Threw:   return "threw";
        case Outcome::Crashed: return "CRASHED";
        default:               return "not isolated";
    }
}

#ifndef _WIN32

// True when the platform can actually contain a crash.
inline constexpr bool supported = true;

// The child's "I caught a C++ exception" exit code.
//
// This deliberately is NOT 1. AddressSanitizer's default error exitcode IS 1, so
// using 1 here would make a sanitizer-detected memory fault indistinguishable from a
// clean, intentional rejection - the robustness suite would report "rejected safely"
// for exactly the buffer overflows it exists to find, and it would do so only in the
// sanitizer build that is supposed to be the authoritative one. Anything that is
// neither 0 nor this value is therefore treated as a crash.
constexpr int kThrewExitCode = 70;

// Runs `fn` in a forked child. The child's exit code encodes the outcome so the
// parent can tell a clean return from a caught exception from a fatal signal.
//
// _exit() (not exit()) on every child path: the child inherits the parent's stdio
// buffers and its atexit handlers, and flushing those would duplicate the parent's
// pending test output into the log.
template <class F>
Outcome run(F&& fn) {
    std::fflush(stdout);
    std::fflush(stderr);

    const pid_t pid = fork();
    if (pid < 0) return Outcome::Unsupported;

    if (pid == 0) {
        int code = 0;
        try {
            fn();
            code = 0;
        } catch (const std::exception&) {
            code = kThrewExitCode;
        } catch (...) {
            code = kThrewExitCode;
        }
        std::fflush(nullptr);
        _exit(code);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return Outcome::Unsupported;

    if (WIFSIGNALED(status)) return Outcome::Crashed;      // SIGSEGV / SIGBUS / SIGABRT
    if (WIFEXITED(status)) {
        const int code = WEXITSTATUS(status);
        if (code == 0)                return Outcome::Ok;
        if (code == kThrewExitCode)   return Outcome::Threw;
        return Outcome::Crashed;   // sanitizer abort, std::terminate, _Exit from a test
    }
    return Outcome::Crashed;
}

#else

inline constexpr bool supported = false;

// No isolation available: run in-process. A genuine memory-safety fault will take
// the runner down here instead of being reported as one failed check.
template <class F>
Outcome run(F&& fn) {
    try {
        fn();
        return Outcome::Ok;
    } catch (const std::exception&) {
        return Outcome::Threw;
    } catch (...) {
        return Outcome::Threw;
    }
}

#endif

} // namespace isolate
