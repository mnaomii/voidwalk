#include "theme.h"

namespace gui {

QFont monoFont() {
	QFont f;
	// Best-first; Qt falls through to the next family that exists on the host.
	f.setFamilies({
		QStringLiteral("Cascadia Mono"),   // Windows 11 / VS
		QStringLiteral("Consolas"),        // Windows fallback
		QStringLiteral("SF Mono"),         // macOS
		QStringLiteral("Menlo"),           // macOS fallback
		QStringLiteral("DejaVu Sans Mono"),// Linux
	});
	f.setStyleHint(QFont::Monospace);
	f.setFixedPitch(true);
	f.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
	return f;
}

Theme Theme::dark() {
	Theme t;
	t.id = QStringLiteral("dark");
	t.bgBase = QColor("#16181d");
	t.bgPanel = QColor("#1c1f26");
	t.border = QColor("#2c2f38");
	t.control = QColor("#262a33");
	t.controlHover = QColor("#2e333e");
	t.controlBorder = QColor("#343945");
	t.text = QColor("#c9ced8");
	t.textBright = QColor("#e8eaf0");
	t.textMuted = QColor("#9aa0ab");
	t.textDim = QColor("#6b7280");
	t.textFaint = QColor("#565d6b");
	t.accent = QColor("#7d9cc0");
	t.accentText = QColor("#c9dbf0");
	t.accentBg = QColor("#223040");
	t.runBg = QColor("#2b3a4d");
	t.ok = QColor("#8fd0a0");
	t.synMnemonic = QColor("#7d9cc0");
	t.synJump = QColor("#f28fad");
	t.synRegister = QColor("#7fd1e8");
	t.synImmediate = QColor("#ffb454");
	t.synTarget = QColor("#8fd0a0");
	t.synPunct = QColor("#565d6b");
	return t;
}

Theme Theme::light() {
	Theme t;
	t.id = QStringLiteral("light");
	t.bgBase = QColor("#fdfcfb");
	t.bgPanel = QColor("#f4f2ef");
	t.border = QColor("#e2dfd9");
	t.control = QColor("#ffffff");
	t.controlHover = QColor("#f0eeea");
	t.controlBorder = QColor("#d8d5ce");
	t.text = QColor("#3a3d45");
	t.textBright = QColor("#22252c");
	t.textMuted = QColor("#6b6e77");
	t.textDim = QColor("#8a8d95");
	t.textFaint = QColor("#a5a8ae");
	t.accent = QColor("#4c6f96");
	t.accentText = QColor("#2e4a68");
	t.accentBg = QColor("#e2eaf3");
	t.runBg = QColor("#d5e2ef");
	t.ok = QColor("#3d8a55");
	t.synMnemonic = QColor("#4c6f96");
	t.synJump = QColor("#c04a6b");
	t.synRegister = QColor("#1a7f9c");
	t.synImmediate = QColor("#b06d10");
	t.synTarget = QColor("#3d8a55");
	t.synPunct = QColor("#a0a3aa");
	return t;
}

Theme Theme::byId(const QString& id) {
	return id == QLatin1String("light") ? light() : dark();
}

QHash<QString, QString> Theme::placeholderMap() const {
	const auto hex = [](const QColor& c) { return c.name(QColor::HexRgb); };
	return {
		{QStringLiteral("bgBase"), hex(bgBase)},
		{QStringLiteral("bgPanel"), hex(bgPanel)},
		{QStringLiteral("border"), hex(border)},
		{QStringLiteral("control"), hex(control)},
		{QStringLiteral("controlHover"), hex(controlHover)},
		{QStringLiteral("controlBorder"), hex(controlBorder)},
		{QStringLiteral("text"), hex(text)},
		{QStringLiteral("textBright"), hex(textBright)},
		{QStringLiteral("textMuted"), hex(textMuted)},
		{QStringLiteral("textDim"), hex(textDim)},
		{QStringLiteral("textFaint"), hex(textFaint)},
		{QStringLiteral("accent"), hex(accent)},
		{QStringLiteral("accentText"), hex(accentText)},
		{QStringLiteral("accentBg"), hex(accentBg)},
		{QStringLiteral("runBg"), hex(runBg)},
		{QStringLiteral("ok"), hex(ok)},
	};
}

} // namespace gui
