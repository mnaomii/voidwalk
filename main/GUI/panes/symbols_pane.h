#ifndef GUI_SYMBOLS_PANE_H
#define GUI_SYMBOLS_PANE_H

#include "../model/gui_session.h"
#include "../model/symbols.h"
#include "../theme/theme.h"

#include <QWidget>
#include <cstdint>
#include <vector>

class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace gui {

// Symbol sidebar: FUNCTIONS / IMPORTS / STRINGS, each a collapsible group with
// name on the left and address (or module) on the right. Selecting a row emits
// navigateRequested(vaddr); MainWindow forwards that to the disassembly pane,
// which is why the toolbar's field can shrink to raw addresses only — symbol
// lookup lives here, in the filter box, instead of behind a dialog.
//
// Contents come from collectSymbols() (model/symbols.h), so the pane needs no
// core API of its own and empty groups simply don't render.
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

private:
	void rebuild();                       // re-applies filter_ to symbols_
	QTreeWidgetItem* addGroup(const QString& title, int count);

	Session* session_ = nullptr;
	Theme theme_ = Theme::dark();
	QLabel* header_ = nullptr;
	QLabel* countLabel_ = nullptr;
	QLineEdit* filter_ = nullptr;
	QTreeWidget* tree_ = nullptr;
	std::vector<SymbolInfo> symbols_;
};

} // namespace gui

#endif
