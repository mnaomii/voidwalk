#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include <QFile>
#include <QFileDevice>
#include <QSettings>
#include <QString>

namespace gui {

// Persisted app settings (QSettings, HKCU\Software\voidwalk\voidwalk-gui on
// Windows, ~/.config/voidwalk/voidwalk-gui.conf on Linux/macOS).
// The AI chat pane is opt-in: it stays hidden and its toolbar toggle disabled
// until aiEnabled is set through the Settings dialog.
//
// Credential handling. There is no OS credential store wired up yet
// (QtKeychain/DPAPI/libsecret would each pull in a dependency the project does
// not otherwise need), so the API key has two homes, in this order of
// precedence:
//
//   1. the VOIDWALK_AI_API_KEY environment variable — read at load(), marked
//      with aiKeyFromEnv, and never written back to disk;
//   2. QSettings — plaintext, readable by anything running as this user.
//
// (2) is a real exposure, not a hypothetical one. save() narrows the settings
// file to owner-only on POSIX, which reduces the blast radius but is a
// mitigation rather than a fix; the Settings dialog states the exposure and
// points at (1). Replacing (2) with a credential store is the actual fix and is
// tracked as H4 in AUDIT.md.
struct AppSettings {
	QString theme = QStringLiteral("dark");                  // "dark" | "light"

	bool aiEnabled = false;
	QString aiApiKey;                                        // never logged, PasswordEcho in the dialog
	bool aiKeyFromEnv = false;                               // came from the environment: do not persist
	QString aiModel = QStringLiteral("claude-sonnet-5");
	QString aiEndpoint = QStringLiteral("https://api.anthropic.com/v1/messages");
	int aiContextLines = 200;                                // disassembly lines sent as context

	// The environment variable that overrides (and keeps off disk) the stored key.
	static constexpr const char* kApiKeyEnvVar = "VOIDWALK_AI_API_KEY";

	// Where the plaintext store lives, for the warning the dialog shows. Empty on
	// platforms where the backend is not a file (the Windows registry).
	static QString storeLocation() {
		QSettings s(QStringLiteral("voidwalk"), QStringLiteral("voidwalk-gui"));
#ifdef Q_OS_WIN
		return QString();
#else
		return s.fileName();
#endif
	}

	static AppSettings load() {
		QSettings s(QStringLiteral("voidwalk"), QStringLiteral("voidwalk-gui"));
		AppSettings a;
		a.theme = s.value(QStringLiteral("ui/theme"), a.theme).toString();
		a.aiEnabled = s.value(QStringLiteral("ai/enabled"), a.aiEnabled).toBool();
		a.aiApiKey = s.value(QStringLiteral("ai/apiKey"), a.aiApiKey).toString();
		a.aiModel = s.value(QStringLiteral("ai/model"), a.aiModel).toString();
		a.aiEndpoint = s.value(QStringLiteral("ai/endpoint"), a.aiEndpoint).toString();
		a.aiContextLines = s.value(QStringLiteral("ai/contextLines"), a.aiContextLines).toInt();

		// The environment wins over the stored copy, and never becomes one.
		const QString envKey = qEnvironmentVariable(kApiKeyEnvVar);
		if (!envKey.isEmpty()) {
			a.aiApiKey = envKey;
			a.aiKeyFromEnv = true;
		}
		return a;
	}

	void save() const {
		QSettings s(QStringLiteral("voidwalk"), QStringLiteral("voidwalk-gui"));
		s.setValue(QStringLiteral("ui/theme"), theme);
		s.setValue(QStringLiteral("ai/enabled"), aiEnabled);
		// A key supplied through the environment is not ours to persist: writing it
		// out would defeat the whole point of putting it there. Clear any stored
		// copy at the same time, so setting the variable once also cleans up a key
		// saved before it existed.
		if (aiKeyFromEnv)
			s.remove(QStringLiteral("ai/apiKey"));
		else
			s.setValue(QStringLiteral("ai/apiKey"), aiApiKey);
		s.setValue(QStringLiteral("ai/model"), aiModel);
		s.setValue(QStringLiteral("ai/endpoint"), aiEndpoint);
		s.setValue(QStringLiteral("ai/contextLines"), aiContextLines);
		s.sync(); // the file has to exist before its permissions can be set
		restrictStorePermissions(s);
	}

private:
	// QSettings creates the file with the process umask — 0644 on a stock Linux,
	// i.e. world-readable, for a file that can hold an API key. Narrow it to the
	// owner. No-op on Windows, where the backend is the registry and the ACL
	// already follows HKCU.
	static void restrictStorePermissions(const QSettings& s) {
#ifdef Q_OS_WIN
		Q_UNUSED(s);
#else
		const QString path = s.fileName();
		if (path.isEmpty() || !QFile::exists(path)) return;
		QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
	}
};

} // namespace gui

#endif
