#pragma once
#include "address_space.hpp"
#include "disassembler/disassembler.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace voidwalk {

// Cross-thread decode state, held behind a shared_ptr so the worker can reference
// it independently of the Session.
//
// `running` is the acquire/release flag readers poll. `note` is the decode's error
// message, written before `running` is cleared and therefore readable only once
// `running` reads false.
struct DecodeState {
	std::atomic<bool> running{false};
	std::string note;
};

// Everything a frontend needs from a loaded binary that is not presentation: the
// address space, the disassembler, and the decode worker. Frontends subclass it and
// add only their own view state.
//
// Lifetime rule: disassembler_ holds an AddressSpace&, so space_ and disassembler_
// are always replaced together. adopt() is the only thing that does so.
class Session {
public:
	Session() = default;
	virtual ~Session() = default;

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = default;
	Session& operator=(Session&&) = default;

	// Loads a binary, replacing whatever was loaded before, and starts a decode.
	// Returns false and leaves the previous binary in place on failure, with the
	// reason in status().
	bool open(const std::string& path);

	// True once a binary has been loaded successfully.
	bool loaded() const { return disassembler_ != nullptr; }

	// Path the loaded binary was opened from; empty when nothing is loaded.
	const std::string& filePath() const { return filePath_; }

	// Container format: "ELF", "PE", or "" when nothing is loaded.
	const std::string& format() const { return format_; }

	// Architecture name, e.g. "x86_64"; empty when nothing is loaded.
	std::string architecture() const;

	// True when the loaded target is 64-bit (x86_64 / AArch64).
	bool is64bit() const;

	// Status-bar message. Also carries "not implemented yet" for stub actions.
	const std::string& status() const { return status_; }
	void setStatus(std::string s) { status_ = std::move(s); }

	// True while the decode worker is still running. Poll this to keep refreshing.
	bool isDecoding() const {
		return decodeState_ && decodeState_->running.load(std::memory_order_acquire);
	}

	// Why the decode produced nothing or stopped early; "" if it ran clean.
	// Reads empty while isDecoding() is true: the note is not safe to read until the
	// worker has published it by clearing `running`.
	const std::string& decodeNote() const;

	// Raw file bytes, clamped at end-of-file; empty when nothing is loaded.
	// NOTE: the offset is a *file offset*, while disassembly addresses are *virtual
	// addresses*. They differ until a debugger provides a loaded image.
	std::vector<uint8_t> bytes(uint64_t offset, size_t count) const;

	// Size of the loaded file in bytes; 0 when nothing is loaded.
	size_t binarySize() const;

	// File offset of .text; 0 when nothing is loaded or it was not found.
	uint64_t textOffset() const;

	// Virtual address of .text; 0 when nothing is loaded or it was not found.
	uint64_t textVaddr() const;

protected:
	// Installs a binary and starts decoding it. `format` is stored as-is so a caller
	// that already knows the detection result does not repeat it.
	void adopt(std::shared_ptr<AddressSpace> space,
	           std::shared_ptr<Disassembler> disassembler,
	           std::string path,
	           std::string format);

	// Stops and joins any previous worker, then launches Disassembler::decode() on a
	// fresh one. Calls onDecodeStarted() after resetting shared state and before the
	// worker is launched.
	void runDecode();

	// Called by runDecode() once per sweep, for subclasses to drop the view state
	// derived from the previous decode. Default does nothing.
	virtual void onDecodeStarted() {}

	std::shared_ptr<AddressSpace> space_;
	std::shared_ptr<Disassembler> disassembler_;
	std::string filePath_;
	std::string format_;
	std::string status_;
	std::shared_ptr<DecodeState> decodeState_;

	// Declared LAST so it is destroyed - and therefore joined - before decodeState_,
	// disassembler_ and space_ are torn down.
	std::jthread decodeThread_;
};

} // namespace voidwalk
