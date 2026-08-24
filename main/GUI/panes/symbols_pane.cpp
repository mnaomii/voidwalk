#include "symbols_pane.h"

#include "../theme/theme.h"

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
	filter_->setClearButtonEnabled(true);
	connect(filter_, &QLineEdit::textChanged, this, [this] { rebuild(); });
	layout->addWidget(filter_);

	tree_ = new QTreeWidget(this);
	tree_->setObjectName(QStringLiteral("symbolsTree"));
	tree_->setColumnCount(2);
	tree_->setHeaderHidden(true);
	tree_->setRootIsDecorated(true);
	tree_->setUniformRowHeights(true);
	tree_->setFont(monoFont());
	tree_->setFrameShape(QFrame::NoFrame);
	tree_->setSelectionMode(QAbstractItemView::SingleSelection);
	tree_->setIndentation(0); // groups carry their own left padding
	tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	tree_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	layout->addWidget(tree_, 1);

	connect(tree_, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* item, int) {
		if (!item || !item->data(0, kAddrRole).isValid()) return; // group header
		emit navigateRequested(item->data(0, kAddrRole).toULongLong());
	});
	// Single click navigates too: the sidebar is a navigator, not a file picker,
	// so requiring a double click would put a beat between intent and jump.
	connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
		if (!item || !item->data(0, kAddrRole).isValid()) return;
		emit navigateRequested(item->data(0, kAddrRole).toULongLong());
	});

	countLabel_ = count;
}

void SymbolsPane::setTheme(const Theme& theme) {
	theme_ = theme;
	rebuild();
}

void SymbolsPane::refresh() {
	symbols_ = session_ ? collectSymbols(*session_) : std::vector<SymbolInfo>{};
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
			row->setSizeHint(0, QSize(0, kRowHeight));
			row->setToolTip(0, QStringLiteral("0x%1")
				.arg(sym->addr, 8, 16, QLatin1Char('0')).toUpper());
			++shown;
		}
	}

	if (countLabel_)
		countLabel_->setText(shown > 0 ? QString::number(shown) : QString());
}

} // namespace gui
