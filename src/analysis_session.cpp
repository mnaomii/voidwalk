#include "analysis_session.hpp"

#include "disassembler/format/detect.hpp"

namespace voidwalk {

// Loads a binary and starts decoding it. Keeps the previous binary on failure.
bool Session::open(const std::string& path) {
	std::shared_ptr<AddressSpace> newSpace;
	std::shared_ptr<Disassembler> newDisassembler;
	// std::exception, not runtime_error: AddressSpace throws length_error (a
	// logic_error) for a truncated or malformed file, which a runtime_error catch
	// lets through - straight out of the caller's event loop.
	try {
		newSpace = std::make_shared<AddressSpace>(path);
		make_disassembler(*newSpace, &newDisassembler);
	}
	catch (const std::exception& e) {
		setStatus(std::string("Open failed: ") + e.what());
		return false;
	}

	bool is_elf = false, is_pe = false;
	determine_filetype(*newSpace, is_elf, is_pe);

	adopt(std::move(newSpace), std::move(newDisassembler), path, is_elf ? "ELF" : "PE");
	setStatus("Loaded " + path);
	return true;
}

// Installs a binary and starts a decode sweep over it.
void Session::adopt(std::shared_ptr<AddressSpace> space,
                    std::shared_ptr<Disassembler> disassembler,
                    std::string path,
                    std::string format) {
	// Replace both together: the disassembler holds a reference to the address space.
	space_ = std::move(space);
	disassembler_ = std::move(disassembler);
	filePath_ = std::move(path);
	format_ = std::move(format);
	runDecode();
}

// Launches Disassembler::decode() on a fresh worker thread.
void Session::runDecode() {
	if (!disassembler_) return;
	// Stop and join the previous worker BEFORE touching any shared state. Until it
	// has returned it may still be writing state->note and state->running, and the
	// reset below would race that write (two threads on one std::string). The
	// jthread's move-assignment would have joined it too, but only *after* the new
	// worker had already started, which left both the race and a window where the
	// old sweep's running.store(false) landed on top of the new sweep's true.
	decodeThread_ = {};
	// One DecodeState per sweep, never reused: even if a worker somehow outlived the
	// join above, it would be writing to its own object, kept alive by its own
	// shared_ptr copy. Costs one allocation per opened binary.
	decodeState_ = std::make_shared<DecodeState>();
	onDecodeStarted();
	decodeState_->running.store(true, std::memory_order_release);

	// Capture shared_ptr copies, never `this`: the Session may be moved, and a
	// re-open may replace its pointers, but the worker's copies keep the
	// disassembler and its address space alive either way. The previous decode was
	// already stopped and joined above; decode() polls the stop_token each line, so
	// that returns promptly.
	decodeThread_ = std::jthread(
		[disasm = disassembler_, space = space_, state = decodeState_](std::stop_token st) {
			(void)space; // held only to keep the AddressSpace alive under the worker
			try {
				disasm->decode(st);
			}
			catch (const std::exception& e) {
				// Partial results survive: whatever decoded before the throw is kept.
				state->note = e.what();
			}
			// Release so a reader that sees running==false also sees note above.
			state->running.store(false, std::memory_order_release);
		});
}

// Architecture name, or "" when nothing is loaded.
std::string Session::architecture() const {
	return loaded() ? disassembler_->getArchitecture() : "";
}

// True for the 64-bit architectures. Format-agnostic.
bool Session::is64bit() const {
	return loaded() && voidwalk::is64Bit(disassembler_->architecture());
}

// The worker's error message, readable only once the worker has stopped.
const std::string& Session::decodeNote() const {
	static const std::string kEmpty;
	if (!decodeState_ || isDecoding()) return kEmpty;
	return decodeState_->note;
}

// Raw file bytes, clamped at end-of-file.
std::vector<uint8_t> Session::bytes(uint64_t offset, size_t count) const {
	std::vector<uint8_t> out;
	if (!space_) return out;
	size_t max = space_->size();
	if (offset >= max) return out;
	if (count > max - offset) count = max - offset;
	out.reserve(count);
	for (size_t i = 0; i < count; ++i)
		out.push_back(space_->read_u8(offset + i));
	return out;
}

// Size of the loaded file, or 0.
size_t Session::binarySize() const {
	return space_ ? space_->size() : 0;
}

// File offset of .text, or 0.
uint64_t Session::textOffset() const {
	return loaded() ? disassembler_->getSections()._text.getOffset() : 0;
}

// Virtual address of .text, or 0.
uint64_t Session::textVaddr() const {
	return loaded() ? disassembler_->getSections()._text.getVaddr() : 0;
}

} // namespace voidwalk
