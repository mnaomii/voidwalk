#include "memory_pane.h"

#include "../theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace gui {

MemoryPane::MemoryPane(QWidget* parent)
	: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);

	auto* gotoRow = new QHBoxLayout();
	gotoRow->addWidget(new QLabel(tr("Go to offset:"), this));
	gotoBox_ = new QLineEdit(this);
	gotoBox_->setPlaceholderText(QStringLiteral("0x0"));
	gotoRow->addWidget(gotoBox_);
	layout->addLayout(gotoRow);

	view_ = new QPlainTextEdit(this);
	view_->setReadOnly(true);
	view_->setFont(monoFont());
	view_->setLineWrapMode(QPlainTextEdit::NoWrap);
	layout->addWidget(view_);

	connect(gotoBox_, &QLineEdit::returnPressed, this, [this]() {
		QString text = gotoBox_->text().trimmed();
		if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
			text.remove(0, 2);

		bool ok = false;
		const std::uint64_t value = text.toULongLong(&ok, 16);
		if (ok)
			gotoOffset(value);
	});
}

void MemoryPane::refresh() {
	if (!session_ || !session_->loaded()) {
		view_->setPlainText(tr("[no binary loaded]"));
		return;
	}

	const auto size = static_cast<std::uint64_t>(session_->binarySize());
	const std::uint64_t count = (size > top_) ? std::min<std::uint64_t>(size - top_, 2048) : 0;
	const std::vector<uint8_t> data = session_->bytes(top_, static_cast<size_t>(count));

	QString out;
	out.reserve(static_cast<int>(data.size()) * 4);

	for (std::uint64_t i = 0; i < data.size(); i += 16) {
		const std::size_t rowLen = std::min<std::size_t>(16, static_cast<std::size_t>(data.size() - i));

		QString hexPart;
		QString ascii;
		for (std::size_t col = 0; col < 16; ++col) {
			if (col > 0) hexPart += QChar(' ');
			if (col == 8) hexPart += QChar(' '); // extra gap between the two 8-byte groups

			if (col < rowLen) {
				const uint8_t b = data[static_cast<std::size_t>(i) + col];
				hexPart += QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
				ascii += (b >= 0x20 && b <= 0x7E) ? QChar(b) : QChar('.');
			}
			else {
				hexPart += QStringLiteral("  "); // pad so the ASCII gutter stays aligned
			}
		}

		out += QString("%1").arg(top_ + i, 8, 16, QLatin1Char('0')).toUpper();
		out += QStringLiteral("  ");
		out += hexPart;
		out += QStringLiteral("  |");
		out += ascii;
		out += QStringLiteral("|\n");
	}

	view_->setPlainText(out);
}

void MemoryPane::gotoOffset(std::uint64_t offset) {
	const auto size = static_cast<std::uint64_t>(session_ ? session_->binarySize() : 0);
	if (size == 0)
		top_ = 0;
	else if (offset >= size)
		top_ = size - 1;
	else
		top_ = offset;

	refresh();
}

} // namespace gui
