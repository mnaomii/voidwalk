#include "panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

namespace tui {

namespace {

// Owns the Menu's selected-row state so it outlives this factory call.
// session.disassemblyLines() is read fresh on every render; refresh() only
// ever mutates the Session's existing vector in place (clear + push_back),
// so the address handed to Menu stays valid for the pane's whole lifetime.
class DisassemblyPaneImpl : public ftxui::ComponentBase {
public:
	explicit DisassemblyPaneImpl(Session& session) : session_(session) {
		menu_ = ftxui::Menu(&session_.disassemblyLines(), &selected_, ftxui::MenuOption::Vertical());
		Add(menu_);
	}

	ftxui::Element OnRender() override {
		// Defensive clamp: the vector can shrink when a new (smaller) file is
		// opened. Menu clamps internally too, but we own selected_ so we clamp
		// it here before it is ever read by anything else.
		int last = static_cast<int>(session_.disassemblyLines().size()) - 1;
		if (last < 0) last = 0;
		if (selected_ > last) selected_ = last;
		if (selected_ < 0) selected_ = 0;

		return ftxui::window(ftxui::text("Disassembly"), menu_->Render() | ftxui::frame);
	}

private:
	Session& session_;
	int selected_ = 0;
	ftxui::Component menu_;
};

} // namespace

ftxui::Component DisassemblyPane(Session& session) {
	return ftxui::Make<DisassemblyPaneImpl>(session);
}

} // namespace tui
