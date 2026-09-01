#ifndef GUI_DISASM_DELEGATE_H
#define GUI_DISASM_DELEGATE_H

#include "../theme/theme.h"

#include <QStyledItemDelegate>
#include <cstdint>
#include <vector>

namespace gui {

// Paints the disassembly table with syntax coloring:
//   col 0 (Gutter)      -> execution marker / breakpoint dot
//   col 1 (Address)     -> dim
//   col 2 (Bytes)       -> faint
//   col 3 (Instruction) -> tokenized: mnemonic / jump / register / immediate /
//                          jump target / punctuation, colors from the Theme.
//   col 4 (Notes)       -> ghosted, right-aligned
//
// The current instruction gets a full-row band (accentBg + a 2px accent rule in
// the gutter) drawn under the text, so it stays visible when the selection is
// somewhere else — that pairing is the whole point of the gutter column.
// Editing is untouched — the default QStyledItemDelegate editor still handles
// the Instruction column.
class DisasmDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit DisasmDelegate(QObject* parent = nullptr);

	void setTheme(const Theme& theme); // call on theme switch, then viewport()->update()

	// Row that holds the instruction pointer, or -1 for none (no debugger yet,
	// so this stays -1 until MainWindow has a step event to report).
	void setCurrentRow(int row) { currentRow_ = row; }
	// Rows carrying a breakpoint. Set from the pane's gutter click handler when
	// breakpoints land; empty today.
	void setBreakpointRows(std::vector<int> rows) { breakpoints_ = std::move(rows); }

	void paint(QPainter* painter, const QStyleOptionViewItem& option,
	           const QModelIndex& index) const override;

	// Tooltip only for a cell the paint above had to clip. A disassembly is dense
	// enough that a tooltip on every row would be noise, and the model returns the
	// same string for ToolTipRole as for DisplayRole, so the popup is exactly the
	// text that did not fit.
	bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
	               const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
	struct Token {
		QString text;
		QColor color;
	};
	std::vector<Token> tokenize(const QString& instruction) const;
	bool isBreakpoint(int row) const;

	Theme theme_;
	int currentRow_ = -1;
	std::vector<int> breakpoints_;
};

} // namespace gui

#endif
