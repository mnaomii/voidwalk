#ifndef GUI_WELCOME_WIDGET_H
#define GUI_WELCOME_WIDGET_H

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
	void openRequested();              // "Open binary…" button
	void fileDropped(const QString& path);

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
};

} // namespace gui

#endif
