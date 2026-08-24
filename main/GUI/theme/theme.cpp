#include "theme.h"

#include <QFontDatabase>
#include <QStringList>

namespace gui {

namespace {
// Set by registerBundledFonts() to the family name Qt reports for the bundled
// TTF, so monoFont() asks for the exact family rather than guessing the name.
QString g_bundledMono;
} // namespace

void registerBundledFonts() {
	static const QStringList files = {
		QStringLiteral(":/fonts/JetBrainsMono-Regular.ttf"),
		QStringLiteral(":/fonts/JetBrainsMono-Medium.ttf"),
		QStringLiteral(":/fonts/JetBrainsMono-Bold.ttf"),
	};
	for (const QString& path : files) {
		const int id = QFontDatabase::addApplicationFont(path);
		if (id < 0) continue; // resource missing — host fonts still cover us
		const QStringList families = QFontDatabase::applicationFontFamilies(id);
		if (!families.isEmpty() && g_bundledMono.isEmpty())
			g_bundledMono = families.first();
	}
}

QFont monoFont() {
	QFont f;
	QStringList families;
	if (!g_bundledMono.isEmpty())
		families << g_bundledMono;      // bundled JetBrains Mono — the design target
	families
		<< QStringLiteral("JetBrains Mono")   // host install, if the user has one
		<< QStringLiteral("Cascadia Mono")    // Windows 11 / VS
		<< QStringLiteral("SF Mono")          // macOS
		<< QStringLiteral("Menlo")            // macOS fallback
		<< QStringLiteral("DejaVu Sans Mono") // Linux
		<< QStringLiteral("Consolas");        // last resort before Courier
	f.setFamilies(families);
	f.setStyleHint(QFont::Monospace);
	f.setFixedPitch(true);
	// Hinting off + antialias: JetBrains Mono's stems get chewed up by Windows
	// full hinting at 13px, which is what made the old pane look ragged.
	f.setHintingPreference(QFont::PreferNoHinting);
	f.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
	f.setLetterSpacing(QFont::PercentageSpacing, 100.0);
	return f;
}

Theme Theme::dark() {
	Theme t;
	t.id = QStringLiteral("dark");
	t.bgBase = QColor("#16181d");
	t.bgPanel = QColor("#1c1f26");
	t.bgSidebar = QColor("#1a1d23");
	t.border = QColor("#24272f");
	t.control = QColor("#262a33");
	t.controlHover = QColor("#2e333e");
	t.controlBorder = QColor("#2c2f38");
	t.text = QColor("#c9ced8");
	t.textBright = QColor("#e8eaf0");
	t.textMuted = QColor("#9aa0ab");
	t.textDim = QColor("#6b7280");
	t.textFaint = QColor("#565d6b");
	t.textGhost = QColor("#4d5462");
	t.accent = QColor("#7d9cc0");
	t.accentText = QColor("#c9dbf0");
	t.accentBg = QColor("#223040");
	t.runBg = QColor("#2b3a4d");
	t.ok = QColor("#8fd0a0");
	t.breakpoint = QColor("#e59aae");
	// Desaturated one stop from the old palette: at 13px on a near-black
	// background the previous #ffb454/#f28fad read as neon and pulled the eye
	// off the mnemonic column.
	t.synMnemonic = QColor("#93b0d1");
	t.synJump = QColor("#e59aae");
	t.synRegister = QColor("#7fc8dd");
	t.synImmediate = QColor("#d9a45f");
	t.synTarget = QColor("#86c79a");
	t.synString = QColor("#86c79a");
	t.synPunct = QColor("#4d5462");
	return t;
}

Theme Theme::light() {
	Theme t;
	t.id = QStringLiteral("light");
	t.bgBase = QColor("#fbfbfa");
	t.bgPanel = QColor("#ebebe7");
	t.bgSidebar = QColor("#f1f1ee");
	t.border = QColor("#dcdcd6");
	t.control = QColor("#e6e6e0");
	t.controlHover = QColor("#dededa");
	t.controlBorder = QColor("#d5d5cf");
	t.text = QColor("#3b4250");
	t.textBright = QColor("#26282c");
	t.textMuted = QColor("#6d7280");
	t.textDim = QColor("#8a8f99");
	t.textFaint = QColor("#a8adb6");
	t.textGhost = QColor("#b0b4bc");
	t.accent = QColor("#1f5fa8");
	t.accentText = QColor("#12417a");
	t.accentBg = QColor("#e4e9f0");
	t.runBg = QColor("#dde6f1");
	t.ok = QColor("#2f7a3f");
	t.breakpoint = QColor("#c0392b");
	t.synMnemonic = QColor("#1f5fa8");
	t.synJump = QColor("#a63a68");
	t.synRegister = QColor("#0f7a72");
	t.synImmediate = QColor("#9a5b12");
	t.synTarget = QColor("#2f7a3f");
	t.synString = QColor("#2f7a3f");
	t.synPunct = QColor("#a8adb6");
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
		{QStringLiteral("bgSidebar"), hex(bgSidebar)},
		{QStringLiteral("border"), hex(border)},
		{QStringLiteral("control"), hex(control)},
		{QStringLiteral("controlHover"), hex(controlHover)},
		{QStringLiteral("controlBorder"), hex(controlBorder)},
		{QStringLiteral("text"), hex(text)},
		{QStringLiteral("textBright"), hex(textBright)},
		{QStringLiteral("textMuted"), hex(textMuted)},
		{QStringLiteral("textDim"), hex(textDim)},
		{QStringLiteral("textFaint"), hex(textFaint)},
		{QStringLiteral("textGhost"), hex(textGhost)},
		{QStringLiteral("accent"), hex(accent)},
		{QStringLiteral("accentText"), hex(accentText)},
		{QStringLiteral("accentBg"), hex(accentBg)},
		{QStringLiteral("runBg"), hex(runBg)},
		{QStringLiteral("ok"), hex(ok)},
		{QStringLiteral("breakpoint"), hex(breakpoint)},
		{QStringLiteral("synString"), hex(synString)},
		{QStringLiteral("synTarget"), hex(synTarget)},
	};
}

} // namespace gui
