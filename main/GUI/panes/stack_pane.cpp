#include "stack_pane.h"

#include "../theme/theme.h"

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
	table_->horizontalHeader()->setStretchLastSection(true);

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
		table_->setItem(row, 0, new QTableWidgetItem(
			QString("0x%1").arg(static_cast<uint64_t>(i) * 8, 8, 16, QLatin1Char('0'))));

		QString valueText = QString("0x%1").arg(st[static_cast<std::size_t>(i)], 16, 16, QLatin1Char('0'));
		if (i == count - 1) valueText += QStringLiteral(" <- esp");

		auto* valueItem = new QTableWidgetItem(valueText);
		valueItem->setFont(mono);
		table_->setItem(row, 1, valueItem);
		++row;
	}
}

} // namespace gui
