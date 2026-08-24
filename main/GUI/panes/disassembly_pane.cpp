#include "disassembly_pane.h"

#include "disasm_delegate.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace gui {

namespace {
// 25px at a 13px mono is a 1.9 line-height: enough that a wall of instructions
// has horizontal air, tight enough that a screen still holds ~28 rows. The old
// 24px with an unhinted 13px font left descenders touching the next row's caps.
constexpr int kRowHeight = 25;
constexpr int kGutterWidth = 26;
constexpr int kNotesWidth = 150;

// Parses "call 0x00401160" / "jb 0x401014" into mnemonic + numeric target.
bool staticTarget(const std::string& text, std::string* mnemonic, uint64_t* target) {
	std::istringstream is(text);
	std::string op, arg;
	if (!(is >> op >> arg)) return false;
	std::transform(op.begin(), op.end(), op.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	*mnemonic = op;
	if (arg.rfind("0x", 0) != 0) return false;
	try {
		*target = std::stoull(arg.substr(2), nullptr, 16);
	} catch (...) {
		return false;
	}
	return true;
}
} // namespace

DisassemblyPane::DisassemblyPane(QWidget* parent)
	: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	banner_ = new QLabel(this);
	banner_->setWordWrap(true);
	banner_->setStyleSheet(QStringLiteral("color: palette(mid); padding: 8px 12px;"));
	banner_->setVisible(false);
	layout->addWidget(banner_);

	table_ = new QTableWidget(this);
	table_->setColumnCount(ColCount);
	// The gutter header stays blank: a caption over a 26px marker column would
	// be noise, and the column's meaning is obvious the moment a marker appears.
	table_->setHorizontalHeaderLabels({
		QString(), tr("ADDRESS"), tr("BYTES"), tr("INSTRUCTION"), tr("NOTES")});
	table_->setFont(monoFont());
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);
	// Only the Instruction column is meant to be edited; the others are made
	// non-editable per-item in refresh() via item flags.
	table_->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::SelectedClicked
		| QAbstractItemView::EditKeyPressed);
	table_->verticalHeader()->setVisible(false);
	table_->verticalHeader()->setDefaultSectionSize(kRowHeight);
	table_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	table_->setShowGrid(false);
	table_->setFrameShape(QFrame::NoFrame);
	table_->setAlternatingRowColors(false); // flat rows; the delegate carries the color
	table_->setWordWrap(false);
	table_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	QHeaderView* head = table_->horizontalHeader();
	head->setHighlightSections(false);
	head->setSectionResizeMode(ColGutter, QHeaderView::Fixed);
	head->setSectionResizeMode(ColAddress, QHeaderView::ResizeToContents);
	head->setSectionResizeMode(ColBytes, QHeaderView::ResizeToContents);
	head->setSectionResizeMode(ColInstruction, QHeaderView::Stretch);
	head->setSectionResizeMode(ColNotes, QHeaderView::Fixed);
	table_->setColumnWidth(ColGutter, kGutterWidth);
	table_->setColumnWidth(ColNotes, kNotesWidth);

	delegate_ = new DisasmDelegate(table_);
	table_->setItemDelegate(delegate_);
	layout->addWidget(table_);

	connect(table_, &QTableWidget::cellChanged, this, [this](int row, int col) {
		if (populating_) return; // programmatic fill in refresh(), not a user edit
		if (col != ColInstruction) return;

		QTableWidgetItem* item = table_->item(row, col);
		if (!item) return;

		const auto rowIndex = static_cast<std::size_t>(row);
		const std::string text = item->text().toStdString();
		for (auto& edit : edits_) {
			if (edit.first == rowIndex) {
				edit.second = text;
				emit editsChanged();
				return;
			}
		}
		edits_.emplace_back(rowIndex, text);
		emit editsChanged();
	});
}

void DisassemblyPane::setTheme(const Theme& theme) {
	delegate_->setTheme(theme);
	table_->viewport()->update();
}

QString DisassemblyPane::noteFor(std::size_t i) const {
	if (!session_) return {};
	const auto& rows = session_->disassembly();
	if (i >= rows.size()) return {};

	std::string mnemonic;
	uint64_t target = 0;
	if (!staticTarget(rows[i].text, &mnemonic, &target)) return {};

	if (mnemonic == "call")
		return QStringLiteral("sub_%1").arg(target, 6, 16, QLatin1Char('0')).toLower();
	// Jumps: say which way, which is the thing you actually scan for when
	// reading a loop. Backwards to an address already on screen = a loop.
	if (mnemonic.size() > 1 && mnemonic.front() == 'j') {
		if (target < rows[i].vaddr) return tr("loop");
		return QString();
	}
	return {};
}

void DisassemblyPane::refresh() {
	populating_ = true;
	edits_.clear();
	table_->setRowCount(0);

	if (!session_ || !session_->loaded()) {
		banner_->setVisible(false);
		populating_ = false;
		return;
	}

	if (!session_->decodedForReal()) {
		const QString note = QString::fromStdString(session_->decodeNote());
		banner_->setText(!note.isEmpty()
			? note
			: tr("The decoder for %1 is a stub - showing raw .text bytes instead of real instructions.")
				.arg(QString::fromStdString(session_->architecture())));
		banner_->setVisible(true);
	}
	else {
		banner_->setVisible(false);
	}

	const auto& rows = session_->disassembly();
	table_->setRowCount(static_cast<int>(rows.size()));
	for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
		const DisasmRow& r = rows[static_cast<std::size_t>(i)];
		const auto nonEditable = [](QTableWidgetItem* item) {
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			return item;
		};

		// Gutter: empty item so the column still hit-tests for a future
		// click-to-toggle-breakpoint; the delegate paints the marker.
		table_->setItem(i, ColGutter, nonEditable(new QTableWidgetItem));

		table_->setItem(i, ColAddress, nonEditable(new QTableWidgetItem(
			QString("0x%1").arg(r.vaddr, 8, 16, QLatin1Char('0')))));
		table_->setItem(i, ColBytes, nonEditable(new QTableWidgetItem(
			QString::fromStdString(r.bytes))));

		// Instruction column keeps Qt::ItemIsEditable (the default flag).
		table_->setItem(i, ColInstruction, new QTableWidgetItem(
			QString::fromStdString(r.text)));

		auto* notes = nonEditable(new QTableWidgetItem(
			noteFor(static_cast<std::size_t>(i))));
		notes->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		table_->setItem(i, ColNotes, notes);
	}

	// Address/Bytes size to content; Instruction stretches; the fixed columns
	// keep their widths — so no resizeColumnsToContents() call here, which would
	// override the header's resize modes and make the gutter collapse.
	populating_ = false;
}

void DisassemblyPane::navigateTo(uint64_t vaddr) {
	if (!session_) return;
	const auto& rows = session_->disassembly();
	if (rows.empty()) return;

	// Closest row at or below vaddr: a symbol address that falls mid-instruction
	// (or between rows in the raw-bytes fallback) should still land somewhere
	// sensible rather than doing nothing.
	int best = -1;
	for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
		if (rows[static_cast<std::size_t>(i)].vaddr <= vaddr) best = i;
		else break;
	}
	if (best < 0) best = 0;

	table_->selectRow(best);
	table_->scrollToItem(table_->item(best, ColInstruction),
	                     QAbstractItemView::PositionAtCenter);
	table_->setFocus(Qt::OtherFocusReason);
}

std::vector<std::pair<std::size_t, std::string>> DisassemblyPane::pendingEdits() const {
	return edits_;
}

void DisassemblyPane::clearPendingEdits() {
	edits_.clear();
}

} // namespace gui
