#ifndef GUI_MEMORY_PANE_H
#define GUI_MEMORY_PANE_H

#include "../model/gui_session.h"

#include <QString>
#include <QWidget>
#include <cstdint>

class QPlainTextEdit;
class QLineEdit;
class QComboBox;

namespace gui {

// Hex-dump view of the loaded file. Reads raw bytes via Session::bytes(); the
// addresses shown are *file offsets* (the memory pane hexdumps AddressSpace
// directly), which differ from the disassembly pane's virtual addresses — same
// known inconsistency as the TUI, rational until a debugger provides a loaded
// image. A "Go to" box seeks to an offset; the view renders 16 bytes/row with
// an ASCII gutter.
//
// A "Section" dropdown jumps to any section the core registered
// (Session::sections()) or, via "See all", renders the whole file at once
// instead of the 2 KiB scrolling window a plain seek shows.
class MemoryPane : public QWidget {
	Q_OBJECT
public:
	explicit MemoryPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }

	// Seek so `offset` is at the top of the view, then repaint. Leaves "see all"
	// mode (an explicit offset is a windowed seek, not a whole-file dump).
	void gotoOffset(std::uint64_t offset);

public slots:
	void refresh();

private:
	// What a dropdown entry does, stored in the item's Qt::UserRole.
	enum SectionKind { KindNone = 0, KindAll, KindSection };

	void populateSections();          // rebuild the dropdown from session sections
	void onSectionActivated(int idx); // user picked a dropdown entry

	Session* session_ = nullptr;
	QPlainTextEdit* view_ = nullptr;
	QLineEdit* gotoBox_ = nullptr;
	QComboBox* sectionBox_ = nullptr;
	std::uint64_t top_ = 0;   // file offset of the first rendered row
	bool showAll_ = false;    // "See all": render the whole file, not a window
	QString lastFile_;        // rebuild the dropdown only when the binary changes
};

} // namespace gui

#endif
