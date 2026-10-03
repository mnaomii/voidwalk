#include "gui/panes/symbols_pane.hpp"

#include "gui/panes/column_fit.hpp"
#include "gui/theme/icons.hpp"
#include "gui/theme/theme.hpp"

#include <QAction>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace gui {

namespace {
constexpr int kRowHeight = 25;      // matches the disassembly row rhythm
constexpr int kAddrRole = Qt::UserRole + 1;
constexpr int kStringRole = Qt::UserRole + 2;   // true: kAddrRole is a file offset
} // namespace

SymbolsPane::SymbolsPane(QWidget* parent) : QWidget(parent) {
	setObjectName(QStringLiteral("symbolsPane"));
	setMinimumWidth(180);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	// Header row: label left, count right — the count is the only number here,
	// so it earns the right edge instead of a parenthesis in the label.
	auto* head = new QHBoxLayout;
	head->setContentsMargins(0, 0, 0, 0);
	header_ = new QLabel(tr("SYMBOLS"), this);
	header_->setObjectName(QStringLiteral("symbolsHeader"));
	auto* count = new QLabel(this);
	count->setObjectName(QStringLiteral("symbolsCount"));
	count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	head->addWidget(header_);
	head->addStretch(1);
	head->addWidget(count);
	layout->addLayout(head);

	filter_ = new QLineEdit(this);
	filter_->setObjectName(QStringLiteral("symbolsFilter"));
	filter_->setPlaceholderText(tr("Filter"));
	// Our own clear button rather than setClearButtonEnabled(), whose icon comes from
	// the desktop's icon theme (a red disc on GNOME) instead of the app's flat set.
	clearAct_ = filter_->addAction(QIcon(), QLineEdit::TrailingPosition);
	clearAct_->setVisible(false);
	connect(clearAct_, &QAction::triggered, filter_, &QLineEdit::clear);
	connect(filter_, &QLineEdit::textChanged, this, [this](const QString& text) {
		clearAct_->setVisible(!text.isEmpty());
		rebuild();
	});
	// The gap around the field lives here, not in the QSS: QLineEdit places its side
	// buttons by the widget's full rect, so a QSS margin pushed the button past the border.
	auto* filterRow = new QHBoxLayout;
	filterRow->setContentsMargins(10, 8, 10, 8);
	filterRow->addWidget(filter_);
	layout->addLayout(filterRow);

	tree_ = new QTreeWidget(this);
	tree_->setObjectName(QStringLiteral("symbolsTree"));
	tree_->setColumnCount(2);
	// The header used to be hidden, which also hid the only handle for resizing
	// the two columns — and with NAME stretched and ADDR sized to its contents,
	// a long string literal or sub_xxxxxx name was clipped in a narrow sidebar
	// with no way to trade width between them. It is a quiet strip (the QSS gives
	// it the same 10px faint caption as the disassembly's) and it earns its row.
	tree_->setHeaderLabels({tr("NAME"), tr("ADDR")});
	tree_->setRootIsDecorated(true);
	tree_->setUniformRowHeights(true);
	tree_->setFont(monoFont());
	tree_->setFrameShape(QFrame::NoFrame);
	tree_->setSelectionMode(QAbstractItemView::SingleSelection);
	tree_->setTextElideMode(Qt::ElideRight);
	tree_->setIndentation(0); // groups carry their own left padding
	tree_->header()->setHighlightSections(false);

	// NAME takes the slack; ADDR starts at the width of the addresses it holds.
	const QFontMetrics mono(monoFont());
	installColumnFit(tree_, 0, mono.horizontalAdvance(QStringLiteral("sub_000000")) + kCellPadding);
	tree_->header()->resizeSection(
		1, mono.horizontalAdvance(QStringLiteral("00000000")) + kCellPadding);

	tree_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	layout->addWidget(tree_, 1);

	auto activate = [this](QTreeWidgetItem* item) {
		if (!item || !item->data(0, kAddrRole).isValid()) return; // group header
		const uint64_t addr = item->data(0, kAddrRole).toULongLong();
		if (item->data(0, kStringRole).toBool()) emit memoryRequested(addr);
		else emit navigateRequested(addr);
	};
	connect(tree_, &QTreeWidget::itemActivated, this, [activate](QTreeWidgetItem* item, int) { activate(item); });
	// Single click navigates too: the sidebar is a navigator, not a file picker,
	// so requiring a double click would put a beat between intent and jump.
	connect(tree_, &QTreeWidget::itemClicked, this, [activate](QTreeWidgetItem* item, int) { activate(item); });

	countLabel_ = count;
}

void SymbolsPane::setTheme(const Theme& theme) {
	theme_ = theme;
	clearAct_->setIcon(Icons::icon(QStringLiteral("clear")));   // re-fetched: a theme switch recolors the set
	rebuild();
}

void SymbolsPane::refresh() {
	if (!session_ || !session_->loaded()) {
		symbols_.clear();
		scanning_ = false;
		rebuild();
		return;
	}

	symbols_.clear();

	// The sweep is still running, so most rows do not exist yet: a scan started now
	// would walk a fraction of the binary and be thrown away by the one
	// MainWindow starts on the decode's final tick. Leave "Scanning…" up instead.
	if (session_->isDecoding()) {
		scanning_ = true;
		rebuild();
		return;
	}

	// Synchronous, on the UI thread. A Snapshot is no longer needed for thread
	// safety, but it stays: it is the read-only row view collectSymbols() takes,
	// and it pins the published row count so the scan cannot run past it.
	symbols_ = collectSymbols(session_->snapshot());
	scanning_ = false;
	rebuild();
}

QTreeWidgetItem* SymbolsPane::addGroup(const QString& title, int count) {
	auto* group = new QTreeWidgetItem(tree_);
	group->setText(0, title);
	group->setForeground(0, theme_.textFaint);
	group->setText(1, count > 0 ? QString::number(count) : QString());
	group->setForeground(1, theme_.textGhost);
	QFont f = group->font(0);
	f.setFamilies(QStringList{QStringLiteral("Segoe UI"), QStringLiteral("system-ui")});
	f.setPointSizeF(f.pointSizeF() * 0.85);
	f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
	group->setFont(0, f);
	group->setFont(1, f);
	group->setFlags(Qt::ItemIsEnabled); // a heading, not a target
	group->setSizeHint(0, QSize(0, 22));
	group->setExpanded(true);
	return group;
}

void SymbolsPane::rebuild() {
	tree_->clear();
	const QString needle = filter_ ? filter_->text().trimmed() : QString();

	if (scanning_) {
		// Say so rather than showing an empty sidebar, which reads as "this binary
		// has no symbols" instead of "the scan hasn't finished".
		auto* note = new QTreeWidgetItem(tree_);
		note->setText(0, tr("Scanning…"));
		note->setForeground(0, theme_.textGhost);
		note->setFlags(Qt::ItemIsEnabled);
		if (countLabel_) countLabel_->clear();
		return;
	}

	struct Bucket {
		SymbolInfo::Kind kind;
		QString title;
		QColor color;
	};
	const Bucket buckets[] = {
		{SymbolInfo::Kind::Function, tr(".TEXT"), theme_.text},
		{SymbolInfo::Kind::Import, tr("IMPORTS"), theme_.text},
		{SymbolInfo::Kind::String, tr("STRINGS"), theme_.synString},
	};

	int shown = 0;
	for (const Bucket& bucket : buckets) {
		std::vector<const SymbolInfo*> matches;
		for (const SymbolInfo& sym : symbols_) {
			if (sym.kind != bucket.kind) continue;
			const QString name = QString::fromStdString(sym.name);
			if (!needle.isEmpty() && !name.contains(needle, Qt::CaseInsensitive))
				continue;
			matches.push_back(&sym);
		}
		if (matches.empty()) continue; // an empty group is noise, not information

		QTreeWidgetItem* group = addGroup(bucket.title, static_cast<int>(matches.size()));
		for (const SymbolInfo* sym : matches) {
			auto* row = new QTreeWidgetItem(group);
			row->setText(0, QString::fromStdString(sym->name));
			row->setForeground(0, bucket.color);
			row->setText(1, QString::fromStdString(sym->detail));
			row->setForeground(1, theme_.textGhost);
			row->setData(0, kAddrRole, QVariant::fromValue<qulonglong>(sym->addr));
			row->setData(0, kStringRole, sym->kind == SymbolInfo::Kind::String);
			row->setSizeHint(0, QSize(0, kRowHeight));
			// Name as well as address: the name is what gets elided in a narrow
			// sidebar, and a string literal's tail is the half worth reading.
			const QString addr = (sym->kind == SymbolInfo::Kind::String ? tr("file offset ") : QString())
				+ QStringLiteral("0x%1").arg(sym->addr, 8, 16, QLatin1Char('0')).toUpper();
			row->setToolTip(0, QString::fromStdString(sym->name) + QStringLiteral("\n") + addr);
			row->setToolTip(1, addr);
			++shown;
		}
	}

	if (countLabel_)
		countLabel_->setText(shown > 0 ? QString::number(shown) : QString());
}

} // namespace gui
