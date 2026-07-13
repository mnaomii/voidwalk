#include "main_window.h"

#include "../dialogs/settings_dialog.h"
#include "../panes/ai_chat_pane.h"
#include "../panes/disassembly_pane.h"
#include "../panes/memory_pane.h"
#include "../panes/registers_pane.h"
#include "../panes/stack_pane.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>

namespace gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle(tr("dat-gui — dynamic analysis tool"));
	resize(1200, 800);

	settings_ = AppSettings::load();

	disasm_ = new DisassemblyPane(this);
	disasm_->setSession(&session_);
	setCentralWidget(disasm_);
	connect(disasm_, &DisassemblyPane::editsChanged, this, &MainWindow::onEditsChanged);

	buildActions();
	buildToolBar();
	buildMenus();
	buildDocks();

	archLabel_ = new QLabel(this);
	statusBar()->addPermanentWidget(archLabel_);

	applyAiVisibility();
	refreshAll();
	setStatus(tr("Ready — open a PE/ELF binary to begin."));
}

void MainWindow::buildActions() {
	const QStyle* s = style();

	openAct_ = new QAction(s->standardIcon(QStyle::SP_DialogOpenButton), tr("&Open…"), this);
	openAct_->setShortcut(QKeySequence::Open);
	connect(openAct_, &QAction::triggered, this, &MainWindow::onOpen);

	runAct_ = new QAction(s->standardIcon(QStyle::SP_MediaPlay), tr("&Run"), this);
	runAct_->setShortcut(Qt::Key_F5);
	connect(runAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	stepAct_ = new QAction(s->standardIcon(QStyle::SP_ArrowDown), tr("Step &Into"), this);
	stepAct_->setShortcut(Qt::Key_F7);
	connect(stepAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	stepOverAct_ = new QAction(s->standardIcon(QStyle::SP_ArrowForward), tr("Step &Over"), this);
	stepOverAct_->setShortcut(Qt::Key_F8);
	connect(stepOverAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	continueAct_ = new QAction(s->standardIcon(QStyle::SP_MediaSeekForward), tr("&Continue"), this);
	continueAct_->setShortcut(Qt::Key_F9);
	connect(continueAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	pauseAct_ = new QAction(s->standardIcon(QStyle::SP_MediaPause), tr("&Pause"), this);
	connect(pauseAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	resetAct_ = new QAction(s->standardIcon(QStyle::SP_MediaStop), tr("&Reset"), this);
	connect(resetAct_, &QAction::triggered, this, &MainWindow::onDebugStub);

	recompileAct_ = new QAction(s->standardIcon(QStyle::SP_BrowserReload), tr("Re&compile"), this);
	recompileAct_->setToolTip(tr("Reassemble the edited instructions"));
	recompileAct_->setEnabled(false); // enabled once an instruction is edited
	connect(recompileAct_, &QAction::triggered, this, &MainWindow::onRecompile);

	settingsAct_ = new QAction(s->standardIcon(QStyle::SP_FileDialogDetailedView), tr("&Settings…"), this);
	connect(settingsAct_, &QAction::triggered, this, &MainWindow::onSettings);
}

void MainWindow::buildToolBar() {
	auto* tb = addToolBar(tr("Main"));
	tb->setObjectName(QStringLiteral("MainToolBar"));
	tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
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
	tb->addSeparator();
	tb->addAction(settingsAct_);
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
	chatDock_ = new QDockWidget(tr("AI Assistant"), this);
	chatDock_->setObjectName(QStringLiteral("ChatDock"));
	chatDock_->setWidget(chat_);
	addDockWidget(Qt::RightDockWidgetArea, chatDock_);

	// Stack the right-hand docks as tabs to save space.
	tabifyDockWidget(registersDock_, stackDock_);
	tabifyDockWidget(stackDock_, chatDock_);
	registersDock_->raise();

	// Populate the View menu with the docks' toggle actions.
	for (QMenu* m : menuBar()->findChildren<QMenu*>()) {
		if (m->title() == tr("&View")) {
			m->addAction(registersDock_->toggleViewAction());
			m->addAction(stackDock_->toggleViewAction());
			m->addAction(memoryDock_->toggleViewAction());
			m->addAction(chatDock_->toggleViewAction());
			break;
		}
	}
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

void MainWindow::onRecompile() {
	const auto edits = disasm_->pendingEdits();
	const std::string result = session_.applyPatches(edits);
	setStatus(QString::fromStdString(result));
	// Assembler backend is a stub: edits stay pending, so leave the action live.
}

void MainWindow::onSettings() {
	SettingsDialog dlg(settings_, this);
	if (dlg.exec() == QDialog::Accepted) {
		settings_ = dlg.settings();
		settings_.save();
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

void MainWindow::refreshAll() {
	disasm_->refresh();
	registers_->refresh();
	stack_->refresh();
	memory_->refresh();

	if (session_.loaded()) {
		archLabel_->setText(tr("%1 · %2")
			.arg(QString::fromStdString(session_.format()),
			     QString::fromStdString(session_.architecture())));
		setWindowTitle(tr("dat-gui — %1").arg(QString::fromStdString(session_.filePath())));
	} else {
		archLabel_->clear();
		setWindowTitle(tr("dat-gui — dynamic analysis tool"));
	}
}

void MainWindow::setStatus(const QString& msg) {
	statusBar()->showMessage(msg);
}

} // namespace gui
