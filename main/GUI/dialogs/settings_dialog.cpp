#include "settings_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace gui {

SettingsDialog::SettingsDialog(const AppSettings& current, QWidget* parent)
	: QDialog(parent) {
	setWindowTitle(tr("Settings"));
	setModal(true);

	auto* aiGroup = new QGroupBox(tr("AI assistant (opt-in)"), this);
	auto* form = new QFormLayout(aiGroup);

	aiEnabled_ = new QCheckBox(tr("Enable AI chat pane"), aiGroup);
	aiEnabled_->setChecked(current.aiEnabled);
	form->addRow(aiEnabled_);

	apiKey_ = new QLineEdit(current.aiApiKey, aiGroup);
	apiKey_->setEchoMode(QLineEdit::Password);
	apiKey_->setPlaceholderText(tr("stored locally; used by your AI backend"));
	form->addRow(tr("API key"), apiKey_);

	model_ = new QLineEdit(current.aiModel, aiGroup);
	form->addRow(tr("Model"), model_);

	endpoint_ = new QLineEdit(current.aiEndpoint, aiGroup);
	form->addRow(tr("Endpoint"), endpoint_);

	contextLines_ = new QSpinBox(aiGroup);
	contextLines_->setRange(0, 5000);
	contextLines_->setValue(current.aiContextLines);
	contextLines_->setToolTip(tr("Disassembly lines sent as context per question"));
	form->addRow(tr("Context lines"), contextLines_);

	// Gate the credential fields on the enable toggle.
	auto sync = [this]() {
		const bool on = aiEnabled_->isChecked();
		apiKey_->setEnabled(on);
		model_->setEnabled(on);
		endpoint_->setEnabled(on);
		contextLines_->setEnabled(on);
	};
	connect(aiEnabled_, &QCheckBox::toggled, this, [sync](bool) { sync(); });
	sync();

	auto* note = new QLabel(
		tr("The disassembler wires the pane to your AI backend; the model "
		   "integration itself is provided separately."),
		this);
	note->setWordWrap(true);
	note->setStyleSheet(QStringLiteral("color: palette(mid);"));

	auto* buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(aiGroup);
	layout->addWidget(note);
	layout->addStretch();
	layout->addWidget(buttons);
}

AppSettings SettingsDialog::settings() const {
	AppSettings s;
	s.aiEnabled = aiEnabled_->isChecked();
	s.aiApiKey = apiKey_->text();
	s.aiModel = model_->text();
	s.aiEndpoint = endpoint_->text();
	s.aiContextLines = contextLines_->value();
	return s;
}

} // namespace gui
