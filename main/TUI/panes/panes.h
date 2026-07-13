#ifndef TUI_PANES_H
#define TUI_PANES_H

#include "../model/session.h"

#include "ftxui/component/component.hpp"

// Each pane is a free factory function returning a focusable ftxui::Component
// bound to the Session (same convention as the parsers/ free functions).
// The Session outlives every pane - the UI class owns both.
//
// Implementations live in one .cpp per pane in this folder.

namespace tui {

// Scrollable list of Session::disassemblyLines(); highlights selected row.
ftxui::Component DisassemblyPane(Session& session);

// Session::registerRows(); short, but still scrollable on tiny terminals.
ftxui::Component RegistersPane(Session& session);

// Scrollable Session::stackRows(), top of stack first.
ftxui::Component StackPane(Session& session);

// Hexdump of the whole binary via Session::bytes(): offset | 16 hex | ascii.
// Arrow keys / PageUp / PageDown / Home scroll by row; starts at .text.
ftxui::Component MemoryPane(Session& session);

} // namespace tui

#endif
