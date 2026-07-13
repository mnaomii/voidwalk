#pragma once
#include <cstdio>
#include <cstdlib>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif


// Console setup for the test runner's output only - the TUI drives its own terminal,
// and the plain interface prints ASCII.
namespace test_console {

    // Empty until init() decides the stream can actually take escape codes, so piping
    // the run to a file ("dat --run-tests bin > log.txt") stays free of escape junk.
    inline std::string_view green, red, cyan, dim, reset;

    inline void init() {

        bool tty = false;

#ifdef _WIN32
        // The Windows console decodes with the process code page, a legacy OEM one by
        // default - the summary's UTF-8 box drawing then arrives as "ΓööΓöÇ".
        SetConsoleOutputCP(CP_UTF8);

        // ANSI escapes are off in conhost unless asked for. Fails on a pipe, which is
        // exactly when colors are unwanted anyway.
        DWORD mode = 0;
        const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        tty = _isatty(_fileno(stdout))
           && GetConsoleMode(out, &mode)
           && SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
        // POSIX terminals are already UTF-8 and already speak ANSI.
        tty = isatty(fileno(stdout));
#endif

        // https://no-color.org - any value set means "do not colorize"
#ifdef _WIN32
        size_t len = 0;
        getenv_s(&len, nullptr, 0, "NO_COLOR");
        const bool noColor = (len > 0);
#else
        const bool noColor = (std::getenv("NO_COLOR") != nullptr);
#endif

        if (!tty || noColor) return;

        green = "\033[32m";
        red   = "\033[31m";
        cyan  = "\033[36m";
        dim   = "\033[2m";
        reset = "\033[0m";
    }
}
