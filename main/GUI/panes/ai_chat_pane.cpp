#include "ai_chat_pane.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

namespace gui {

AiChatPane::AiChatPane(QWidget* parent) : QWidget(parent) {
	transcript_ = new QTextEdit(this);
	transcript_->setReadOnly(true);

	input_ = new QLineEdit(this);
	input_->setPlaceholderText(tr("Ask about the disassembly…"));

	send_ = new QPushButton(tr("Send"), this);

	auto* inputRow = new QHBoxLayout();
	inputRow->addWidget(input_);
	inputRow->addWidget(send_);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(transcript_);
	layout->addLayout(inputRow);

	connect(send_, &QPushButton::clicked, this, &AiChatPane::onSend);
	connect(input_, &QLineEdit::returnPressed, this, &AiChatPane::onSend);

	// So the pane is exercisable before a real backend is injected.
	setBackend(new StubAiBackend(this));
}

void AiChatPane::setBackend(AiBackend* backend) {
	if (backend == nullptr) backend = new StubAiBackend(this);

	if (backend_ && backend_ != backend) {
		disconnect(backend_, nullptr, this, nullptr);
		backend_->deleteLater();
	}

	backend_ = backend;
	backend_->setParent(this);

	connect(backend_, &AiBackend::responseReady, this, &AiChatPane::onResponse, Qt::UniqueConnection);
	connect(backend_, &AiBackend::errorOccurred, this, &AiChatPane::onError, Qt::UniqueConnection);
}

void AiChatPane::onSend() {
	const QString text = input_->text().trimmed();
	if (text.isEmpty()) return;

	appendTurn(tr("You"), text);
	input_->clear();
	setInputEnabled(false);
	backend_->ask(text, buildContext());
}

void AiChatPane::onResponse(const QString& text) {
	appendTurn(tr("AI"), text);
	setInputEnabled(true);
}

void AiChatPane::onError(const QString& message) {
	appendTurn(tr("Error"), message);
	setInputEnabled(true);
}

void AiChatPane::setInputEnabled(bool on) {
	input_->setEnabled(on);
	send_->setEnabled(on);
	if (on) input_->setFocus();
}

void AiChatPane::appendTurn(const QString& who, const QString& text) {
	transcript_->append(QStringLiteral("<b>") + who.toHtmlEscaped() + QStringLiteral(":</b> ") +
		text.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>")));
}

QString AiChatPane::buildContext() const {
	if (!session_ || !session_->loaded())
		return QStringLiteral("No binary loaded.");

	const QString header = QString("File: %1\nFormat: %2\nArchitecture: %3")
		.arg(QString::fromStdString(session_->filePath()),
		     QString::fromStdString(session_->format()),
		     QString::fromStdString(session_->architecture()));

	int limit = settings_.aiContextLines;
	if (limit <= 0) limit = 200;

	QStringList lines;
	int count = 0;
	for (const auto& row : session_->disassembly()) {
		if (count >= limit) break;
		lines << QString("0x%1  %2  %3")
			.arg(row.vaddr, 8, 16, QLatin1Char('0'))
			.arg(QString::fromStdString(row.bytes))
			.arg(QString::fromStdString(row.text));
		++count;
	}

	return header + QStringLiteral("\nDisassembly:\n") + lines.join(QStringLiteral("\n"));
}

} // namespace gui
