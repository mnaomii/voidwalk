#pragma once

// GUI entry point, called from main/main.cpp's "--gui" dispatch (guarded by
// VOIDWALK_WITH_GUI). Not a main() of its own, so the Qt GUI links into the
// single bundled voidwalk binary alongside the TUI and test harness.
int GUIstart(int argc, char** argv);
