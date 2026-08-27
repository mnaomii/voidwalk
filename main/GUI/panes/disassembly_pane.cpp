#include "disassembly_pane.h"

#include "disasm_delegate.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
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

// ---------------------------------------------------------------------------
// DisasmModel
// ---------------------------------------------------------------------------

void DisasmModel::resetRows() {
	beginResetModel();
	rows_ = 0;
	endResetModel();
}

void DisasmModel::syncTo(int n) {
	if (n < 0) n = 0;
	if (n == rows_) return;
	if (n > rows_) {
		beginInsertRows({}, rows_, n - 1);
		rows_ = n;
		endInsertRows();
	} else {
		beginRemoveRows({}, n, rows_ - 1);
		rows_ = n;
		endRemoveRows();
	}
}

void DisasmModel::clearEdits() {
	if (edits_.empty()) return;
	edits_.clear();
	// Repaint the Instruction column so any override text reverts to the decode.
	if (rows_ > 0)
		emit dataChanged(index(0, DisassemblyPane::ColInstruction),
		                 index(rows_ - 1, DisassemblyPane::ColInstruction),
		                 {Qt::DisplayRole, Qt::EditRole});
}

int DisasmModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : rows_;
}

int DisasmModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : DisassemblyPane::ColCount;
}

QString DisasmModel::noteFor(int i) const {
	if (!session_) return {};
	const auto idx = static_cast<std::size_t>(i);
	if (idx >= session_->rowCount()) return {};

	std::string mnemonic;
	uint64_t target = 0;
	if (!staticTarget(session_->rowText(idx), &mnemonic, &target)) return {};

	if (mnemonic == "call")
		return QStringLiteral("sub_%1").arg(target, 6, 16, QLatin1Char('0')).toLower();
	// Jumps: say which way, which is what you actually scan for reading a loop.
	// Backwards to an address already on screen = a loop.
	if (mnemonic.size() > 1 && mnemonic.front() == 'j') {
		if (target < session_->rowVaddr(idx)) return tr("loop");
		return QString();
	}
	return {};
}

QVariant DisasmModel::data(const QModelIndex& index, int role) const {
	if (!session_ || !index.isValid()) return {};
	if (role != Qt::DisplayRole && role != Qt::EditRole) return {};

	const auto row = static_cast<std::size_t>(index.row());
	// Never read past what the view has been told about, nor past the live rows.
	// Each row*() call reads through to the core for this one row only.
	if (index.row() >= rows_ || row >= session_->rowCount()) return {};

	switch (index.column()) {
	case DisassemblyPane::ColGutter:
		return {}; // delegate paints the marker; no text
	case DisassemblyPane::ColAddress:
		return QStringLiteral("0x%1").arg(session_->rowVaddr(row), 8, 16, QLatin1Char('0'));
	case DisassemblyPane::ColBytes:
		return QString::fromStdString(session_->rowBytes(row));
	case DisassemblyPane::ColInstruction:
		for (const auto& e : edits_)
			if (e.first == row) return QString::fromStdString(e.second);
		return QString::fromStdString(session_->rowText(row));
	case DisassemblyPane::ColNotes:
		return noteFor(index.row());
	}
	return {};
}

QVariant DisasmModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
	switch (section) {
	// The gutter header stays blank: a caption over a 26px marker column would be
	// noise, and the column's meaning is obvious the moment a marker appears.
	case DisassemblyPane::ColGutter:      return QString();
	case DisassemblyPane::ColAddress:     return tr("ADDRESS");
	case DisassemblyPane::ColBytes:       return tr("BYTES");
	case DisassemblyPane::ColInstruction: return tr("INSTRUCTION");
	case DisassemblyPane::ColNotes:       return tr("NOTES");
	}
	return {};
}

Qt::ItemFlags DisasmModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	// Only the Instruction column is editable; the rest are read-only.
	if (index.isValid() && index.column() == DisassemblyPane::ColInstruction)
		f |= Qt::ItemIsEditable;
	return f;
}

bool DisasmModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid() || role != Qt::EditRole
		|| index.column() != DisassemblyPane::ColInstruction)
		return false;

	const auto row = static_cast<std::size_t>(index.row());
	const std::string text = value.toString().toStdString();
	bool found = false;
	for (auto& e : edits_)
		if (e.first == row) { e.second = text; found = true; break; }
	if (!found) edits_.emplace_back(row, text);

	emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
	emit editsChanged();
	return true;
}

// ---------------------------------------------------------------------------
// DisassemblyPane
// ---------------------------------------------------------------------------

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

	model_ = new DisasmModel(this);

	view_ = new QTableView(this);
	view_->setModel(model_);
	view_->setFont(monoFont());
	view_->setSelectionBehavior(QAbstractItemView::SelectRows);
	view_->setSelectionMode(QAbstractItemView::SingleSelection);
	// Only the Instruction column is editable (enforced by the model's flags()).
	view_->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::SelectedClicked
		| QAbstractItemView::EditKeyPressed);
	view_->verticalHeader()->setVisible(false);
	view_->verticalHeader()->setDefaultSectionSize(kRowHeight);
	view_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	view_->setShowGrid(false);
	view_->setFrameShape(QFrame::NoFrame);
	view_->setAlternatingRowColors(false); // flat rows; the delegate carries the color
	view_->setWordWrap(false);
	view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	view_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

	QHeaderView* head = view_->horizontalHeader();
	head->setHighlightSections(false);
	// NOTE: no ResizeToContents anywhere. On a virtualized view it would measure
	// every row (O(n)) and bring the freeze straight back. Widths are derived from
	// the mono font and fixed; the Instruction column absorbs the slack. Bytes is
	// left draggable since instruction length varies.
	const QFontMetrics fm(monoFont());
	head->setSectionResizeMode(ColGutter, QHeaderView::Fixed);
	head->setSectionResizeMode(ColAddress, QHeaderView::Fixed);
	head->setSectionResizeMode(ColBytes, QHeaderView::Interactive);
	head->setSectionResizeMode(ColInstruction, QHeaderView::Stretch);
	head->setSectionResizeMode(ColNotes, QHeaderView::Fixed);
	view_->setColumnWidth(ColGutter, kGutterWidth);
	view_->setColumnWidth(ColAddress, fm.horizontalAdvance(QStringLiteral("0x00000000")) + 24);
	view_->setColumnWidth(ColBytes, fm.horizontalAdvance(QStringLiteral("00 00 00 00 00 00 ")) + 16);
	view_->setColumnWidth(ColNotes, kNotesWidth);

	delegate_ = new DisasmDelegate(view_);
	view_->setItemDelegate(delegate_);
	layout->addWidget(view_);

	// The model owns the edit set (setData is the only place edits arrive); bubble
	// its change signal up so MainWindow's Recompile action tracks it as before.
	connect(model_, &DisasmModel::editsChanged, this, &DisassemblyPane::editsChanged);
}

void DisassemblyPane::setSession(Session* s) {
	session_ = s;
	model_->setSession(s);
}

void DisassemblyPane::setTheme(const Theme& theme) {
	delegate_->setTheme(theme);
	view_->viewport()->update();
}

void DisassemblyPane::refresh() {
	if (!session_ || !session_->loaded()) {
		banner_->setVisible(false);
		model_->resetRows();
		lastGen_ = session_ ? session_->decodeGeneration() : 0;
		return;
	}

	// decodedForReal() is briefly false at the start of every decode, before the
	// first instructions publish. Don't flash a "stub" banner at a decoder that is
	// simply still working — only surface it once the worker has finished and
	// genuinely produced no real instructions (a truly unimplemented arch).
	if (!session_->decodedForReal() && !session_->isDecoding()) {
		const QString note = QString::fromStdString(session_->decodeNote());
		banner_->setText(!note.isEmpty()
			? note
			: tr("The decoder for %1 is a stub - showing raw .text bytes instead of real instructions.")
				.arg(QString::fromStdString(session_->architecture())));
		banner_->setVisible(true);
	} else {
		banner_->setVisible(false);
	}

	// A new binary since the last poll: the rows are entirely different content, so
	// reset the model rather than diff counts against the previous binary's rows.
	const uint64_t gen = session_->decodeGeneration();
	if (gen != lastGen_) {
		model_->resetRows();
		lastGen_ = gen;
	}
	// Append-only growth: the view keeps the rows it has and ingests just the tail.
	model_->syncTo(static_cast<int>(session_->rowCount()));
}

void DisassemblyPane::navigateTo(uint64_t vaddr) {
	if (!session_) return;
	// Only rows the view knows about (rowCount) are navigable; row vaddrs are read
	// through to the core. Addresses ascend, so stop at the first one past vaddr.
	const int shown = model_->rowCount();
	if (shown <= 0) return;
	int best = -1;
	for (int i = 0; i < shown; ++i) {
		if (session_->rowVaddr(static_cast<std::size_t>(i)) <= vaddr) best = i;
		else break;
	}
	if (best < 0) best = 0;

	view_->selectRow(best);
	view_->scrollTo(model_->index(best, ColInstruction), QAbstractItemView::PositionAtCenter);
	view_->setFocus(Qt::OtherFocusReason);
}

} // namespace gui
