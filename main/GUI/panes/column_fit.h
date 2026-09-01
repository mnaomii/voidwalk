#ifndef GUI_COLUMN_FIT_H
#define GUI_COLUMN_FIT_H

#include <QObject>

class QAbstractItemView;
class QHeaderView;

namespace gui {

// Shared column-sizing policy for every table/tree pane.
//
// The panes used to size columns with QHeaderView::Stretch (disassembly
// INSTRUCTION, symbols NAME) or ResizeToContents (register names, symbol
// addresses). Both fill the view, and neither can be dragged: at a window width
// the layout was not designed for, long instructions and long symbol names were
// clipped with no way to widen the column and no way to scroll to the rest.
// ResizeToContents is doubly out on the disassembly, which is virtualized —
// measuring it means measuring every row.
//
// So every section is left Interactive (draggable, with a real minimum) and this
// object hands the leftover width to one designated column, recomputing whenever
// the view is resized or another column is dragged. The moment the user drags the
// fill column itself it stops interfering, so a hand-set width survives the next
// resize; setUserSized(false) gives the automatic behaviour back.
//
// Horizontal padding a cell loses before its text starts: the stylesheet gives
// QTableView::item / QTreeView::item 8px each side, and the delegate adds a
// couple more. A caller sizing a column from a string's width should add this,
// or the column comes out exactly too narrow for the text it was measured from.
constexpr int kCellPadding = 24;

// Parents itself to the view, so it lives exactly as long as the view does.
class ColumnFit : public QObject {
public:
	// `view` must be a QTableView or QTreeView (that is where the header comes
	// from). `fillColumn` absorbs the slack and never shrinks below `minFillWidth`.
	ColumnFit(QAbstractItemView* view, int fillColumn, int minFillWidth);

	// Recompute now — after column widths or the set of columns changed
	// programmatically.
	void refit();

	// Hand control of the fill column back to (false) or away from (true) this
	// object. Set automatically when the user drags that column.
	void setUserSized(bool userSized) { userSized_ = userSized; }
	bool userSized() const { return userSized_; }

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	QAbstractItemView* view_ = nullptr;
	QHeaderView* header_ = nullptr;
	int fillColumn_ = 0;
	int minFillWidth_ = 0;
	bool applying_ = false;  // guards our own resizeSection against re-entry
	bool userSized_ = false; // the user has set this column by hand
};

// Applies the shared header policy to `view` — every section Interactive, a real
// minimum section size, no last-section stretch (it fights ColumnFit) — and
// installs a ColumnFit on `fillColumn`. Returns it in case the caller needs to
// refit after repopulating.
ColumnFit* installColumnFit(QAbstractItemView* view, int fillColumn,
                            int minFillWidth, int minSectionWidth = 44);

} // namespace gui

#endif
