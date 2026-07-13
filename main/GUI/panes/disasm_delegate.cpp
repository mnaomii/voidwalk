#include "disasm_delegate.h"

#include <QApplication>
#include <QPainter>
#include <QSet>

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

	// Background (selection, hover, alternate) via the style, text by hand.
	const QString text = opt.text;
	opt.text.clear();
	const QWidget* widget = option.widget;
	QStyle* style = widget ? widget->style() : QApplication::style();
	style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

	painter->save();
	painter->setRenderHint(QPainter::TextAntialiasing, true);
	painter->setFont(opt.font);
	const QRect r = opt.rect.adjusted(8, 0, -4, 0);
	const bool selected = opt.state & QStyle::State_Selected;

	if (index.column() == 0 || index.column() == 1) {
		painter->setPen(selected ? theme_.accentText
			: (index.column() == 0 ? theme_.textFaint : theme_.textDim));
		painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter, text);
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
			painter->setPen(selected ? theme_.textBright : tok.color);
			painter->drawText(QPointF(x, baseline), tok.text);
			x += fm.horizontalAdvance(tok.text);
			if (x > r.right()) break;
		}
	}
	painter->restore();
}

} // namespace gui
