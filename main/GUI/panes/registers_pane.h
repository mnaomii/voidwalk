#ifndef GUI_REGISTERS_PANE_H
#define GUI_REGISTERS_PANE_H

#include "../model/gui_session.h"

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

	void setSession(Session* s) { session_ = s; }

public slots:
	void refresh();

private:
	Session* session_ = nullptr;
	QTreeWidget* tree_ = nullptr;
};

} // namespace gui

#endif
