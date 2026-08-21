#include "ai_client.h"

#include <QTimer>
// ** AI GENERATED PLACEHOLDERS, WILL MODIFY ** 


namespace gui {

void StubAiBackend::ask(const QString& userMessage, const QString& /*context*/) {
	// Reply on the next event-loop turn so callers see the same async shape a
	// real backend has (input disabled -> re-enabled on responseReady).
	const QString echo = userMessage;
	QTimer::singleShot(0, this, [this, echo]() {
		emit responseReady(
			QStringLiteral("[AI backend not wired yet]\n\n"
			               "You asked: \"%1\"\n\n"
			               "Inject a real AiBackend via AiChatPane::setBackend() to "
			               "have a model analyze the disassembly and answer here.")
				.arg(echo));
	});
}

} // namespace gui
