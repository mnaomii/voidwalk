#include "icons.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace gui {

namespace {
QColor g_normal = QColor("#c9ced8");
QColor g_disabled = QColor("#565d6b");
QHash<QString, QIcon> g_cache;

QPixmap renderSvg(const QByteArray& svg, int size) {
	QSvgRenderer renderer(svg);
	QPixmap pm(size, size);
	pm.fill(Qt::transparent);
	QPainter painter(&pm);
	renderer.render(&painter);
	return pm;
}
} // namespace

void Icons::setColors(const QColor& normal, const QColor& disabled) {
	g_normal = normal;
	g_disabled = disabled;
	g_cache.clear(); // re-render lazily with the new colors
}

QIcon Icons::icon(const QString& name) {
	const auto cached = g_cache.constFind(name);
	if (cached != g_cache.constEnd())
		return cached.value();

	QFile f(QStringLiteral(":/icons/%1.svg").arg(name));
	if (!f.open(QIODevice::ReadOnly))
		return QIcon(); // missing resource: empty icon, action still shows text

	const QByteArray original = f.readAll();
	QIcon result;
	for (const int size : {16, 20, 24, 32, 48}) {
		QByteArray normal = original;
		normal.replace("#000000", g_normal.name(QColor::HexRgb).toLatin1());
		result.addPixmap(renderSvg(normal, size), QIcon::Normal);

		QByteArray disabled = original;
		disabled.replace("#000000", g_disabled.name(QColor::HexRgb).toLatin1());
		result.addPixmap(renderSvg(disabled, size), QIcon::Disabled);
	}

	g_cache.insert(name, result);
	return result;
}

} // namespace gui
