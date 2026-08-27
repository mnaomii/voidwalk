#ifndef GUI_DISASSEMBLY_PANE_H
#define GUI_DISASSEMBLY_PANE_H

#include "../model/gui_session.h"
#include "../theme/theme.h"

#include <QAbstractTableModel>
#include <QWidget>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class QTableView;
class QLabel;

namespace gui {

class DisasmDelegate;

// Virtualizing model over Session's rows (read through Session::row*()). A QTableView only asks it for
// the cells that are actually on screen, so the disassembly can be millions of
// rows and still paint (and grow) instantly — unlike the old QTableWidget, which
// materialised one heap item per cell whether visible or not and froze the UI on
// a large binary. During a decode the row set only ever grows, so the poll loop
// tells the model the new count via syncTo() and the view ingests just the tail.
//
// Columns match DisassemblyPane::Column (the delegate keys off those names).
class DisasmModel : public QAbstractTableModel {
	Q_OBJECT
public:
	explicit DisasmModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

	void setSession(Session* s) { session_ = s; }

	// Grow (or, rarely, shrink) the visible row count to n, wrapped in the
	// begin/end signals the view needs. Append-only during a decode: a shown row's
	// content never changes, so this is all the view has to hear about.
	void syncTo(int n);
	// Full reset — the backing rows are now a different binary's, so drop the lot.
	void resetRows();

	// Edits typed into the Instruction column, as row index -> new text. Kept here
	// (not in Session) because setData() is the only place they arrive, and data()
	// has to hand the edited text back so the change stays on screen.
	std::vector<std::pair<std::size_t, std::string>> pendingEdits() const { return edits_; }
	bool hasEdits() const { return !edits_.empty(); }
	void clearEdits();

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

signals:
	void editsChanged();

private:
	// The NOTES text for row i: resolved symbol for a call, direction for a jump,
	// "" when there is nothing worth saying. Never invents facts.
	QString noteFor(int row) const;

	Session* session_ = nullptr;
	int rows_ = 0; // rows the view has been told about (<= session rows)
	std::vector<std::pair<std::size_t, std::string>> edits_;
};

// Disassembly view: one row per instruction, columns
//   [gutter] | ADDRESS | BYTES | INSTRUCTION | NOTES
//
// A QTableView + DisasmModel (virtualized) so a large binary neither freezes the
// UI nor blocks the progressive fill while the background decode runs. The gutter
// is a fixed 26px column carrying the execution marker and future breakpoint
// dots. NOTES is derived, right-aligned and ghosted. INSTRUCTION is editable and
// MainWindow's Recompile action feeds pendingEdits() to Session::applyPatches()
// (assembler backend is a stub, so edits stay pending).
//
// A banner explains the raw-bytes fallback when the arch decoder is unimplemented
// (Session::decodedForReal()/decodeNote()). Painting goes through DisasmDelegate
// for per-token syntax coloring.
class DisassemblyPane : public QWidget {
	Q_OBJECT
public:
	// Column order, shared with DisasmDelegate and DisasmModel.
	enum Column { ColGutter = 0, ColAddress, ColBytes, ColInstruction, ColNotes, ColCount };

	explicit DisassemblyPane(QWidget* parent = nullptr);

	void setSession(Session* s);
	void setTheme(const Theme& theme); // forwards to the delegate, repaints

	// Row index -> new instruction text, for every row edited since the last
	// clearPendingEdits(). Index is into Session's rows (Session::rowCount()).
	std::vector<std::pair<std::size_t, std::string>> pendingEdits() const { return model_->pendingEdits(); }
	bool hasPendingEdits() const { return model_->hasEdits(); }
	void clearPendingEdits() { model_->clearEdits(); }

public slots:
	void refresh();
	// Scrolls to (and selects) the row at `vaddr`, or the closest row above it.
	// Wired to SymbolsPane::navigateRequested and the toolbar's address field.
	void navigateTo(uint64_t vaddr);

signals:
	void editsChanged();

private:
	Session* session_ = nullptr;
	QTableView* view_ = nullptr;
	DisasmModel* model_ = nullptr;
	QLabel* banner_ = nullptr;
	DisasmDelegate* delegate_ = nullptr;
	uint64_t lastGen_ = 0; // Session::decodeGeneration() at the last refresh()
};

} // namespace gui

#endif
