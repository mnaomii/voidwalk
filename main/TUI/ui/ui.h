#ifndef UI_H
#define UI_H

#include "../model/session.h"

namespace tui {

// Owns the Session and the FTXUI screen loop. Layout:
//
//   [Open] [Save*] [Run*] [Step*] [Break*] [Quit]        top bar (*=placeholder)
//   +-- Disassembly ---------------+-- Registers --+
//   |                              +-- Stack ------+
//   +-- Memory ------------------------------------+
//   file | format | arch | status                        status bar
//
// Placeholder actions only set session.setStatus("... not implemented yet").
// "Open" is real: modal path prompt -> Session::open().
class UI {
public:
	explicit UI(Session session);

	// Builds the component tree and blocks in ScreenInteractive::Loop
	// until the user quits. Returns process exit code.
	int start();

private:
	Session session_;
};

} // namespace tui

#endif
