#include "stack_pane.h"

#include "column_fit.h"
#include "../theme/theme.h"

#include <QFontMetrics>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {

StackPane::StackPane(QWidget* parent) : QWidget(parent) {
	placeholder_ = new QLabel(this);
	placeholder_->setWordWrap(true);
	placeholder_->setAlignment(Qt::AlignCenter);

	table_ = new QTableWidget(0, 2, this);
	table_->setHorizontalHeaderLabels({tr("Address"), tr("Value")});
	table_->verticalHeader()->setVisible(false);
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->setTextElideMode(Qt::ElideRight);
	table_->setWordWrap(false);

	// Both columns draggable, Value taking the slack — a stretched last section
	// cannot be resized, and "0x0000000000000000 <- esp" is the widest thing in
	// this pane, so it was the first thing to get cut off in a narrow dock.
	const QFontMetrics mono(monoFont());
	// The minimum covers the value itself; the " <- esp" marker on the top entry is
	// a bonus that may elide (its tooltip and a drag both bring it back).
	installColumnFit(table_, 1,
	                 mono.horizontalAdvance(QStringLiteral("0x0000000000000000")) + kCellPadding);
	table_->horizontalHeader()->resizeSection(
		0, mono.horizontalAdvance(QStringLiteral("0x00000000")) + kCellPadding);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(placeholder_);
	layout->addWidget(table_);
}

void StackPane::refresh() {
	if (!session_) return;

	const auto& st = session_->stack();
	if (st.empty()) {
		placeholder_->setText(tr("Stack empty — simulation is WIP (fills once the debugger can execute instructions)."));
		placeholder_->setVisible(true);
		table_->setVisible(false);
		table_->setRowCount(0);
		return;
	}

	placeholder_->setVisible(false);
	table_->setVisible(true);

	const QFont mono = monoFont();
	const int count = static_cast<int>(st.size());
	table_->setRowCount(count);

	// Top of stack first: walk from the last element down to the first.
	int row = 0;
	for (int i = count - 1; i >= 0; --i) {
		const QString addrText =
			QString("0x%1").arg(static_cast<uint64_t>(i) * 8, 8, 16, QLatin1Char('0'));
		auto* addrItem = new QTableWidgetItem(addrText);
		addrItem->setToolTip(addrText);
		table_->setItem(row, 0, addrItem);

		QString valueText = QString("0x%1").arg(st[static_cast<std::size_t>(i)], 16, 16, QLatin1Char('0'));
		if (i == count - 1) valueText += QStringLiteral(" <- esp");

		auto* valueItem = new QTableWidgetItem(valueText);
		valueItem->setFont(mono);
		// The full value on hover, for when the column is dragged below its width.
		valueItem->setToolTip(valueText);
		table_->setItem(row, 1, valueItem);
		++row;
	}
}

} // namespace gui
