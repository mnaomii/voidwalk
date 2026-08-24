#include "disasm_delegate.h"

#include "disassembly_pane.h"

#include <QApplication>
#include <QPainter>
#include <QSet>
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

	const QRect r = opt.rect.adjusted(8, 0, -4, 0);

	if (column == DisassemblyPane::ColAddress || column == DisassemblyPane::ColBytes) {
		const QColor base = column == DisassemblyPane::ColAddress ? theme_.textDim : theme_.textFaint;
		painter->setPen(selected || isCurrent ? theme_.accentText : base);
		painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter, text);
	}
	else if (column == DisassemblyPane::ColNotes) {
		painter->setPen(selected || isCurrent ? theme_.accent : theme_.textGhost);
		painter->drawText(opt.rect.adjusted(0, 0, -12, 0),
		                  Qt::AlignRight | Qt::AlignVCenter, text);
	}
	else {
		// Tokens are drawn one at a time, so the pen advances by hand. Keep the
		// advance in qreal and draw on a baseline: rounding each token to whole
		// pixels (QFontMetrics/QRect) accumulates across a line and shows up as
		// uneven letter spacing.
		const QFontMetricsF fm(opt.font);
		const qreal baseline = r.top() + (r.height() + fm.ascent() - fm.descent()) / 2.0;
		qreal x = r.left();
		painter->setClipRect(r);
		for (const Token& tok : tokenize(text)) {
			// On the current row every token flattens to bright text: the band
			// already carries the emphasis, and keeping the syntax colors on top
			// of accentBg turned the one row you care about into the noisiest.
			painter->setPen(selected || isCurrent ? theme_.textBright : tok.color);
			painter->drawText(QPointF(x, baseline), tok.text);
			x += fm.horizontalAdvance(tok.text);
			if (x > r.right()) break;
		}
	}
	painter->restore();
}

} // namespace gui
