#ifndef GUI_AI_BACKEND_H
#define GUI_AI_BACKEND_H

#include <QObject>
#include <QString>

// ** AI GENERATED PLACEHOLDERS, WILL MODIFY ** 

namespace gui {

// Frontend hook for the AI chat pane. The GUI owns the pane and this contract;
// the actual model integration (Anthropic Messages API or otherwise) is
// implemented by you against this interface and injected via
// AiChatPane::setBackend(). The pane never knows how the answer is produced.
//
// Contract:
//   - ask() is called when the user sends a chat message. `userMessage` is the
//     text typed; `context` is the analysis context the pane assembled for this
//     turn (see AiChatPane — currently the visible disassembly plus file info).
//   - Implementations run asynchronously and must emit exactly one of
//     responseReady / errorOccurred per ask() call.
//   - busy() lets the pane disable its input while a request is in flight.
class AiBackend : public QObject {
	Q_OBJECT
public:
	explicit AiBackend(QObject* parent = nullptr) : QObject(parent) {}
	~AiBackend() override = default;

	virtual void ask(const QString& userMessage, const QString& context) = 0;
	virtual bool busy() const { return false; }

signals:
	void responseReady(const QString& assistantText);
	void errorOccurred(const QString& message);
};

// No-op backend so the GUI builds and runs before the real integration exists.
// Echoes a "not wired yet" notice. Replace by injecting your own AiBackend.
class StubAiBackend : public AiBackend {
	Q_OBJECT
public:
	explicit StubAiBackend(QObject* parent = nullptr) : AiBackend(parent) {}
	void ask(const QString& userMessage, const QString& context) override;
};

} // namespace gui

#endif
