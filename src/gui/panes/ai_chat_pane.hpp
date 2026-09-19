#pragma once
#include "gui/ai/ai_client.hpp"
#include "gui/dialogs/settings.hpp"
#include "gui/session.hpp"

#include <QWidget>

class QTextEdit;
class QLineEdit;
class QPushButton;

namespace gui {

// Opt-in AI chat pane. Renders a transcript and an input box; on send it asks
// the injected AiBackend, passing the user's message plus a context string it
// assembles from the current disassembly (bounded by AppSettings::aiContextLines).
// The pane owns no model logic — setBackend() injects the integration. Until a
// real backend is set, a StubAiBackend answers so the UI is exercisable.
class AiChatPane : public QWidget {
	Q_OBJECT
public:
	explicit AiChatPane(QWidget* parent = nullptr);

	void setSession(Session* s) { session_ = s; }
	void setSettings(const AppSettings& s) { settings_ = s; }

	// Takes ownership (reparents to this). Passing nullptr restores the stub.
	void setBackend(AiBackend* backend);
	AiBackend* backend() const { return backend_; }

private slots:
	void onSend();
	void onResponse(const QString& text);
	void onError(const QString& message);

private:
	QString buildContext() const;
	void appendTurn(const QString& who, const QString& text);
	void setInputEnabled(bool on);

	Session* session_ = nullptr;
	AppSettings settings_;
	AiBackend* backend_ = nullptr;

	QTextEdit* transcript_ = nullptr;
	QLineEdit* input_ = nullptr;
	QPushButton* send_ = nullptr;
};

} // namespace gui

