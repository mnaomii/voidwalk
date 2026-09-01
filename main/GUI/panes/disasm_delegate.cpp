#include "disasm_delegate.h"

#include "disassembly_pane.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QHelpEvent>
#include <QPainter>
#include <QSet>
#include <QToolTip>
#include <algorithm>

namespace gui {

namespace {
bool isJumpMnemonic(const QString& w) {
	// call/ret/loop and the whole jcc family.
	if (w == QLatin1String("call") || w == QLatin1String("ret")
		|| w == QLatin1String("retn") || w == QLatin1String("iret")
		|| w.startsWith(QLatin1String("loop")))
		return true;
	return w.startsWith(QLatin1Char('j'));
}

bool isRegister(const QString& w) {
	static const QSet<QString> regs = {
		// 32-bit GP + segments + eip
		QStringLiteral("eax"), QStringLiteral("ebx"), QStringLiteral("ecx"), QStringLiteral("edx"),
		QStringLiteral("esi"), QStringLiteral("edi"), QStringLiteral("ebp"), QStringLiteral("esp"),
		QStringLiteral("eip"),
		QStringLiteral("cs"), QStringLiteral("ds"), QStringLiteral("ss"), QStringLiteral("es"),
		QStringLiteral("fs"), QStringLiteral("gs"),
		// 16/8-bit forms
		QStringLiteral("ax"), QStringLiteral("bx"), QStringLiteral("cx"), QStringLiteral("dx"),
		QStringLiteral("si"), QStringLiteral("di"), QStringLiteral("bp"), QStringLiteral("sp"),
		QStringLiteral("al"), QStringLiteral("bl"), QStringLiteral("cl"), QStringLiteral("dl"),
		QStringLiteral("ah"), QStringLiteral("bh"), QStringLiteral("ch"), QStringLiteral("dh"),
		// 64-bit, for the AMD64 decoder later
		QStringLiteral("rax"), QStringLiteral("rbx"), QStringLiteral("rcx"), QStringLiteral("rdx"),
		QStringLiteral("rsi"), QStringLiteral("rdi"), QStringLiteral("rbp"), QStringLiteral("rsp"),
		QStringLiteral("rip"), QStringLiteral("r8"), QStringLiteral("r9"), QStringLiteral("r10"),
		QStringLiteral("r11"), QStringLiteral("r12"), QStringLiteral("r13"), QStringLiteral("r14"),
		QStringLiteral("r15"),
	};
	return regs.contains(w);
}

bool isImmediate(const QString& w) {
	if (w.startsWith(QLatin1String("0x")) || w.startsWith(QLatin1String("-0x")))
		return true;
	bool ok = false;
	w.toLongLong(&ok, 10);
	return ok;
}

bool isWordChar(QChar c) {
	return c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('.');
}

// Horizontal padding the paint path leaves inside a cell, kept here so the
// "does it fit?" test in helpEvent() measures against the same width the text
// is actually drawn into.
constexpr int kCellPadLeft = 8;
constexpr int kCellPadRight = 4;

// Drawn where a cell runs out of room. A real ellipsis, not "...", so it costs
// one character's worth of the column instead of three.
const QString& ellipsis() {
	static const QString kEllipsis = QStringLiteral("\u2026");
	return kEllipsis;
}
} // namespace

DisasmDelegate::DisasmDelegate(QObject* parent)
	: QStyledItemDelegate(parent), theme_(Theme::dark()) {}

void DisasmDelegate::setTheme(const Theme& theme) {
	theme_ = theme;
}

bool DisasmDelegate::isBreakpoint(int row) const {
	return std::find(breakpoints_.begin(), breakpoints_.end(), row) != breakpoints_.end();
}

std::vector<DisasmDelegate::Token> DisasmDelegate::tokenize(const QString& text) const {
	std::vector<Token> out;
	bool mnemonicSeen = false;
	bool jumpContext = false; // color 0x… as a target after jmp/jcc/call
	bool inComment = false;

	int i = 0;
	while (i < text.size()) {
		const QChar c = text.at(i);

		if (inComment) {
			out.push_back({text.mid(i), theme_.synPunct});
			break;
		}
		if (c == QLatin1Char(';')) {
			inComment = true;
			continue;
		}

		if (isWordChar(c) || (c == QLatin1Char('-') && i + 1 < text.size()
			&& text.at(i + 1) == QLatin1Char('0'))) {
			int start = i;
			if (c == QLatin1Char('-')) ++i;
			while (i < text.size() && isWordChar(text.at(i))) ++i;
			const QString word = text.mid(start, i - start);
			const QString lower = word.toLower();

			QColor color = theme_.text;
			if (!mnemonicSeen) {
				mnemonicSeen = true;
				jumpContext = isJumpMnemonic(lower);
				color = jumpContext ? theme_.synJump : theme_.synMnemonic;
			}
			else if (isRegister(lower)) color = theme_.synRegister;
			else if (isImmediate(lower)) color = jumpContext ? theme_.synTarget : theme_.synImmediate;
			else if (lower == QLatin1String("dword") || lower == QLatin1String("word")
				|| lower == QLatin1String("qword") || lower == QLatin1String("byte")
				|| lower == QLatin1String("ptr")) color = theme_.textMuted;

			out.push_back({word, color});
			continue;
		}

		// punctuation / whitespace run
		int start = i;
		while (i < text.size() && !isWordChar(text.at(i)) && text.at(i) != QLatin1Char(';')
			&& !(text.at(i) == QLatin1Char('-') && i + 1 < text.size() && text.at(i + 1) == QLatin1Char('0')))
			++i;
		out.push_back({text.mid(start, i - start), theme_.synPunct});
	}
	return out;
}

void DisasmDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {
	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);

	const int row = index.row();
	const int column = index.column();
	const bool isCurrent = row == currentRow_;

	// Background (selection, hover) via the style, text by hand.
	const QString text = opt.text;
	opt.text.clear();
	const QWidget* widget = option.widget;
	QStyle* style = widget ? widget->style() : QApplication::style();
	style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

	const bool selected = opt.state & QStyle::State_Selected;

	// Execution band: painted over the style's background so it survives whether
	// or not the row is also selected.
	if (isCurrent && !selected)
		painter->fillRect(opt.rect, theme_.accentBg);

	painter->save();
	painter->setRenderHint(QPainter::TextAntialiasing, true);
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setFont(opt.font);

	if (column == DisassemblyPane::ColGutter) {
		if (isCurrent) {
			// 2px rule at the left edge + a small triangle: the rule is what the
			// eye catches while scrolling, the triangle is what confirms it.
			painter->fillRect(QRect(opt.rect.left(), opt.rect.top(), 2, opt.rect.height()),
			                  theme_.accent);
			const QPointF c = QPointF(opt.rect.center()) + QPointF(1.5, 0.5);
			const QPolygonF tri({QPointF(c.x() - 3, c.y() - 4),
			                     QPointF(c.x() + 4, c.y()),
			                     QPointF(c.x() - 3, c.y() + 4)});
			painter->setBrush(theme_.accent);
			painter->setPen(Qt::NoPen);
			painter->drawPolygon(tri);
		}
		else if (isBreakpoint(row)) {
			painter->setBrush(theme_.breakpoint);
			painter->setPen(Qt::NoPen);
			painter->drawEllipse(QPointF(opt.rect.center()) + QPointF(0.5, 0.5), 3.5, 3.5);
		}
		painter->restore();
		return;
	}

	const QRect r = opt.rect.adjusted(kCellPadLeft, 0, -kCellPadRight, 0);

	if (column == DisassemblyPane::ColAddress || column == DisassemblyPane::ColBytes) {
		const QColor base = column == DisassemblyPane::ColAddress ? theme_.textDim : theme_.textFaint;
		painter->setPen(selected || isCurrent ? theme_.accentText : base);
		// Elide rather than let drawText clip at the rect edge: a half-drawn hex
		// digit reads as data, an ellipsis reads as "there is more here" — and
		// helpEvent() then offers the rest on hover.
		const QFontMetrics fm(opt.font);
		painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter,
		                  fm.elidedText(text, Qt::ElideRight, r.width()));
	}
	else if (column == DisassemblyPane::ColNotes) {
		painter->setPen(selected || isCurrent ? theme_.accent : theme_.textGhost);
		const QRect notesRect = opt.rect.adjusted(0, 0, -12, 0);
		const QFontMetrics fm(opt.font);
		// Right-aligned, so the front of the name is what has to go.
		painter->drawText(notesRect, Qt::AlignRight | Qt::AlignVCenter,
		                  fm.elidedText(text, Qt::ElideLeft, notesRect.width()));
	}
	else {
		// Tokens are drawn one at a time, so the pen advances by hand. Keep the
		// advance in qreal and draw on a baseline: rounding each token to whole
		// pixels (QFontMetrics/QRect) accumulates across a line and shows up as
		// uneven letter spacing.
		const QFontMetricsF fm(opt.font);
		const qreal baseline = r.top() + (r.height() + fm.ascent() - fm.descent()) / 2.0;
		painter->setClipRect(r);

		// Elide by hand: QFontMetrics::elidedText works on a whole string, and this
		// column is painted token by token so each can carry its own color. Reserve
		// the ellipsis's width up front so it lands inside the cell rather than
		// being the thing that gets clipped.
		const bool overflows = fm.horizontalAdvance(text) > r.width();
		const qreal limit = overflows ? r.right() - fm.horizontalAdvance(ellipsis())
		                              : static_cast<qreal>(r.right());
		qreal x = r.left();
		for (const Token& tok : tokenize(text)) {
			if (x >= limit) break;
			QString piece = tok.text;
			qreal advance = fm.horizontalAdvance(piece);
			if (x + advance > limit) {
				// Trim this token to whole characters instead of drawing half of one.
				int fits = 0;
				qreal used = 0;
				while (fits < piece.size()) {
					const qreal charWidth = fm.horizontalAdvance(piece.at(fits));
					if (x + used + charWidth > limit) break;
					used += charWidth;
					++fits;
				}
				piece.truncate(fits);
				advance = used;
			}
			// On the current row every token flattens to bright text: the band
			// already carries the emphasis, and keeping the syntax colors on top
			// of accentBg turned the one row you care about into the noisiest.
			painter->setPen(selected || isCurrent ? theme_.textBright : tok.color);
			painter->drawText(QPointF(x, baseline), piece);
			x += advance;
			if (piece.size() != tok.text.size()) break; // this token was the cut point
		}
		if (overflows) {
			painter->setPen(selected || isCurrent ? theme_.textBright : theme_.synPunct);
			painter->drawText(QPointF(x, baseline), ellipsis());
		}
	}
	painter->restore();
}

bool DisasmDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                               const QStyleOptionViewItem& option, const QModelIndex& index) {
	if (!event || event->type() != QEvent::ToolTip || !index.isValid()
		|| index.column() == DisassemblyPane::ColGutter)
		return QStyledItemDelegate::helpEvent(event, view, option, index);

	const QString text = index.data(Qt::ToolTipRole).toString();
	// Same padding the paint path uses, so "did it fit" here means the same thing
	// as "did it fit" there.
	const int available = option.rect.width()
		- (index.column() == DisassemblyPane::ColNotes ? 12 : kCellPadLeft + kCellPadRight);
	const QFontMetrics fm(option.font);
	if (!text.isEmpty() && fm.horizontalAdvance(text) > available) {
		QToolTip::showText(event->globalPos(), text, view);
		return true;
	}
	// It fits, so there is nothing a tooltip could add. Consume the event rather
	// than falling through, which would show the full text on every hover.
	QToolTip::hideText();
	return true;
}

} // namespace gui
