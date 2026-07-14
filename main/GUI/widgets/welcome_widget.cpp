#include "welcome_widget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace gui {

WelcomeWidget::WelcomeWidget(QWidget* parent) : QWidget(parent) {
	setAcceptDrops(true);

	auto* badge = new QLabel(QStringLiteral("01"), this);
	badge->setObjectName(QStringLiteral("welcomeBadge"));
	badge->setFixedSize(72, 72);
	badge->setAlignment(Qt::AlignCenter);

	auto* title = new QLabel(tr("Open a binary to begin"), this);
	title->setObjectName(QStringLiteral("welcomeTitle"));
	title->setAlignment(Qt::AlignCenter);

	auto* hint = new QLabel(
		tr("voidwalk analyzes PE and ELF executables — sections, disassembly,\n"
		   "and (soon) live debugging. Drop a file here or press Ctrl+O."),
		this);
	hint->setObjectName(QStringLiteral("welcomeHint"));
	hint->setAlignment(Qt::AlignCenter);

	auto* open = new QPushButton(tr("Open binary…"), this);
	open->setDefault(true); // picks up the accent :default styling
	open->setCursor(Qt::PointingHandCursor);
	connect(open, &QPushButton::clicked, this, &WelcomeWidget::openRequested);

	auto* chips = new QHBoxLayout();
	chips->setSpacing(8);
	chips->addStretch();
	for (const char* ext : {".exe", ".dll", ".so", ".elf"}) {
		auto* chip = new QLabel(QString::fromLatin1(ext), this);
		chip->setObjectName(QStringLiteral("welcomeChip"));
		chips->addWidget(chip);
	}
	chips->addStretch();

	auto* column = new QVBoxLayout();
	column->setSpacing(18);
	column->addWidget(badge, 0, Qt::AlignHCenter);
	column->addWidget(title);
	column->addWidget(hint);
	column->addWidget(open, 0, Qt::AlignHCenter);
	column->addLayout(chips);

	auto* layout = new QVBoxLayout(this);
	layout->addStretch();
	layout->addLayout(column);
	layout->addStretch();
}

void WelcomeWidget::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void WelcomeWidget::dropEvent(QDropEvent* event) {
	const QList<QUrl> urls = event->mimeData()->urls();
	if (!urls.isEmpty() && urls.first().isLocalFile())
		emit fileDropped(urls.first().toLocalFile());
}

} // namespace gui
