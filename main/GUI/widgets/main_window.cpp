#include "main_window.h"

#include "../dialogs/settings_dialog.h"
#include "../panes/ai_chat_pane.h"
#include "../panes/disassembly_pane.h"
#include "../panes/memory_pane.h"
#include "../panes/registers_pane.h"
#include "../panes/stack_pane.h"
#include "../panes/symbols_pane.h"
#include "../theme/icons.h"
#include "../theme/theme_manager.h"
#include "welcome_widget.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

namespace gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle(tr("voidwalk"));
	resize(1200, 800);
	setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);

	settings_ = AppSettings::load();
	applyTheme(); // style/palette/QSS + icon colors, before any widget paints

	disasm_ = new DisassemblyPane(this);
	disasm_->setSession(&session_);
	connect(disasm_, &DisassemblyPane::editsChanged, this, &MainWindow::onEditsChanged);

	welcome_ = new WelcomeWidget(this);
	connect(welcome_, &WelcomeWidget::openRequested, this, &MainWindow::onOpen);
	connect(welcome_, &WelcomeWidget::fileDropped, this, &MainWindow::openPath);

	central_ = new QStackedWidget(this);
	central_->addWidget(welcome_);
	central_->addWidget(disasm_);
	setCentralWidget(central_);

	buildActions();
	buildToolBar();
	buildMenus();
	buildDocks();

	countLabel_ = new QLabel(this);
	archLabel_ = new QLabel(this);
	archLabel_->setObjectName(QStringLiteral("archLabel"));
	statusBar()->addPermanentWidget(countLabel_);
	statusBar()->addPermanentWidget(archLabel_);
	statusBar()->setSizeGripEnabled(false);

	disasm_->setTheme(ThemeManager::current()); // delegate colors (created after applyTheme)
	symbols_->setTheme(ThemeManager::current());

	applyAiVisibility();
	refreshAll();
	setStatus(tr("Ready — open a PE/ELF binary to begin."));
}

void MainWindow::buildActions() {
	openAct_ = new QAction(Icons::icon(QStringLiteral("open")), tr("&Open…"), this);
	openAct_->setShortcut(QKeySequence::Open);
	connect(openAct_, &QAction::triggered, this, &MainWindow::onOpen);

	runAct_ = new QAction(Icons::icon(QStringLiteral("run")), tr("&Run"), this);
	runAct_->setShortcut(Qt::Key_F5);
	connect(runAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	stepAct_ = new QAction(Icons::icon(QStringLiteral("step-into")), tr("Step &Into"), this);
	stepAct_->setShortcut(Qt::Key_F7);
	connect(stepAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	stepOverAct_ = new QAction(Icons::icon(QStringLiteral("step-over")), tr("Step &Over"), this);
	stepOverAct_->setShortcut(Qt::Key_F8);
	connect(stepOverAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	continueAct_ = new QAction(Icons::icon(QStringLiteral("continue")), tr("&Continue"), this);
	continueAct_->setShortcut(Qt::Key_F9);
	connect(continueAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	pauseAct_ = new QAction(Icons::icon(QStringLiteral("pause")), tr("&Pause"), this);
	connect(pauseAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	resetAct_ = new QAction(Icons::icon(QStringLiteral("reset")), tr("&Reset"), this);
	connect(resetAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	recompileAct_ = new QAction(Icons::icon(QStringLiteral("recompile")), tr("Re&compile"), this);
	recompileAct_->setToolTip(tr("Reassemble the edited instructions"));
	recompileAct_->setEnabled(false); // enabled once an instruction is edited
	connect(recompileAct_, &QAction::triggered, this, &MainWindow::onRecompile);

	settingsAct_ = new QAction(Icons::icon(QStringLiteral("settings")), tr("&Settings…"), this);
	connect(settingsAct_, &QAction::triggered, this, &MainWindow::onSettings);

	// Sidebar toggle. Checkable so the flat toolbar button can show state — with
	// no box at rest, "checked" is the only affordance telling you the pane is
	// open. Wired to the dock in buildDocks().
	sidebarAct_ = new QAction(ThemeManager::sidebarIcon(ThemeManager::current()),
	                          tr("Symbol &Sidebar"), this);
	sidebarAct_->setCheckable(true);
	sidebarAct_->setChecked(true);
	sidebarAct_->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
	sidebarAct_->setToolTip(tr("Symbol sidebar — Ctrl+B"));
}

void MainWindow::buildToolBar() {
	auto* tb = addToolBar(tr("Main"));
	tb->setObjectName(QStringLiteral("MainToolBar"));
	tb->setMovable(false);
	tb->setIconSize(QSize(16, 16));

	// Open and Run carry text; the rest are compact icon buttons with tooltips
	// (the shortcut shows in the tooltip and the Debug menu spells them out).
	tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
	tb->addAction(sidebarAct_);
	tb->addSeparator();
	tb->addAction(openAct_);
	tb->addSeparator();
	tb->addAction(runAct_);
	tb->addAction(stepAct_);
	tb->addAction(stepOverAct_);
	tb->addAction(continueAct_);
	tb->addAction(pauseAct_);
	tb->addAction(resetAct_);
	tb->addSeparator();
	tb->addAction(recompileAct_);

	auto* spacer = new QWidget(tb);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	tb->addWidget(spacer);

	// Raw addresses only — symbol lookup is the sidebar's filter box, so this
	// field no longer has to be a general-purpose search and can stay narrow.
	gotoField_ = new QLineEdit(tb);
	gotoField_->setObjectName(QStringLiteral("gotoField"));
	gotoField_->setPlaceholderText(tr("Go to address"));
	gotoField_->setClearButtonEnabled(true);
	gotoField_->setMaximumWidth(200);
	connect(gotoField_, &QLineEdit::returnPressed, this, &MainWindow::onGotoSubmitted);
	tb->addWidget(gotoField_);

	tb->addAction(settingsAct_);

	// Text on the two primary actions; QSS objectNames pick up the emphasis.
	if (auto* openBtn = qobject_cast<QToolButton*>(tb->widgetForAction(openAct_)))
		openBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	if (auto* runBtn = qobject_cast<QToolButton*>(tb->widgetForAction(runAct_))) {
		runBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		runBtn->setObjectName(QStringLiteral("runButton"));
	}
	if (auto* gear = qobject_cast<QToolButton*>(tb->widgetForAction(settingsAct_)))
		gear->setObjectName(QStringLiteral("settingsButton"));
	if (auto* side = qobject_cast<QToolButton*>(tb->widgetForAction(sidebarAct_)))
		side->setObjectName(QStringLiteral("sidebarButton"));
}

void MainWindow::buildMenus() {
	auto* file = menuBar()->addMenu(tr("&File"));
	file->addAction(openAct_);
	file->addSeparator();
	auto* quit = file->addAction(tr("&Quit"));
	quit->setShortcut(QKeySequence::Quit);
	connect(quit, &QAction::triggered, this, &QWidget::close);

	auto* debug = menuBar()->addMenu(tr("&Debug"));
	debug->addAction(runAct_);
	debug->addAction(stepAct_);
	debug->addAction(stepOverAct_);
	debug->addAction(continueAct_);
	debug->addAction(pauseAct_);
	debug->addAction(resetAct_);

	auto* edit = menuBar()->addMenu(tr("&Edit"));
	edit->addAction(recompileAct_);

	// View menu gets each dock's built-in toggle action after buildDocks().
	menuBar()->addMenu(tr("&View"));

	auto* tools = menuBar()->addMenu(tr("&Tools"));
	tools->addAction(settingsAct_);
}

void MainWindow::buildDocks() {
	// --- left: symbol sidebar ------------------------------------------------
	symbols_ = new SymbolsPane(this);
	symbols_->setSession(&session_);
	symbolsDock_ = new QDockWidget(tr("Symbols"), this);
	symbolsDock_->setObjectName(QStringLiteral("SymbolsDock"));
	symbolsDock_->setWidget(symbols_);
	// The pane draws its own header, so the dock title bar would repeat it.
	symbolsDock_->setTitleBarWidget(new QWidget(symbolsDock_));
	symbolsDock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
	addDockWidget(Qt::LeftDockWidgetArea, symbolsDock_);
	resizeDocks({symbolsDock_}, {208}, Qt::Horizontal);

	connect(symbols_, &SymbolsPane::navigateRequested, this, [this](uint64_t vaddr) {
		disasm_->navigateTo(vaddr);
		setStatus(tr("Jumped to 0x%1").arg(vaddr, 8, 16, QLatin1Char('0')));
	});
	// Two-way binding: the toolbar button drives the dock, closing the dock (or
	// the View-menu entry) un-checks the button.
	connect(sidebarAct_, &QAction::toggled, symbolsDock_, &QWidget::setVisible);
	connect(symbolsDock_, &QDockWidget::visibilityChanged, sidebarAct_, [this](bool visible) {
		if (sidebarAct_->isChecked() != visible)
			sidebarAct_->setChecked(visible);
	});

	// --- right: registers / stack / AI, tabbed -------------------------------
	registers_ = new RegistersPane(this);
	registers_->setSession(&session_);
	registersDock_ = new QDockWidget(tr("Registers"), this);
	registersDock_->setObjectName(QStringLiteral("RegistersDock"));
	registersDock_->setWidget(registers_);
	addDockWidget(Qt::RightDockWidgetArea, registersDock_);

	stack_ = new StackPane(this);
	stack_->setSession(&session_);
	stackDock_ = new QDockWidget(tr("Stack"), this);
	stackDock_->setObjectName(QStringLiteral("StackDock"));
	stackDock_->setWidget(stack_);
	addDockWidget(Qt::RightDockWidgetArea, stackDock_);

	memory_ = new MemoryPane(this);
	memory_->setSession(&session_);
	memoryDock_ = new QDockWidget(tr("Memory"), this);
	memoryDock_->setObjectName(QStringLiteral("MemoryDock"));
	memoryDock_->setWidget(memory_);
	addDockWidget(Qt::BottomDockWidgetArea, memoryDock_);

	chat_ = new AiChatPane(this);
	chat_->setSession(&session_);
	chat_->setSettings(settings_);
	chatDock_ = new QDockWidget(tr("Assistant"), this);
	chatDock_->setObjectName(QStringLiteral("ChatDock"));
	chatDock_->setWidget(chat_);
	addDockWidget(Qt::RightDockWidgetArea, chatDock_);

	// Stack the right-hand docks as tabs to save space; tabs read better on top.
	setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
	tabifyDockWidget(registersDock_, stackDock_);
	tabifyDockWidget(stackDock_, chatDock_);
	registersDock_->raise();
	resizeDocks({registersDock_}, {300}, Qt::Horizontal);
	resizeDocks({memoryDock_}, {214}, Qt::Vertical);

	// The tab bar already names the active pane, so the dock title bar would
	// repeat it in a redundant strip right underneath. Drop that strip by giving
	// each tabified dock an empty title bar widget. Done for the whole tab group
	// (not just Registers/Stack) so the content doesn't shift when switching to a
	// tab that still had one. Panels remain toggleable from the View menu.
	for (QDockWidget* d : {registersDock_, stackDock_, chatDock_})
		d->setTitleBarWidget(new QWidget(d));

	// Populate the View menu with the docks' toggle actions.
	for (QMenu* m : menuBar()->findChildren<QMenu*>()) {
		if (m->title() == tr("&View")) {
			m->addAction(sidebarAct_);
			m->addSeparator();
			m->addAction(registersDock_->toggleViewAction());
			m->addAction(stackDock_->toggleViewAction());
			m->addAction(memoryDock_->toggleViewAction());
			m->addAction(chatDock_->toggleViewAction());
			break;
		}
	}
}

void MainWindow::applyTheme() {
	const Theme theme = Theme::byId(settings_.theme);
	ThemeManager::apply(theme);
	Icons::setColors(theme.textMuted, theme.textGhost);

	// Re-fetch icons: the cache was invalidated with the new colors.
	if (openAct_) {
		openAct_->setIcon(Icons::icon(QStringLiteral("open")));
		runAct_->setIcon(Icons::icon(QStringLiteral("run")));
		stepAct_->setIcon(Icons::icon(QStringLiteral("step-into")));
		stepOverAct_->setIcon(Icons::icon(QStringLiteral("step-over")));
		continueAct_->setIcon(Icons::icon(QStringLiteral("continue")));
		pauseAct_->setIcon(Icons::icon(QStringLiteral("pause")));
		resetAct_->setIcon(Icons::icon(QStringLiteral("reset")));
		recompileAct_->setIcon(Icons::icon(QStringLiteral("recompile")));
		settingsAct_->setIcon(Icons::icon(QStringLiteral("settings")));
		sidebarAct_->setIcon(ThemeManager::sidebarIcon(theme));
	}
	if (disasm_)
		disasm_->setTheme(theme);
	if (symbols_)
		symbols_->setTheme(theme);
}

void MainWindow::applyAiVisibility() {
	// The AI pane is opt-in: hidden entirely unless enabled in Settings.
	chat_->setSettings(settings_);
	chatDock_->setVisible(settings_.aiEnabled);
	chatDock_->toggleViewAction()->setEnabled(settings_.aiEnabled);
	if (settings_.aiEnabled)
		chatDock_->raise();
}

void MainWindow::openPath(const QString& path) {
	if (path.isEmpty()) return;
	if (session_.open(path.toStdString())) {
		disasm_->clearPendingEdits();
		recompileAct_->setEnabled(false);
	}
	refreshAll();
	setStatus(QString::fromStdString(session_.status()));
}

void MainWindow::onOpen() {
	const QString path = QFileDialog::getOpenFileName(
		this, tr("Open binary"), QString(),
		tr("Executables (*.exe *.dll *.so *.elf *.bin);;All files (*)"));
	openPath(path);
}

void MainWindow::onGotoSubmitted() {
	const QString raw = gotoField_->text().trimmed();
	if (raw.isEmpty()) return;
	bool ok = false;
	// Accept "401000", "0x401000" and "0X401000" — hex either way; a decimal
	// address is never what someone types into a disassembler.
	const QString digits = raw.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)
		? raw.mid(2) : raw;
	const qulonglong vaddr = digits.toULongLong(&ok, 16);
	if (!ok) {
		setStatus(tr("\"%1\" is not a hex address.").arg(raw));
		return;
	}
	disasm_->navigateTo(vaddr);
	gotoField_->clearFocus();
}

void MainWindow::onRecompile() {
	const auto edits = disasm_->pendingEdits();
	const std::string result = session_.applyPatches(edits);
	setStatus(QString::fromStdString(result));
	// Assembler backend is a stub: edits stay pending, so leave the action live.
}

void MainWindow::onSettings() {
	SettingsDialog dlg(settings_, this);
	if (dlg.exec() == QDialog::Accepted) {
		const QString oldTheme = settings_.theme;
		settings_ = dlg.settings();
		settings_.save();
		if (settings_.theme != oldTheme)
			applyTheme();
		applyAiVisibility();
		setStatus(tr("Settings saved."));
	}
}

void MainWindow::onDebugStub() {
	auto* act = qobject_cast<QAction*>(sender());
	const QString name = act ? act->text().remove(QLatin1Char('&')) : tr("That action");
	setStatus(tr("%1: not implemented yet — the execution engine is WIP.").arg(name));
}

void MainWindow::onEditsChanged() {
	recompileAct_->setEnabled(disasm_->hasPendingEdits());
}

void MainWindow::setCounts() {
	if (!session_.loaded()) {
		countLabel_->clear();
		return;
	}
	countLabel_->setText(tr("%n instr", "", static_cast<int>(session_.disassembly().size())));
}

void MainWindow::refreshAll() {
	disasm_->refresh();
	symbols_->refresh();
	registers_->refresh();
	stack_->refresh();
	memory_->refresh();

	central_->setCurrentWidget(session_.loaded()
		? static_cast<QWidget*>(disasm_) : static_cast<QWidget*>(welcome_));

	// Nothing to navigate before a binary loads: hide the sidebar and disable the
	// address field rather than presenting two empty controls on the welcome
	// screen. Both come back on their own when a file opens.
	const bool loaded = session_.loaded();
	gotoField_->setEnabled(loaded);
	if (!loaded && symbolsDock_->isVisible())
		symbolsDock_->setVisible(false);
	else if (loaded && sidebarAct_->isChecked())
		symbolsDock_->setVisible(true);

	setCounts();

	if (loaded) {
		archLabel_->setText(tr("%1 · %2")
			.arg(QString::fromStdString(session_.format()),
			     QString::fromStdString(session_.architecture())));
		setWindowTitle(tr("voidwalk — %1").arg(QString::fromStdString(session_.filePath())));
	} else {
		archLabel_->clear();
		setWindowTitle(tr("voidwalk"));
	}
}

void MainWindow::setStatus(const QString& msg) {
	statusBar()->showMessage(msg);
}

} // namespace gui
