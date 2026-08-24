#ifndef GUI_DISASSEMBLY_PANE_H
#define GUI_DISASSEMBLY_PANE_H

#include "../model/gui_session.h"
#include "../theme/theme.h"

#include <QWidget>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class QTableWidget;
class QLabel;

namespace gui {

class DisasmDelegate;

// Disassembly view: one row per instruction, columns
//   [gutter] | ADDRESS | BYTES | INSTRUCTION | NOTES
//
// The gutter is a fixed 26px column carrying the execution marker and future
// breakpoint dots — previously the current instruction had nowhere to live but
// the selection color, which vanished the moment the user clicked another row.
// NOTES is derived, right-aligned and ghosted: resolved call targets, argument
// slots, loop back-edges. Both are non-editable; INSTRUCTION keeps its editor
// and MainWindow's Recompile action feeds pendingEdits() to
// Session::applyPatches() (assembler backend is a stub, so edits stay pending).
//
// A banner explains the raw-bytes fallback when the arch decoder is
// unimplemented (Session::decodedForReal()/decodeNote()).
// Painting goes through DisasmDelegate for per-token syntax coloring.
class DisassemblyPane : public QWidget {
	Q_OBJECT
public:
	// Column order, shared with DisasmDelegate.
	enum Column { ColGutter = 0, ColAddress, ColBytes, ColInstruction, ColNotes, ColCount };

	explicit DisassemblyPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }
	void setTheme(const Theme& theme); // forwards to the delegate, repaints

	// Row index -> new instruction text, for every row edited since the last
	// refresh(). Index is into Session::disassembly().
	std::vector<std::pair<std::size_t, std::string>> pendingEdits() const;
	bool hasPendingEdits() const { return !edits_.empty(); }
	void clearPendingEdits();

public slots:
	void refresh();
	// Scrolls to (and selects) the row at `vaddr`, or the closest row above it.
	// Wired to SymbolsPane::navigateRequested and the toolbar's address field.
	void navigateTo(uint64_t vaddr);

signals:
	void editsChanged();

private:
	// The NOTES text for row `i`: resolved symbol for a call, direction for a
	// jump, "" when there is nothing worth saying. Never invents facts — an
	// unresolvable target yields an empty note rather than a guess.
	QString noteFor(std::size_t i) const;

	Session* session_ = nullptr;
	QTableWidget* table_ = nullptr;
	QLabel* banner_ = nullptr;
	DisasmDelegate* delegate_ = nullptr;
	// row -> edited text; populated from cellChanged, cleared on refresh.
	std::vector<std::pair<std::size_t, std::string>> edits_;
	bool populating_ = false; // guard cellChanged during programmatic fill
};

} // namespace gui

#endif
