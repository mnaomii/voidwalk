#pragma once
#include <memory>

// Each frontend exposes exactly one entry point named start(), in its own
// namespace: gui::start, tui::start, cli::start.
namespace gui {

// Runs the Qt GUI to completion and returns the process exit code. Opens the binary
// at argv[argc - 1] on startup when argc > 2. Called from src/main.cpp's "--gui"
// dispatch; not a main() of its own, so the GUI links into the bundled binary.
int start(int argc, char** argv);

} // namespace gui
