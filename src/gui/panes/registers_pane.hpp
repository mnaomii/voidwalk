#pragma once
#include "gui/session.hpp"

#include <QWidget>

class QTreeWidget;

namespace gui {

// Register file view. Reads Session::registers() (the core's emulated Registers
// struct — all zero until the debugger exists; real plumbing, placeholder
// semantics). Registers are grouped by category under collapsible headers —
// General Purpose, Instruction Pointer, Segment, Flags. When a 64-bit target is
// loaded (Session::is64bit()) the general-purpose set and instruction pointer are
// shown under their 64-bit names (rax/rbx…/rip) instead of the 32-bit ones, and
// their values widen to 16 hex digits.
class RegistersPane : public QWidget {
	Q_OBJECT
public:
	explicit RegistersPane(QWidget* parent = nullptr);

	// Borrows `s`; the caller keeps ownership and must outlive this pane.
	void setSession(Session* s) { session_ = s; }

public slots:
	// Rebuilds the tree from Session::registers(), re-choosing 32- vs 64-bit names
	// from Session::is64bit(). Fixed cost - the register set is a constant size.
	// Called from MainWindow::refreshAll().
	void refresh();

private:
	Session* session_ = nullptr;
	QTreeWidget* tree_ = nullptr;
};

} // namespace gui

