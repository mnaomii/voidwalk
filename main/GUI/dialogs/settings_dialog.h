#ifndef GUI_SETTINGS_DIALOG_H
#define GUI_SETTINGS_DIALOG_H

#include "settings.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace gui {

// Settings dialog. Appearance (theme) plus the opt-in AI toggle that shows the
// AI chat pane; the model/endpoint/key fields are captured here and handed to
// whatever AiBackend you wire in (the GUI does not use them itself). Editing is
// non-destructive: values are read from AppSettings on open and written back
// only on accept().
class SettingsDialog : public QDialog {
	Q_OBJECT
public:
	explicit SettingsDialog(const AppSettings& current, QWidget* parent = nullptr);

	// Valid after exec() returns Accepted.
	AppSettings settings() const;

private:
	QComboBox* theme_ = nullptr;
	QCheckBox* aiEnabled_ = nullptr;
	QLineEdit* apiKey_ = nullptr;
	QLineEdit* model_ = nullptr;
	QLineEdit* endpoint_ = nullptr;
	QSpinBox* contextLines_ = nullptr;
};

} // namespace gui

#endif
