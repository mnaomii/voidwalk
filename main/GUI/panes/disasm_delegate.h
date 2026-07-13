#ifndef GUI_DISASM_DELEGATE_H
#define GUI_DISASM_DELEGATE_H

#include "../theme/theme.h"

#include <QStyledItemDelegate>

namespace gui {

// Paints the disassembly table with syntax coloring:
//   col 0 (Address)     -> faint
//   col 1 (Bytes)       -> dim
//   col 2 (Instruction) -> tokenized: mnemonic / jump / register / immediate /
//                          jump target / punctuation, colors from the Theme.
// Editing is untouched — the default QStyledItemDelegate editor still handles
// the Instruction column.
class DisasmDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit DisasmDelegate(QObject* parent = nullptr);

	void setTheme(const Theme& theme); // call on theme switch, then viewport()->update()

	void paint(QPainter* painter, const QStyleOptionViewItem& option,
	           const QModelIndex& index) const override;

private:
	struct Token {
		QString text;
		QColor color;
	};
	std::vector<Token> tokenize(const QString& instruction) const;

	Theme theme_;
};

} // namespace gui

#endif
