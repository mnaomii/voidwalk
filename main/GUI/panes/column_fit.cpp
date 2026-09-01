#include "column_fit.h"

#include <QEvent>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>

#include <algorithm>

namespace gui {

namespace {

// QTableView and QTreeView both have a horizontal QHeaderView; they just spell
// the accessor differently, and QAbstractItemView does not declare either.
QHeaderView* horizontalHeaderOf(QAbstractItemView* view) {
	if (auto* table = qobject_cast<QTableView*>(view)) return table->horizontalHeader();
	if (auto* tree = qobject_cast<QTreeView*>(view)) return tree->header();
	return nullptr;
}

} // namespace

ColumnFit::ColumnFit(QAbstractItemView* view, int fillColumn, int minFillWidth)
	: QObject(view), view_(view), header_(horizontalHeaderOf(view)),
	  fillColumn_(fillColumn), minFillWidth_(minFillWidth) {
	if (!view_ || !header_) return;

	// The viewport is what actually changes width when the pane, the dock or the
	// window is resized (the view's own width includes the scrollbar).
	view_->viewport()->installEventFilter(this);

	connect(header_, &QHeaderView::sectionResized, this, [this](int index, int, int) {
		if (applying_) return; // our own resizeSection, not a drag
		if (index == fillColumn_) {
			// A drag needs a visible header to drag, so a resize arriving while the
			// view is still hidden is the pane setting up its initial widths — not a
			// decision by the user, and not a reason to stop fitting. Without this,
			// a pane that sizes its own fill column in the constructor silently
			// switches the automatic behaviour off for the rest of the session.
			if (!view_->isVisible()) return;
			userSized_ = true; // hand-set from here on; stop overriding it
		} else {
			refit();           // another column moved: re-absorb what is left
		}
	});
}

void ColumnFit::refit() {
	if (!view_ || !header_ || userSized_) return;
	if (fillColumn_ < 0 || fillColumn_ >= header_->count()) return;

	int used = 0;
	for (int i = 0; i < header_->count(); ++i) {
		if (i == fillColumn_ || header_->isSectionHidden(i)) continue;
		used += header_->sectionSize(i);
	}

	const int want = std::max(minFillWidth_, view_->viewport()->width() - used);
	if (want == header_->sectionSize(fillColumn_)) return;

	applying_ = true;
	header_->resizeSection(fillColumn_, want);
	applying_ = false;
}

bool ColumnFit::eventFilter(QObject* watched, QEvent* event) {
	if (view_ && watched == view_->viewport() && event->type() == QEvent::Resize)
		refit();
	return QObject::eventFilter(watched, event);
}

ColumnFit* installColumnFit(QAbstractItemView* view, int fillColumn,
                            int minFillWidth, int minSectionWidth) {
	QHeaderView* header = horizontalHeaderOf(view);
	if (!header) return nullptr;

	// Interactive everywhere: a Stretch or ResizeToContents section refuses to be
	// dragged, which is the whole problem this exists to fix.
	header->setSectionResizeMode(QHeaderView::Interactive);
	// A floor, so a section can't be dragged to nothing and then be impossible to
	// find again.
	header->setMinimumSectionSize(minSectionWidth);
	// Would fight ColumnFit for the same pixels, and makes the last section
	// undraggable into the bargain.
	header->setStretchLastSection(false);
	// Fitting to the visible rows on a double-clicked divider is bounded by the
	// viewport in QTableView/QTreeView, so it stays cheap even on the virtualized
	// disassembly model — unlike ResizeToContents, which measures every row.
	header->setCascadingSectionResizes(false);

	// Anything past the columns' total width stays reachable rather than lost.
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	return new ColumnFit(view, fillColumn, minFillWidth);
}

} // namespace gui
