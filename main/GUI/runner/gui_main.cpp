#include "../widgets/main_window.h"

#include <QApplication>

// Entry point for the voidwalk-gui executable. Kept separate from the shared
// main/main.cpp (which routes the TUI/test harness) so the GUI project builds
// standalone with no VOIDWALK_WITH_TUI / test dependencies. A binary path may be
// passed as the last argument to open it on startup.
int main(int argc, char** argv) {
	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("voidwalk-gui"));
	QApplication::setOrganizationName(QStringLiteral("voidwalk"));

	gui::MainWindow window;
	window.show();

	if (argc > 1)
		window.openPath(QString::fromLocal8Bit(argv[argc - 1]));

	return app.exec();
}
