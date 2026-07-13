#include "disassembly_pane.h"

#include <QAbstractItemView>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace gui {

DisassemblyPane::DisassemblyPane(QWidget* parent)
	: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);

	banner_ = new QLabel(this);
	banner_->setWordWrap(true);
	banner_->setStyleSheet(QStringLiteral("color: palette(mid);"));
	banner_->setVisible(false);
	layout->addWidget(banner_);

	table_ = new QTableWidget(this);
	table_->setColumnCount(3);
	table_->setHorizontalHeaderLabels({tr("Address"), tr("Bytes"), tr("Instruction")});
	table_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	// Only the Instruction column is meant to be edited; columns 0/1 are made
	// non-editable per-item in refresh() via item flags.
	table_->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::SelectedClicked
		| QAbstractItemView::EditKeyPressed);
	table_->verticalHeader()->setVisible(false);
	table_->horizontalHeader()->setStretchLastSection(true);
	table_->setAlternatingRowColors(true);
	layout->addWidget(table_);

	connect(table_, &QTableWidget::cellChanged, this, [this](int row, int col) {
		if (populating_) return; // programmatic fill in refresh(), not a user edit
		if (col != 2) return; // only the Instruction column is editable

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

		auto* addrItem = new QTableWidgetItem(QString("0x%1").arg(r.vaddr, 8, 16, QLatin1Char('0')));
		addrItem->setFlags(addrItem->flags() & ~Qt::ItemIsEditable);
		table_->setItem(i, 0, addrItem);

		auto* bytesItem = new QTableWidgetItem(QString::fromStdString(r.bytes));
		bytesItem->setFlags(bytesItem->flags() & ~Qt::ItemIsEditable);
		table_->setItem(i, 1, bytesItem);

		// Instruction column keeps Qt::ItemIsEditable (the default flag).
		auto* textItem = new QTableWidgetItem(QString::fromStdString(r.text));
		table_->setItem(i, 2, textItem);
	}

	table_->resizeColumnsToContents();
	populating_ = false;
}

std::vector<std::pair<std::size_t, std::string>> DisassemblyPane::pendingEdits() const {
	return edits_;
}

void DisassemblyPane::clearPendingEdits() {
	edits_.clear();
}

} // namespace gui
