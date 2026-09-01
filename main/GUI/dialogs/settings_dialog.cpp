#include "settings_dialog.h"

#include <QCheckBox>
#include <QComboBox>
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

	auto* uiGroup = new QGroupBox(tr("Appearance"), this);
	auto* uiForm = new QFormLayout(uiGroup);
	theme_ = new QComboBox(uiGroup);
	theme_->addItem(tr("Dark"), QStringLiteral("dark"));
	theme_->addItem(tr("Light"), QStringLiteral("light"));
	theme_->setCurrentIndex(current.theme == QLatin1String("light") ? 1 : 0);
	uiForm->addRow(tr("Theme"), theme_);

	auto* aiGroup = new QGroupBox(tr("AI assistant (opt-in)"), this);
	auto* form = new QFormLayout(aiGroup);

	aiEnabled_ = new QCheckBox(tr("Enable AI chat pane"), aiGroup);
	aiEnabled_->setChecked(current.aiEnabled);
	form->addRow(aiEnabled_);

	keyFromEnv_ = current.aiKeyFromEnv;
	envKey_ = current.aiApiKey;

	apiKey_ = new QLineEdit(keyFromEnv_ ? QString() : current.aiApiKey, aiGroup);
	apiKey_->setEchoMode(QLineEdit::Password);
	apiKey_->setPlaceholderText(tr("stored locally; used by your AI backend"));
	form->addRow(tr("API key"), apiKey_);

	// Say where the key ends up rather than leaving it to be discovered. The two
	// branches are the two storage paths in AppSettings, and this is the only
	// place the user gets told which one is in force.
	auto* keyNote = new QLabel(aiGroup);
	keyNote->setWordWrap(true);
	keyNote->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 11px;"));
	if (keyFromEnv_) {
		apiKey_->setReadOnly(true);
		apiKey_->setPlaceholderText(tr("supplied by %1")
			.arg(QLatin1String(AppSettings::kApiKeyEnvVar)));
		keyNote->setText(tr("The key is coming from the %1 environment variable and is "
		                    "not written to disk. Unset it to store one here instead.")
			.arg(QLatin1String(AppSettings::kApiKeyEnvVar)));
	} else {
		const QString store = AppSettings::storeLocation();
		keyNote->setText(store.isEmpty()
			? tr("Saved in plain text in the registry under HKCU. Set %1 in the "
			     "environment instead to keep it off disk.")
				.arg(QLatin1String(AppSettings::kApiKeyEnvVar))
			: tr("Saved in plain text in %1 (owner-readable only). Set %2 in the "
			     "environment instead to keep it off disk.")
				.arg(store, QLatin1String(AppSettings::kApiKeyEnvVar)));
	}
	form->addRow(QString(), keyNote);

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
		// An environment-supplied key stays visible but never editable here.
		apiKey_->setEnabled(on && !keyFromEnv_);
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
	layout->addWidget(uiGroup);
	layout->addWidget(aiGroup);
	layout->addWidget(note);
	layout->addStretch();
	layout->addWidget(buttons);
}

AppSettings SettingsDialog::settings() const {
	AppSettings s;
	s.theme = theme_->currentData().toString();
	s.aiEnabled = aiEnabled_->isChecked();
	// Carry the environment key straight through: the field was never populated
	// with it, so reading the widget back would silently blank the key.
	s.aiKeyFromEnv = keyFromEnv_;
	s.aiApiKey = keyFromEnv_ ? envKey_ : apiKey_->text();
	s.aiModel = model_->text();
	s.aiEndpoint = endpoint_->text();
	s.aiContextLines = contextLines_->value();
	return s;
}

} // namespace gui
