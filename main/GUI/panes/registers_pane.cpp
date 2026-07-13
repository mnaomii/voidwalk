#include "registers_pane.h"

#include <QFontDatabase>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

#include <utility>

namespace gui {

RegistersPane::RegistersPane(QWidget* parent) : QWidget(parent) {
	table_ = new QTableWidget(0, 2, this);
	table_->setHorizontalHeaderLabels({tr("Register"), tr("Value")});
	table_->verticalHeader()->setVisible(false);
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->horizontalHeader()->setStretchLastSection(true);
	table_->setAlternatingRowColors(true);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(table_);
}

void RegistersPane::refresh() {
	if (!session_) return;

	const Registers& r = session_->registers();
	const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	// General-purpose set + eip, then the segment registers. Order matches the
	// pane's contract: eax..esp, eip, then cs/ds/ss/es/fs/gs, then flags.
	const std::pair<const char*, uint64_t> gpRows[] = {
		{"eax", r.eax}, {"ebx", r.ebx}, {"ecx", r.ecx}, {"edx", r.edx},
		{"esi", r.esi}, {"edi", r.edi}, {"ebp", r.ebp}, {"esp", r.esp},
		{"eip", r.eip},
		{"cs", r.cs}, {"ds", r.ds}, {"ss", r.ss}, {"es", r.es},
		{"fs", r.fs}, {"gs", r.gs},
	};
	const int gpCount = static_cast<int>(sizeof(gpRows) / sizeof(gpRows[0]));

	table_->setRowCount(gpCount + 1); // + flags row

	int row = 0;
	for (const auto& [name, value] : gpRows) {
		table_->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(name)));
		auto* valueItem = new QTableWidgetItem(QString("0x%1").arg(value, 16, 16, QLatin1Char('0')));
		valueItem->setFont(mono);
		table_->setItem(row, 1, valueItem);
		++row;
	}

	table_->setItem(row, 0, new QTableWidgetItem(QStringLiteral("flags")));
	auto* flagsItem = new QTableWidgetItem(QString("0x%1").arg(r.flags, 2, 16, QLatin1Char('0')));
	flagsItem->setFont(mono);
	table_->setItem(row, 1, flagsItem);
}

} // namespace gui
