#pragma once
#include "tui/session.hpp"

namespace tui {

// Owns the Session and the FTXUI screen loop. Layout and vocabulary follow the
// Qt MainWindow one-to-one (ANSI 16 colours, CP437 glyphs):
//
//   File  Debug  Edit  View  Tools                         voidwalk - <path>
//   ≡ │ Open... ^O │ ► Run F5  Step Into F7 ... │ Recompile   [Go to address ^G]  Settings
//   ──────────────────────────────────────────────────────────────────────────
//   SYMBOLS │ ADDRESS  BYTES  INSTRUCTION  NOTES │ Registers  Stack
//   ──────────────────────────────────────────────────────────────────────────
//   Memory   Section: [Jump to... ▼]   Go to offset: [0x0]
//   ──────────────────────────────────────────────────────────────────────────
//   <status message>                          │ N instr │ ELF ∙ x86-64
class UI {
public:
	explicit UI(Session session);

	// Builds the view and blocks in ScreenInteractive::Loop until the user quits.
	int start();

private:
	Session session_;
};

} // namespace tui
