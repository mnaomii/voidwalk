#ifndef GUI_REGISTERS_PANE_H
#define GUI_REGISTERS_PANE_H

#include "../model/gui_session.h"

#include <QWidget>

class QTableWidget;

namespace gui {

// Register file view. Reads Session::registers() (the core's emulated Registers
// struct — all zero until the debugger exists; real plumbing, placeholder
// semantics). Shows the IA-32 general-purpose set plus eip, the segment
// registers, and flags.
class RegistersPane : public QWidget {
	Q_OBJECT
public:
	explicit RegistersPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }

public slots:
	void refresh();

private:
	Session* session_ = nullptr;
	QTableWidget* table_ = nullptr;
};

} // namespace gui

#endif
