#pragma once
#include <QWidget>

class QLabel;

namespace gui {

// Empty state shown as the central widget until a binary is loaded.
// Styled entirely by the theme QSS via object names (welcomeBadge,
// welcomeTitle, welcomeHint, welcomeChip). Dropped files are MainWindow's to take.
class WelcomeWidget : public QWidget {
	Q_OBJECT
public:
	explicit WelcomeWidget(QWidget* parent = nullptr);

signals:
	void openRequested();                   // "Open binary…" button was clicked
};

} // namespace gui

