#pragma once
#include "gui/session.hpp"

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

	// Borrows `s`; the caller keeps ownership and must outlive this pane.
	void setSession(Session* s) { session_ = s; }

public slots:
	// Rebuilds the table from Session::stack(). O(stack depth), which is 0 today.
	// Called from MainWindow::refreshAll().
	void refresh();

private:
	Session* session_ = nullptr;
	QTableWidget* table_ = nullptr;
	QLabel* placeholder_ = nullptr;
};

} // namespace gui

