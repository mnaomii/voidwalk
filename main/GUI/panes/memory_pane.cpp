#include "memory_pane.h"

#include "../theme/theme.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <algorithm>

namespace gui {

MemoryPane::MemoryPane(QWidget* parent)
	: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);

	auto* gotoRow = new QHBoxLayout();
	gotoRow->addWidget(new QLabel(tr("Section:"), this));
	sectionBox_ = new QComboBox(this);
	// NOT AdjustToContents: that sizes the box to ".rodata  (off 0x2000, 918264
	// bytes)" and makes it the pane's minimum width, which at a narrow dock pushed
	// the offset field off the edge. Give it a readable minimum instead and let it
	// elide its own label; the popup still shows every entry in full.
	sectionBox_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	sectionBox_->setMinimumContentsLength(12);
	sectionBox_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	gotoRow->addWidget(sectionBox_, 1);
	gotoRow->addSpacing(12);
	gotoRow->addWidget(new QLabel(tr("Go to offset:"), this));
	gotoBox_ = new QLineEdit(this);
	gotoBox_->setPlaceholderText(QStringLiteral("0x0"));
	gotoBox_->setMinimumWidth(80);
	gotoRow->addWidget(gotoBox_);
	layout->addLayout(gotoRow);

	populateSections(); // seed with the "not loaded" placeholder + See all

	view_ = new QPlainTextEdit(this);
	view_->setReadOnly(true);
	view_->setFont(monoFont());
	view_->setLineWrapMode(QPlainTextEdit::NoWrap);
	layout->addWidget(view_);

	connect(sectionBox_, QOverload<int>::of(&QComboBox::activated),
		this, &MemoryPane::onSectionActivated);

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

void MemoryPane::populateSections() {
	// Rebuilding fires no activation: block the signal so the reset to index 0
	// doesn't look like a user picking a section.
	const QSignalBlocker block(sectionBox_);
	sectionBox_->clear();

	sectionBox_->addItem(tr("Jump to…"));
	sectionBox_->setItemData(0, KindNone, Qt::UserRole);

	sectionBox_->addItem(tr("See all (entire file)"));
	sectionBox_->setItemData(sectionBox_->count() - 1, KindAll, Qt::UserRole);

	if (session_ && session_->loaded()) {
		for (const SectionInfo& s : session_->sections()) {
			sectionBox_->addItem(QString("%1  (off 0x%2, %3 bytes)")
				.arg(QString::fromStdString(s.name))
				.arg(s.offset, 0, 16)
				.arg(s.size));
			const int i = sectionBox_->count() - 1;
			// The box elides when the dock is narrow; the tooltip keeps the full
			// entry reachable.
			sectionBox_->setItemData(i, sectionBox_->itemText(i), Qt::ToolTipRole);
			sectionBox_->setItemData(i, KindSection, Qt::UserRole);
			sectionBox_->setItemData(i, static_cast<qulonglong>(s.offset), Qt::UserRole + 1);
		}
	}
	sectionBox_->setCurrentIndex(0);
}

void MemoryPane::onSectionActivated(int idx) {
	if (idx < 0) return;
	switch (sectionBox_->itemData(idx, Qt::UserRole).toInt()) {
	case KindAll:
		showAll_ = true;
		top_ = 0;
		refresh();
		break;
	case KindSection:
		// gotoOffset clears showAll_ and repaints the windowed view.
		gotoOffset(sectionBox_->itemData(idx, Qt::UserRole + 1).toULongLong());
		break;
	default:
		break; // KindNone placeholder — nothing to do
	}
}

void MemoryPane::refresh() {
	if (!session_ || !session_->loaded()) {
		lastFile_.clear();
		showAll_ = false;
		top_ = 0;
		populateSections();
		view_->setPlainText(tr("[no binary loaded]"));
		return;
	}

	// New binary: reset the view to the top and rebuild the section list.
	const QString path = QString::fromStdString(session_->filePath());
	if (path != lastFile_) {
		lastFile_ = path;
		showAll_ = false;
		top_ = 0;
		populateSections();
	}

	const auto size = static_cast<std::uint64_t>(session_->binarySize());
	// "See all" dumps the whole file from 0; otherwise a 2 KiB window from top_.
	const std::uint64_t base = showAll_ ? 0 : top_;
	const std::uint64_t count = showAll_
		? size
		: ((size > top_) ? std::min<std::uint64_t>(size - top_, 2048) : 0);
	const std::vector<uint8_t> data = session_->bytes(base, static_cast<size_t>(count));

	QString out;
	// ~76 chars per 16-byte row; use qsizetype so a whole-file "See all" dump on a
	// large binary can't overflow the reserve hint (the old int math wrapped >0.5 GB).
	out.reserve(static_cast<qsizetype>(data.size()) * 5);

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

		out += QString("%1").arg(base + i, 8, 16, QLatin1Char('0')).toUpper();
		out += QStringLiteral("  ");
		out += hexPart;
		out += QStringLiteral("  |");
		out += ascii;
		out += QStringLiteral("|\n");
	}

	view_->setPlainText(out);
}

void MemoryPane::gotoOffset(std::uint64_t offset) {
	showAll_ = false; // an explicit seek is a window, not the whole-file dump
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
