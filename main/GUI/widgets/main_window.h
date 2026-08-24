#ifndef GUI_MAIN_WINDOW_H
#define GUI_MAIN_WINDOW_H

#include "../dialogs/settings.h"
#include "../model/gui_session.h"

#include <QMainWindow>
#include <cstdint>

class QAction;
class QDockWidget;
class QLabel;
class QLineEdit;
class QStackedWidget;

namespace gui {

class DisassemblyPane;
class RegistersPane;
class StackPane;
class MemoryPane;
class AiChatPane;
class SymbolsPane;
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
// Layout: the symbol sidebar is a left dock (toggle at the head of the toolbar
// and in the View menu, Ctrl+B), so navigation is a visible list rather than a
// dialog; the toolbar keeps a narrow field for raw addresses only. Registers /
// Stack / AI stay tabbed on the right, Memory across the bottom.
//
// Action reality check: Open, Recompile, Settings, symbol navigation and the
// AI-pane toggle are real. The debugger transport (Run/Step/Step Over/Continue/
// Pause/Reset) does not exist yet, so those actions report "not implemented
// yet" in the status bar — wired and discoverable, waiting on the engine.
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
	void onGotoSubmitted();   // toolbar address field -> disasm_->navigateTo()

private:
	void buildActions();
	void buildToolBar();
	void buildMenus();
	void buildDocks();
	void applyTheme();        // Fusion + palette + QSS + icon recolor
	void applyAiVisibility(); // show/hide the AI dock per settings_.aiEnabled
	void refreshAll();
	void setStatus(const QString& msg);
	void setCounts();         // status-bar symbol/instruction counters

	Session session_;
	AppSettings settings_;

	QStackedWidget* central_ = nullptr;
	WelcomeWidget* welcome_ = nullptr;
	DisassemblyPane* disasm_ = nullptr;
	RegistersPane* registers_ = nullptr;
	StackPane* stack_ = nullptr;
	MemoryPane* memory_ = nullptr;
	AiChatPane* chat_ = nullptr;
	SymbolsPane* symbols_ = nullptr;

	QDockWidget* symbolsDock_ = nullptr;
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
	QAction* sidebarAct_ = nullptr; // checkable: shows/hides symbolsDock_

	QLineEdit* gotoField_ = nullptr;
	QLabel* countLabel_ = nullptr; // "N instr"
	QLabel* archLabel_ = nullptr;  // format/arch
};

} // namespace gui

#endif
