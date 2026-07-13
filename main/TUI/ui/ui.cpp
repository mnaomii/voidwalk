#include "ui.h"

#include "../panes/panes.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

#include <string>
#include <utility>

namespace tui {

using namespace ftxui;

UI::UI(Session session) : session_(std::move(session)) {}

int UI::start() {
	auto screen = ScreenInteractive::Fullscreen();

	// Modal state. showOpen drives the Modal decorator; pathInput backs the
	// dialog's Input. Both outlive the loop (locals of start()).
	bool showOpen = false;
	std::string pathInput;

	// --- top bar ---------------------------------------------------------
	// Open is the only real action; the rest are debugger placeholders that
	// stay as distinct buttons so the wiring has an obvious home later.
	auto btnOpen = Button("Open", [&] { showOpen = true; }, ButtonOption::Ascii());
	auto btnSave = Button("Save", [&] { session_.setStatus("Save: not implemented yet."); }, ButtonOption::Ascii());
	auto btnRun = Button("Run", [&] { session_.setStatus("Run: not implemented yet."); }, ButtonOption::Ascii());
	auto btnStep = Button("Step", [&] { session_.setStatus("Step: not implemented yet."); }, ButtonOption::Ascii());
	auto btnBreak = Button("Break", [&] { session_.setStatus("Break: not implemented yet."); }, ButtonOption::Ascii());
	auto btnQuit = Button("Quit", screen.ExitLoopClosure(), ButtonOption::Ascii());

	auto topBar = Container::Horizontal({
		btnOpen, btnSave, btnRun, btnStep, btnBreak, btnQuit,
	});

	// --- panes -----------------------------------------------------------
	auto disasm = DisassemblyPane(session_);
	auto registers = RegistersPane(session_);
	auto stack = StackPane(session_);
	auto memory = MemoryPane(session_);

	// Split sizes are in cells for the split's first argument; kept mutable so
	// dragging a separator persists across frames.
	int leftSize = 72; // disassembly width
	int regSize = 12;  // registers height (remainder of right column = stack)
	int memSize = 12;  // memory strip height

	auto right = ResizableSplitTop(registers, stack, &regSize);
	auto upper = ResizableSplitLeft(disasm, right, &leftSize);
	auto body = ResizableSplitBottom(memory, upper, &memSize);

	// topBar + body share the container so Tab/arrows route focus between them.
	auto layout = Container::Vertical({ topBar, body });

	// Whole-screen frame: top bar, body, then the (non-focusable) status bar,
	// re-read from the Session every frame.
	auto mainComponent = Renderer(layout, [&] {
		std::string path = session_.filePath();
		if (path.empty()) path = "-";
		std::string fmt = session_.format();
		if (fmt.empty()) fmt = "-";
		std::string arch = session_.architecture();
		if (arch.empty()) arch = "-";

		auto statusBar = hbox({
			text(" " + path + " ") | bold,
			separator(),
			text(" " + fmt + " "),
			separator(),
			text(" " + arch + " "),
			separator(),
			text(" " + session_.status() + " "),
			filler(),
		}) | inverted;

		return vbox({
			topBar->Render(),
			separator(),
			body->Render() | yflex,
			statusBar,
		});
	});

	// --- Open modal ------------------------------------------------------
	auto pathInputComp = Input(&pathInput, "path to binary");

	auto btnLoad = Button("Load", [&] {
		// open() sets status and refreshes internally; only close on success.
		if (session_.open(pathInput))
			showOpen = false;
	}, ButtonOption::Ascii());
	auto btnCancel = Button("Cancel", [&] { showOpen = false; }, ButtonOption::Ascii());

	auto openControls = Container::Vertical({
		pathInputComp,
		Container::Horizontal({ btnLoad, btnCancel }),
	});

	auto openDialog = Renderer(openControls, [&] {
		return vbox({
			text("Open binary") | bold,
			separator(),
			hbox({ text("Path: "), pathInputComp->Render() | flex }),
			separator(),
			hbox({ btnLoad->Render(), text("  "), btnCancel->Render() }),
		}) | border | size(WIDTH, GREATER_THAN, 50);
	});

	// Escape cancels the dialog (dom convention: escape closes a modal).
	openDialog |= CatchEvent([&](Event e) {
		if (e == Event::Escape) {
			showOpen = false;
			return true;
		}
		return false;
	});

	// Global shortcuts, but only while the modal is closed so the path Input
	// keeps its keys ('o'/'q' would otherwise be swallowed mid-typing).
	mainComponent |= CatchEvent([&](Event e) {
		if (showOpen)
			return false;
		if (e == Event::Character('q')) {
			screen.ExitLoopClosure()();
			return true;
		}
		if (e == Event::Character('o')) {
			showOpen = true;
			return true;
		}
		return false;
	});

	mainComponent |= Modal(openDialog, &showOpen);

	screen.Loop(mainComponent);
	return 0;
}

} // namespace tui
