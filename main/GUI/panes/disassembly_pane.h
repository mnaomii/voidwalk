#ifndef GUI_DISASSEMBLY_PANE_H
#define GUI_DISASSEMBLY_PANE_H

#include "../model/gui_session.h"

#include <QWidget>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class QTableWidget;
class QLabel;

namespace gui {

// Disassembly view: one row per instruction with Address | Bytes | Instruction
// columns. The Instruction column is editable — the user rewrites mnemonics and
// the main window's Recompile action feeds pendingEdits() to
// Session::applyPatches() (assembler backend is a stub for now, so edits stay
// pending). A banner explains the raw-bytes fallback when the arch decoder is
// unimplemented (Session::decodedForReal()/decodeNote()).
class DisassemblyPane : public QWidget {
	Q_OBJECT
public:
	explicit DisassemblyPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }

	// Row index -> new instruction text, for every row edited since the last
	// refresh(). Index is into Session::disassembly().
	std::vector<std::pair<std::size_t, std::string>> pendingEdits() const;
	bool hasPendingEdits() const { return !edits_.empty(); }
	void clearPendingEdits();

public slots:
	void refresh();

signals:
	void editsChanged();

private:
	Session* session_ = nullptr;
	QTableWidget* table_ = nullptr;
	QLabel* banner_ = nullptr;
	// row -> edited text; populated from cellChanged, cleared on refresh.
	std::vector<std::pair<std::size_t, std::string>> edits_;
	bool populating_ = false; // guard cellChanged during programmatic fill
};

} // namespace gui

#endif
