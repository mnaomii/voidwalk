#pragma once
#include "gui/session.hpp"

#include <QAbstractListModel>
#include <QWidget>
#include <cstdint>

class QListView;
class QLineEdit;
class QComboBox;

namespace gui {

// One row per 16 bytes of the loaded file, formatted only when the view asks for
// it - so the whole file is browsable for the cost of the rows on screen.
class HexModel : public QAbstractListModel {
public:
	using QAbstractListModel::QAbstractListModel;

	void setSession(Session* s) { session_ = s; }
	void reload();   // re-reads the file size; call when the binary changes

	int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : rows_; }
	QVariant data(const QModelIndex& index, int role) const override;

private:
	Session* session_ = nullptr;
	int rows_ = 0;
};

// Hex-dump view of the loaded file. Reads raw bytes via Session::bytes(); the
// addresses shown are *file offsets* (the memory pane hexdumps AddressSpace
// directly), which differ from the disassembly pane's virtual addresses — same
// known inconsistency as the TUI, rational until a debugger provides a loaded
// image. A "Go to" box seeks to an offset; each row is 16 bytes with an ASCII
// gutter, and a "Section" dropdown jumps to any section the core registered.
class MemoryPane : public QWidget {
	Q_OBJECT
public:
	explicit MemoryPane(QWidget* parent = nullptr);

	// Borrows `s`; the caller keeps ownership and must outlive this pane.
	void setSession(Session* s) { session_ = s; model_->setSession(s); }

	// Scrolls so the row holding `offset` is at the top, and selects it.
	void gotoOffset(std::uint64_t offset);

public slots:
	// Rebuilds the rows and the section dropdown when a new decode has started
	// (a re-open, even of the same path); otherwise does nothing - the view pulls
	// its rows on its own. Called from MainWindow::refreshAll().
	void refresh();

private:
	void populateSections();          // rebuild the dropdown from session sections

	Session* session_ = nullptr;
	HexModel* model_ = nullptr;
	QListView* view_ = nullptr;
	QLineEdit* gotoBox_ = nullptr;
	QComboBox* sectionBox_ = nullptr;
	std::uint64_t lastGen_ = ~0ull;   // Session::decodeGeneration() the rows belong to
};

} // namespace gui
