#include "tui/panes/panes.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"

namespace tui {

// The base registers are plain text re-read from session.registerRows() every
// render; the vector sets (MMX/SSE/AVX/AVX-512) sit below as Collapsibles, closed
// until the user opens one, each a Menu so its rows scroll. yframe keeps a tiny
// terminal from having the register block blow out the rest of the layout.
//
// The base text is the container's first (and initially active) child and marks
// its first row with `focus`: Checkbox always emits `focus`, so without this the
// frame would scroll down to the MMX header and hide rax.. on a short pane.
ftxui::Component RegistersPane(Session& session) {
	static constexpr const char* kTitles[] = {"MMX", "SSE", "AVX", "AVX-512"};

	auto base = ftxui::Renderer([&session] {
		ftxui::Elements rows;
		for (const auto& row : session.registerRows())
			rows.push_back(ftxui::text(row));
		if (!rows.empty()) rows.front() |= ftxui::focus;
		return ftxui::vbox(std::move(rows));
	});

	auto content = ftxui::Container::Vertical({base});
	for (size_t g = 0; g < 4; ++g) {
		ftxui::MenuOption menu = ftxui::MenuOption::Vertical();
		menu.entries = &session.vectorRegisterRows()[g];
		content->Add(ftxui::Collapsible(kTitles[g], ftxui::Menu(menu)));
	}

	return ftxui::Renderer(content, [content] {
		return ftxui::window(ftxui::text("Registers"), content->Render() | ftxui::yframe);
	});
}

} // namespace tui
