#include "panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"

namespace tui {

// Plain display, no per-row selection makes sense for registers - a Renderer
// re-reads session.registerRows() fresh on every render. yframe keeps a tiny
// terminal from having the register block blow out the rest of the layout.
ftxui::Component RegistersPane(Session& session) {
	return ftxui::Renderer([&session] {
		ftxui::Elements rows;
		for (const auto& row : session.registerRows())
			rows.push_back(ftxui::text(row));
		return ftxui::window(ftxui::text("Registers"), ftxui::vbox(std::move(rows)) | ftxui::yframe);
	});
}

} // namespace tui
