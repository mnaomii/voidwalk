#include "panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

namespace tui {

namespace {

// Same shape as DisassemblyPaneImpl: a Menu over session.stackRows() with the
// selected index owned by the component so it survives across renders.
class StackPaneImpl : public ftxui::ComponentBase {
public:
	explicit StackPaneImpl(Session& session) : session_(session) {
		menu_ = ftxui::Menu(&session_.stackRows(), &selected_, ftxui::MenuOption::Vertical());
		Add(menu_);
	}

	ftxui::Element OnRender() override {
		// Defensive clamp: stackRows() can shrink (e.g. re-open with no
		// simulated stack yet) between renders.
		int last = static_cast<int>(session_.stackRows().size()) - 1;
		if (last < 0) last = 0;
		if (selected_ > last) selected_ = last;
		if (selected_ < 0) selected_ = 0;

		return ftxui::window(ftxui::text("Stack"), menu_->Render() | ftxui::frame);
	}

private:
	Session& session_;
	int selected_ = 0;
	ftxui::Component menu_;
};

} // namespace

ftxui::Component StackPane(Session& session) {
	return ftxui::Make<StackPaneImpl>(session);
}

} // namespace tui
