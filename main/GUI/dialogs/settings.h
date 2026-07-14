#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include <QSettings>
#include <QString>

namespace gui {

// Persisted app settings (QSettings, HKCU\Software\voidwalk\voidwalk-gui).
// The AI chat pane is opt-in: it stays hidden and its toolbar toggle disabled
// until aiEnabled is set through the Settings dialog.
struct AppSettings {
	QString theme = QStringLiteral("dark");                  // "dark" | "light"

	bool aiEnabled = false;
	QString aiApiKey;                                        // never logged, PasswordEcho in the dialog
	QString aiModel = QStringLiteral("claude-sonnet-5");
	QString aiEndpoint = QStringLiteral("https://api.anthropic.com/v1/messages");
	int aiContextLines = 200;                                // disassembly lines sent as context

	static AppSettings load() {
		QSettings s(QStringLiteral("voidwalk"), QStringLiteral("voidwalk-gui"));
		AppSettings a;
		a.theme = s.value(QStringLiteral("ui/theme"), a.theme).toString();
		a.aiEnabled = s.value(QStringLiteral("ai/enabled"), a.aiEnabled).toBool();
		a.aiApiKey = s.value(QStringLiteral("ai/apiKey"), a.aiApiKey).toString();
		a.aiModel = s.value(QStringLiteral("ai/model"), a.aiModel).toString();
		a.aiEndpoint = s.value(QStringLiteral("ai/endpoint"), a.aiEndpoint).toString();
		a.aiContextLines = s.value(QStringLiteral("ai/contextLines"), a.aiContextLines).toInt();
		return a;
	}

	void save() const {
		QSettings s(QStringLiteral("voidwalk"), QStringLiteral("voidwalk-gui"));
		s.setValue(QStringLiteral("ui/theme"), theme);
		s.setValue(QStringLiteral("ai/enabled"), aiEnabled);
		s.setValue(QStringLiteral("ai/apiKey"), aiApiKey);
		s.setValue(QStringLiteral("ai/model"), aiModel);
		s.setValue(QStringLiteral("ai/endpoint"), aiEndpoint);
		s.setValue(QStringLiteral("ai/contextLines"), aiContextLines);
	}
};

} // namespace gui

#endif
