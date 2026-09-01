#include "registers_pane.h"

#include "column_fit.h"
#include "../theme/theme.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <array>
#include <utility>

namespace gui {

RegistersPane::RegistersPane(QWidget* parent) : QWidget(parent) {
	tree_ = new QTreeWidget(this);
	tree_->setColumnCount(2);
	tree_->setHeaderLabels({tr("Register"), tr("Value")});
	tree_->setRootIsDecorated(true);       // expand arrows on the category rows
	tree_->setUniformRowHeights(true);
	tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree_->setTextElideMode(Qt::ElideRight);

	// Both columns draggable, Value taking the slack. The register names were
	// ResizeToContents and the value column stretched, so neither could be
	// resized: docked narrow, a 16-digit 64-bit value was simply cut off with no
	// way to widen it. Values now start wide enough for the 64-bit form and the
	// pane scrolls sideways instead of clipping when the dock is narrower.
	const QFontMetrics mono(monoFont());
	// The minimum is the width of a 64-bit value plus the padding the cell eats,
	// so the widest thing this pane shows still fits at that minimum.
	installColumnFit(tree_, 1,
	                 mono.horizontalAdvance(QStringLiteral("0x0000000000000000")) + kCellPadding);
	// Sized for a register name, not for "General Purpose": the category rows are
	// first-column-spanned, so they never need column 0 to hold them, and sizing
	// for them cost ~25px that the value column then had to scroll for.
	tree_->header()->resizeSection(
		0, tree_->indentation() + mono.horizontalAdvance(QStringLiteral("Register")) + kCellPadding);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(tree_);
}

void RegistersPane::refresh() {
	if (!session_) return;

	const Registers_x86_64& r = session_->registers();
	const bool is64 = session_->is64bit();
	const QFont mono = monoFont();
	const int gpDigits = is64 ? 16 : 8; // 64- vs 32-bit value width

	tree_->clear();

	// A bold, non-value header row that spans both columns.
	auto addCategory = [this](const QString& title) {
		auto* cat = new QTreeWidgetItem(tree_, {title});
		QFont f = cat->font(0);
		f.setBold(true);
		cat->setFont(0, f);
		cat->setFirstColumnSpanned(true);
		cat->setFlags(Qt::ItemIsEnabled); // not selectable/editable
		return cat;
	};

	auto addReg = [&mono](QTreeWidgetItem* cat, const QString& name, uint64_t value, int digits) {
		auto* item = new QTreeWidgetItem(cat);
		const QString text = QString("0x%1").arg(value, digits, 16, QLatin1Char('0'));
		item->setText(0, name);
		item->setText(1, text);
		item->setFont(1, mono);
		// The full value on hover, for when the column is narrower than 16 digits.
		item->setToolTip(0, name + QLatin1Char(' ') + text);
		item->setToolTip(1, text);
	};

	// General purpose — renamed to the 64-bit set when a 64-bit target is loaded.
	auto* gp = addCategory(tr("General Purpose"));
	const std::array<std::pair<const char*, uint64_t>, 8> gp32 = {{
		{"eax", r.rax}, {"ebx", r.rbx}, {"ecx", r.rcx}, {"edx", r.rdx},
		{"esi", r.rsi}, {"edi", r.rdi}, {"ebp", r.rbp}, {"esp", r.rsp},
	}};
	static constexpr std::array<const char*, 8> gp64 = {
		"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
	};
	for (int i = 0; i < 8; ++i)
		addReg(gp, QString::fromLatin1(is64 ? gp64[i] : gp32[i].first), gp32[i].second, gpDigits);

	// Instruction pointer.
	auto* ip = addCategory(tr("Instruction Pointer"));
	addReg(ip, is64 ? QStringLiteral("rip") : QStringLiteral("eip"), r.rip, gpDigits);

	// Segment registers (16-bit selectors).
	auto* seg = addCategory(tr("Segment"));
	const std::pair<const char*, uint64_t> segs[] = {
		{"cs", r.cs}, {"ds", r.ds}, {"ss", r.ss},
		{"es", r.es}, {"fs", r.fs}, {"gs", r.gs},
	};
	for (const auto& [name, value] : segs)
		addReg(seg, QString::fromLatin1(name), value, 4);

	// Flags.
	auto* fl = addCategory(tr("Flags"));
	addReg(fl, QStringLiteral("flags"), r.flags, 2);

	tree_->expandAll();
}

} // namespace gui
