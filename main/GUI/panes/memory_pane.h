#ifndef GUI_MEMORY_PANE_H
#define GUI_MEMORY_PANE_H

#include "../model/gui_session.h"

#include <QWidget>
#include <cstdint>

class QPlainTextEdit;
class QLineEdit;

namespace gui {

// Hex-dump view of the loaded file. Reads raw bytes via Session::bytes(); the
// addresses shown are *file offsets* (the memory pane hexdumps AddressSpace
// directly), which differ from the disassembly pane's virtual addresses — same
// known inconsistency as the TUI, rational until a debugger provides a loaded
// image. A "Go to" box seeks to an offset; the view renders 16 bytes/row with
// an ASCII gutter.
class MemoryPane : public QWidget {
	Q_OBJECT
public:
	explicit MemoryPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }

	// Seek so `offset` is at the top of the view, then repaint.
	void gotoOffset(std::uint64_t offset);

public slots:
	void refresh();

private:
	Session* session_ = nullptr;
	QPlainTextEdit* view_ = nullptr;
	QLineEdit* gotoBox_ = nullptr;
	std::uint64_t top_ = 0; // file offset of the first rendered row
};

} // namespace gui

#endif
