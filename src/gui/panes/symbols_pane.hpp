#pragma once
#include "gui/session.hpp"
#include "gui/symbols.hpp"
#include "gui/theme/theme.hpp"

#include <QWidget>
#include <cstdint>
#include <vector>

class QAction;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace gui {

// Symbol sidebar: FUNCTIONS / IMPORTS / STRINGS, each a collapsible group with
// name on the left and address (or module) on the right. Selecting a function
// emits navigateRequested(vaddr), which MainWindow forwards to the disassembly
// pane; selecting a string emits memoryRequested(fileOffset) for the memory pane.
// That is why the toolbar's field can shrink to raw addresses only — symbol
// lookup lives here, in the filter box, instead of behind a dialog.
//
// Contents come from collectSymbols() (model/symbols.h), so the pane needs no
// core API of its own and empty groups simply don't render.
//
// The scan runs synchronously on the UI thread. It used to run on a worker, but
// that made it a second reader of the core's instruction vectors, concurrent with
// the UI thread — which blocks any post-decode compaction of those vectors (a
// reallocation under a live reader is exactly the crash the reserve in decode()
// exists to prevent). Running it here means that when refresh() returns, nothing
// is reading the core but this thread.
//
// The cost is real: the scan walks every decoded row and formats a string for
// each, so on a large binary refresh() blocks the UI for as long as that takes.
// refresh() is called from MainWindow::refreshAll() on the decode's final tick.
class SymbolsPane : public QWidget {
	Q_OBJECT
public:
	explicit SymbolsPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }
	void setTheme(const Theme& theme); // recolors the group headers / string rows

public slots:
	void refresh();

signals:
	void navigateRequested(uint64_t vaddr);
	void memoryRequested(uint64_t fileOffset);

private:
	void rebuild();                       // re-applies filter_ to symbols_
	QTreeWidgetItem* addGroup(const QString& title, int count);

	Session* session_ = nullptr;
	Theme theme_ = Theme::dark();
	QLabel* header_ = nullptr;
	QLabel* countLabel_ = nullptr;
	QLineEdit* filter_ = nullptr;
	QAction* clearAct_ = nullptr;         // filter_'s clear button, shown while it has text
	QTreeWidget* tree_ = nullptr;
	std::vector<SymbolInfo> symbols_;

	// True between the decode starting and the scan producing its rows, so rebuild()
	// shows "Scanning…" instead of an empty sidebar. UI thread only.
	bool scanning_ = false;
};

} // namespace gui

