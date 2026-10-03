#include "tui/panes/panes.hpp"

namespace tui {

using namespace ftxui;

// gui::StackPane shows this placeholder while Session::stack() is empty, which it
// always is until the debugger executes instructions.
Element StackPane::render(bool /*focused*/) {
	(void)s_;
	const std::string msg = std::string("Stack empty ") + t_.g().dash
		+ " simulation is WIP (fills once the debugger can execute instructions).";
	return vbox({ filler(), hbox({ text(" "), paragraphAlignCenter(msg) | t_.fg(Role::Muted), text(" ") }), filler() });
}

} // namespace tui
