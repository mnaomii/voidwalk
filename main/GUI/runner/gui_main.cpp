#include "gui_main.h"
#include "../theme/theme.h"
#include "../widgets/main_window.h"

#include <QApplication>

#ifdef _WIN32
#include <windows.h>
#endif

// GUI entry point. Invoked from main/main.cpp's dispatcher for the "--gui" mode
// (guarded by VOIDWALK_WITH_GUI) rather than being a main() of its own, so the
// GUI compiles into the single bundled voidwalk binary next to the TUI and test
// harness. A binary path may be passed as the last argument to open on startup.
int GUIstart(int argc, char** argv) {
#ifdef _WIN32
	// The bundled binary is a console subsystem app so --ui/--run-tests get a
	// console. That would otherwise leave a stray console window sitting behind
	// the GUI, so detach from it here on the --gui path. Harmless when launched
	// from an existing terminal (that console belongs to the parent shell).
	FreeConsole();
#endif

	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("voidwalk-gui"));
	QApplication::setOrganizationName(QStringLiteral("voidwalk"));

	// Register the bundled JetBrains Mono (:/fonts) before any widget exists, so
	// monoFont() resolves to it. A no-op when the font resource isn't compiled in
	// (the TTFs aren't shipped) — monoFont() then falls back to host mono fonts.
	gui::registerBundledFonts();

	gui::MainWindow window;
	window.show();

	// In the bundled binary argv[1] is the "--gui" mode flag, so a real file
	// path is only present when there is an argument beyond it (argc > 2).
	if (argc > 2)
		window.openPath(QString::fromLocal8Bit(argv[argc - 1]));

	return app.exec();
}
