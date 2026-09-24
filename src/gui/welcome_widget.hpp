#pragma once
#include <QWidget>

class QLabel;

namespace gui {

// Empty state shown as the central widget until a binary is loaded.
// Styled entirely by the theme QSS via object names (welcomeBadge,
// welcomeTitle, welcomeHint, welcomeChip). Accepts drag-and-dropped files.
class WelcomeWidget : public QWidget {
	Q_OBJECT
public:
	explicit WelcomeWidget(QWidget* parent = nullptr);

signals:
	void openRequested();                   // "Open binary…" button was clicked
	void fileDropped(const QString& path);  // a local file was dropped on the widget

protected:
	// Accept a drag only when it carries local file URLs, so the cursor does not
	// promise a drop the widget would then ignore.
	void dragEnterEvent(QDragEnterEvent* event) override;

	// Emits fileDropped with the first local path in the drop.
	void dropEvent(QDropEvent* event) override;
};

} // namespace gui

