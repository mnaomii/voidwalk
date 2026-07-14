#ifndef GUI_MAIN_WINDOW_H
#define GUI_MAIN_WINDOW_H

#include "../dialogs/settings.h"
#include "../model/gui_session.h"

#include <QMainWindow>

class QAction;
class QDockWidget;
class QLabel;
class QStackedWidget;

namespace gui {

class DisassemblyPane;
class RegistersPane;
class StackPane;
class MemoryPane;
class AiChatPane;
class WelcomeWidget;

// Top-level window for voidwalk-gui. Owns the Session view-model and the panes,
// wires the toolbar/menus, and fans a single refreshAll() out to every pane
// after any state change (Open today; debugger steps later).
//
// Appearance: applyTheme() (Fusion + palette + QSS via ThemeManager) runs at
// startup and whenever the theme changes in Settings. Icons come from the
// recolorable SVG set in :/icons (theme/icons.h). The central widget is a
// stack: WelcomeWidget until a binary loads, DisassemblyPane after.
//
// Action reality check: Open, Recompile, Settings, and the AI-pane toggle are
// real. The debugger transport (Run/Step/Step Over/Continue/Pause/Reset) does
// not exist yet, so those actions report "not implemented yet" in the status
// bar — they're wired and discoverable, waiting on the execution engine.
class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	// Loads a binary at startup (e.g. from argv). Safe to call with an empty
	// path (no-op). Errors surface in the status bar, not a dialog.
	void openPath(const QString& path);

private slots:
	void onOpen();
	void onRecompile();
	void onSettings();
	void onDebugStub();       // shared handler for the not-yet-implemented actions
	void onEditsChanged();

private:
	void buildActions();
	void buildToolBar();
	void buildMenus();
	void buildDocks();
	void applyTheme();        // Fusion + palette + QSS + icon recolor
	void applyAiVisibility(); // show/hide the AI dock per settings_.aiEnabled
	void refreshAll();
	void setStatus(const QString& msg);

	Session session_;
	AppSettings settings_;

	QStackedWidget* central_ = nullptr;
	WelcomeWidget* welcome_ = nullptr;
	DisassemblyPane* disasm_ = nullptr;
	RegistersPane* registers_ = nullptr;
	StackPane* stack_ = nullptr;
	MemoryPane* memory_ = nullptr;
	AiChatPane* chat_ = nullptr;

	QDockWidget* registersDock_ = nullptr;
	QDockWidget* stackDock_ = nullptr;
	QDockWidget* memoryDock_ = nullptr;
	QDockWidget* chatDock_ = nullptr;

	QAction* openAct_ = nullptr;
	QAction* runAct_ = nullptr;
	QAction* stepAct_ = nullptr;
	QAction* stepOverAct_ = nullptr;
	QAction* continueAct_ = nullptr;
	QAction* pauseAct_ = nullptr;
	QAction* resetAct_ = nullptr;
	QAction* recompileAct_ = nullptr;
	QAction* settingsAct_ = nullptr;

	QLabel* archLabel_ = nullptr; // permanent status-bar widget: format/arch
};

} // namespace gui

#endif
