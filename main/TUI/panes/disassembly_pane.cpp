#include "panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

#include <algorithm>
#include <string>

namespace tui {

namespace {

// Scrollable disassembly list with its own cursor + window state. It renders ONLY
// the lines around the cursor, never the whole vector: feeding all of
// Session::disassemblyLines() to a Menu (as this pane used to) builds a DOM element
// per line every single frame — O(n) — which made a large binary crawl even after
// decode finished. The window follows the cursor; yframe + focus do the fine scroll
// within the pane's actual height. session.disassemblyLines() is read fresh every
// render, and refresh() only appends/clears it, so there are no stale iterators.
class DisassemblyPaneImpl : public ftxui::ComponentBase {
public:
	explicit DisassemblyPaneImpl(Session& session) : session_(session) {}

	bool Focusable() const override { return true; }

	bool OnEvent(ftxui::Event event) override {
		const int total = static_cast<int>(session_.disassemblyLines().size());
		if (total == 0) return false;

		if (event == ftxui::Event::ArrowUp)   { move(-1, total); return true; }
		if (event == ftxui::Event::ArrowDown) { move(+1, total); return true; }
		if (event == ftxui::Event::PageUp)    { move(-kPage, total); return true; }
		if (event == ftxui::Event::PageDown)  { move(+kPage, total); return true; }
		if (event == ftxui::Event::Home)      { selected_ = 0; return true; }
		if (event == ftxui::Event::End)       { selected_ = total - 1; return true; }
		return false;
	}

	ftxui::Element OnRender() override {
		ftxui::Element title = ftxui::text("Disassembly");
		if (Focused()) title = title | ftxui::inverted; // visible focus cue

		const auto& lines = session_.disassemblyLines();
		const int total = static_cast<int>(lines.size());
		if (total == 0)
			return ftxui::window(title, ftxui::text("  [decoding...]"));

		// The vector can shrink when a new (smaller) file is opened.
		if (selected_ >= total) selected_ = total - 1;
		if (selected_ < 0) selected_ = 0;

		// Slide the render window so it always contains the cursor, then clamp it.
		if (selected_ < rowOffset_) rowOffset_ = selected_;
		if (selected_ >= rowOffset_ + kRenderRows) rowOffset_ = selected_ - kRenderRows + 1;
		const int maxOffset = total > kRenderRows ? total - kRenderRows : 0;
		if (rowOffset_ > maxOffset) rowOffset_ = maxOffset;
		if (rowOffset_ < 0) rowOffset_ = 0;

		const int end = std::min(total, rowOffset_ + kRenderRows);
		ftxui::Elements out;
		out.reserve(static_cast<size_t>(end - rowOffset_));
		for (int idx = rowOffset_; idx < end; ++idx) {
			ftxui::Element row = ftxui::text(lines[static_cast<size_t>(idx)]);
			if (idx == selected_) {
				// inverted = the highlight; focus/select = the anchor yframe scrolls to
				// (focus while this pane holds focus, the weaker select otherwise).
				row = row | ftxui::inverted;
				row = Focused() ? (row | ftxui::focus) : (row | ftxui::select);
			}
			out.push_back(std::move(row));
		}

		return ftxui::window(title, ftxui::vbox(std::move(out)) | ftxui::yframe);
	}

private:
	void move(int delta, int total) {
		selected_ += delta;
		if (selected_ < 0) selected_ = 0;
		if (selected_ >= total) selected_ = total - 1;
	}

	// kRenderRows is a render budget, not the pane height: build a generous window
	// (cheap) and let yframe clip it to whatever the terminal actually gives us.
	static constexpr int kRenderRows = 200;
	static constexpr int kPage = 20;

	Session& session_;
	int selected_ = 0;
	int rowOffset_ = 0;
};

} // namespace

ftxui::Component DisassemblyPane(Session& session) {
	return ftxui::Make<DisassemblyPaneImpl>(session);
}

} // namespace tui
