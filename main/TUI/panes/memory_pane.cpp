#include "panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace tui {

namespace {

// "%08X  4D 5A 90 00 00 00 00 00  00 00 00 00 00 00 00 00  MZ.............."
std::string formatHexRow(uint64_t rowIndex, const std::vector<uint8_t>& raw) {
	char offsetBuf[9];
	std::snprintf(offsetBuf, sizeof(offsetBuf), "%08llX", static_cast<unsigned long long>(rowIndex * 16));

	std::string hexPart;
	std::string asciiPart;
	for (int i = 0; i < 16; ++i) {
		if (i == 8) hexPart += ' '; // visual gap between the two 8-byte groups
		if (static_cast<size_t>(i) < raw.size()) {
			char byteBuf[4];
			std::snprintf(byteBuf, sizeof(byteBuf), "%02X ", raw[i]);
			hexPart += byteBuf;
			unsigned char c = raw[i];
			asciiPart += (c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : '.';
		}
		else {
			hexPart += "   "; // keep column alignment past EOF
			asciiPart += ' ';
		}
	}
	return std::string(offsetBuf) + "  " + hexPart + " " + asciiPart;
}

// Focusable hexdump with its own row-scroll state (no Menu here - entries
// aren't discrete selectable rows, just a scrolled byte window).
class MemoryPaneImpl : public ftxui::ComponentBase {
public:
	explicit MemoryPaneImpl(Session& session) : session_(session) {
		rowOffset_ = session_.textOffset() / 16;
	}

	bool Focusable() const override { return true; }

	bool OnEvent(ftxui::Event event) override {
		size_t total = session_.binarySize();
		if (total == 0) return false;
		uint64_t totalRows = (total + 15) / 16;
		uint64_t maxOffset = totalRows > 0 ? totalRows - 1 : 0;

		if (event == ftxui::Event::ArrowUp) {
			if (rowOffset_ > 0) --rowOffset_;
			return true;
		}
		if (event == ftxui::Event::ArrowDown) {
			if (rowOffset_ < maxOffset) ++rowOffset_;
			return true;
		}
		if (event == ftxui::Event::PageUp) {
			rowOffset_ = rowOffset_ > kPageRows ? rowOffset_ - kPageRows : 0;
			return true;
		}
		if (event == ftxui::Event::PageDown) {
			rowOffset_ = std::min(rowOffset_ + kPageRows, maxOffset);
			return true;
		}
		if (event == ftxui::Event::Home) {
			rowOffset_ = 0;
			return true;
		}
		return false;
	}

	ftxui::Element OnRender() override {
		ftxui::Element title = ftxui::text("Memory");
		if (Focused()) title = title | ftxui::inverted; // visible focus cue

		size_t total = session_.binarySize();
		if (total == 0)
			return ftxui::window(title, ftxui::text("[no binary loaded]"));

		uint64_t totalRows = (total + 15) / 16;
		uint64_t maxOffset = totalRows > 0 ? totalRows - 1 : 0;
		if (rowOffset_ > maxOffset) rowOffset_ = maxOffset; // defensive: binary can shrink on reopen

		uint64_t rowsToShow = kVisibleRows;
		if (rowOffset_ + rowsToShow > totalRows) rowsToShow = totalRows - rowOffset_;

		ftxui::Elements lines;
		lines.reserve(static_cast<size_t>(rowsToShow));
		for (uint64_t i = 0; i < rowsToShow; ++i) {
			uint64_t row = rowOffset_ + i;
			std::vector<uint8_t> raw = session_.bytes(row * 16, 16);
			lines.push_back(ftxui::text(formatHexRow(row, raw)));
		}

		return ftxui::window(title, ftxui::vbox(std::move(lines)) | ftxui::yframe);
	}

private:
	static constexpr uint64_t kVisibleRows = 40;
	static constexpr uint64_t kPageRows = 16;

	Session& session_;
	uint64_t rowOffset_ = 0;
};

} // namespace

ftxui::Component MemoryPane(Session& session) {
	return ftxui::Make<MemoryPaneImpl>(session);
}

} // namespace tui
