#include "gui/panes/memory_pane.hpp"

#include "gui/theme/theme.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <algorithm>
#include <climits>

namespace gui {

void HexModel::reload() {
	beginResetModel();
	const std::uint64_t size = (session_ && session_->loaded()) ? session_->binarySize() : 0;
	rows_ = static_cast<int>(std::min<std::uint64_t>((size + 15) / 16, INT_MAX));
	endResetModel();
}

QVariant HexModel::data(const QModelIndex& index, int role) const {
	if (role != Qt::DisplayRole || !session_ || !index.isValid() || index.row() >= rows_) return {};

	const std::uint64_t base = static_cast<std::uint64_t>(index.row()) * 16;
	const std::vector<uint8_t> data = session_->bytes(base, 16);

	QString hexPart;
	QString ascii;
	for (std::size_t col = 0; col < 16; ++col) {
		if (col > 0) hexPart += QChar(' ');
		if (col == 8) hexPart += QChar(' '); // extra gap between the two 8-byte groups

		if (col < data.size()) {
			const uint8_t b = data[col];
			hexPart += QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
			ascii += (b >= 0x20 && b <= 0x7E) ? QChar(b) : QChar('.');
		}
		else {
			hexPart += QStringLiteral("  "); // pad so the ASCII gutter stays aligned
		}
	}
	return QString("%1").arg(base, 8, 16, QLatin1Char('0')).toUpper()
		+ QStringLiteral("  ") + hexPart + QStringLiteral("  |") + ascii + QStringLiteral("|");
}

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

	populateSections(); // seed with the "Jump to…" placeholder

	model_ = new HexModel(this);
	view_ = new QListView(this);
	view_->setModel(model_);
	view_->setFont(monoFont());
	view_->setUniformItemSizes(true); // lets the view size every row from one
	view_->setSelectionMode(QAbstractItemView::SingleSelection);
	layout->addWidget(view_);

	connect(sectionBox_, QOverload<int>::of(&QComboBox::activated), this, [this](int idx) {
		if (idx > 0) gotoOffset(sectionBox_->itemData(idx, Qt::UserRole).toULongLong());
	});

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
			sectionBox_->setItemData(i, static_cast<qulonglong>(s.offset), Qt::UserRole);
		}
	}
	sectionBox_->setCurrentIndex(0);
}

void MemoryPane::refresh() {
	const std::uint64_t gen = (session_ && session_->loaded()) ? session_->decodeGeneration() : 0;
	if (gen == lastGen_) return;
	lastGen_ = gen;
	model_->reload();
	populateSections();
	view_->scrollToTop();
}

void MemoryPane::gotoOffset(std::uint64_t offset) {
	const int rows = model_->rowCount();
	if (rows == 0) return;
	const QModelIndex idx = model_->index(static_cast<int>(std::min<std::uint64_t>(offset / 16, rows - 1)));
	view_->setCurrentIndex(idx);
	view_->scrollTo(idx, QAbstractItemView::PositionAtTop);
}

} // namespace gui
