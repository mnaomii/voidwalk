#ifndef GUI_STACK_PANE_H
#define GUI_STACK_PANE_H

#include "../model/gui_session.h"

#include <QWidget>

class QTableWidget;
class QLabel;

namespace gui {

// Simulated stack view. Reads Session::stack() (the core's virtStack, empty
// until execution exists). Renders top-of-stack first with an <- esp marker on
// the current top; shows a placeholder notice while empty.
class StackPane : public QWidget {
	Q_OBJECT
public:
	explicit StackPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }

public slots:
	void refresh();

private:
	Session* session_ = nullptr;
	QTableWidget* table_ = nullptr;
	QLabel* placeholder_ = nullptr;
};

} // namespace gui

#endif
