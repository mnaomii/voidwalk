#include "tui/session.hpp"

#include "disassembler/format/detect.hpp"

#include <algorithm>
#include <cstdio>
#include <set>

namespace tui {

using voidwalk::determine_filetype;

namespace {

std::string hexUp(uint64_t v, int width) {
	char buf[24];
	std::snprintf(buf, sizeof(buf), "%0*llX", width, static_cast<unsigned long long>(v));
	return buf;
}

// Same tidy-up gui::Session applies to "<MNEMONIC> \t<operands>": split on the tab,
// trim, pad the mnemonic to column 7 so operands line up.
std::string formatDisasmText(const std::string& raw) {
	std::string mnemonic, operands;
	const auto tab = raw.find('\t');
	if (tab == std::string::npos) mnemonic = raw;
	else { mnemonic = raw.substr(0, tab); operands = raw.substr(tab + 1); }

	auto trim = [](std::string& s) {
		const auto first = s.find_first_not_of(" \t");
		if (first == std::string::npos) { s.clear(); return; }
		s = s.substr(first, s.find_last_not_of(" \t") - first + 1);
	};
	trim(mnemonic);
	trim(operands);
	if (operands.empty()) return mnemonic;

	constexpr std::size_t kColumn = 7;
	if (mnemonic.size() < kColumn) mnemonic.append(kColumn - mnemonic.size(), ' ');
	else mnemonic.push_back(' ');
	return mnemonic + operands;
}

const Registers_x86_64 kZeroRegisters{};

bool printable(uint8_t b) { return b >= 0x20 && b <= 0x7e; }

constexpr int kMinStringLen = 4;
constexpr int kMaxStringDisplay = 28;

} // namespace

Session::Session(std::shared_ptr<AddressSpace> space,
                 std::shared_ptr<Disassembler> disassembler,
                 std::string filePath) {
	std::string format;
	if (disassembler) {
		bool is_elf = false, is_pe = false;
		determine_filetype(*space, is_elf, is_pe);
		format = is_elf ? "ELF" : (is_pe ? "PE" : "?");
	}
	adopt(std::move(space), std::move(disassembler), std::move(filePath), std::move(format));
	refresh();
}

void Session::onDecodeStarted() {
	real_ = false;
	shown_ = 0;
	fallback_.clear();
	banner_.clear();
	symbols_.clear();
}

std::string Session::functionName(uint64_t addr, uint64_t entry) {
	return addr == entry ? "entry" : "sub_" + hexUp(addr, 6);
}

void Session::refresh() {
	throwIfFileChanged();
	banner_.clear();
	if (!loaded()) {
		real_ = false;
		shown_ = 0;
		fallback_.clear();
		symbols_.clear();
		return;
	}

	// Read before anything else: seeing the worker stopped is the acquire that makes
	// the note and every published row visible below.
	const bool done = !isDecoding();
	const size_t ready = disassembler_->readyInstructions();
	const std::string note = done ? decodeNote() : std::string();

	if (ready > 0) {
		real_ = true;
		shown_ = ready;
		if (!note.empty()) banner_ = "Decode stopped: " + note;
	}
	else if (!done) {
		banner_ = "Decoding...";
	}
	else {
		// Same wording as gui::DisassemblyPane's banner.
		real_ = false;
		buildFallback();
		banner_ = !note.empty()
			? note
			: "The decoder for " + architecture() + " is a stub - showing raw .text bytes instead of real instructions.";
	}

	if (done && symbolsGen_ != decodeGeneration()) {
		collectSymbols();
		symbolsGen_ = decodeGeneration();
	}
}

void Session::buildFallback() {
	fallback_.clear();
	constexpr uint64_t kMaxBytes = 4096;
	uint64_t budget = kMaxBytes;
	for (const Header& text : disassembler_->getSections().text) {
		if (budget == 0) break;
		if (text.getSize() == 0) continue;
		const uint64_t total = std::min<uint64_t>(text.getSize(), budget);
		const auto raw = bytes(text.getOffset(), static_cast<size_t>(total));
		budget -= raw.size();
		for (size_t i = 0; i < raw.size(); i += 8) {
			FallbackRow row;
			row.vaddr = text.getVaddr() + i;
			const size_t n = std::min<size_t>(8, raw.size() - i);
			row.text = "db ";
			for (size_t b = 0; b < n; ++b) {
				row.bytes += (b ? " " : "") + hexUp(raw[i + b], 2);
				row.text += (b ? ", 0x" : "0x") + hexUp(raw[i + b], 2);
			}
			fallback_.push_back(std::move(row));
		}
	}
}

size_t Session::rowCount() const {
	if (!loaded()) return 0;
	return real_ ? shown_ : fallback_.size();
}

uint64_t Session::rowVaddr(size_t i) const {
	if (!loaded()) return 0;
	if (!real_) return i < fallback_.size() ? fallback_[i].vaddr : 0;
	if (i >= shown_) return 0;
	return disassembler_->getInstructionAddresses()[i];
}

std::string Session::rowBytes(size_t i) const {
	if (!loaded()) return {};
	if (!real_) return i < fallback_.size() ? fallback_[i].bytes : std::string();
	if (i >= shown_) return {};
	std::string b = disassembler_->getDecodedInstructions()[i]->getMachineCode();
	while (!b.empty() && b.back() == ' ') b.pop_back();
	return b;
}

std::string Session::rowText(size_t i) const {
	if (!loaded()) return {};
	if (!real_) return i < fallback_.size() ? fallback_[i].text : std::string();
	if (i >= shown_) return {};
	return formatDisasmText(disassembler_->getDecodedInstructions()[i]->decodeLineString());
}

Instruction::Flow Session::rowFlow(size_t i) const {
	if (!loaded() || !real_ || i >= shown_) return Instruction::Flow::None;
	return disassembler_->getDecodedInstructions()[i]->flow();
}

uint64_t Session::rowTarget(size_t i) const {
	if (!loaded() || !real_ || i >= shown_) return 0;
	return disassembler_->getDecodedInstructions()[i]->target();
}

std::string Session::rowNote(size_t i) const {
	const uint64_t target = rowTarget(i);
	if (target == 0) return {};
	switch (rowFlow(i)) {
	case Instruction::Flow::Call:
		return functionName(target, entryPoint());
	case Instruction::Flow::Jump:
	case Instruction::Flow::CondJump:
		return target < rowVaddr(i) ? "backward" : std::string();
	default:
		return {};
	}
}

size_t Session::rowIndexFor(uint64_t vaddr) const {
	const size_t shown = rowCount();
	if (shown == 0) return 0;
	auto at = [this](size_t i) { return rowVaddr(i); };
	auto split = [](size_t lo, size_t hi, auto pred) {
		while (lo < hi) {
			const size_t mid = lo + (hi - lo) / 2;
			if (pred(mid)) lo = mid + 1; else hi = mid;
		}
		return lo;
	};
	size_t best = 0, start = 0;
	uint64_t bestVaddr = 0;
	for (const SectionInfo& s : sections()) {
		if (s.name != ".text" || start >= shown) continue;
		const size_t end = split(start, shown, [&](size_t i) {
			return at(i) >= s.vaddr && at(i) - s.vaddr < s.size;
		});
		const size_t k = split(start, end, [&](size_t i) { return at(i) <= vaddr; });
		if (k > start && at(k - 1) >= bestVaddr) { best = k - 1; bestVaddr = at(best); }
		start = end;
	}
	return best;
}

const Registers_x86_64& Session::registers() const {
	return loaded() ? disassembler_->getRegisters() : kZeroRegisters;
}

std::vector<SectionInfo> Session::sections() const {
	std::vector<SectionInfo> out;
	if (!loaded()) return out;
	const Sections& s = disassembler_->getSections();
	auto add = [&out](const char* name, const std::vector<Header>& list) {
		for (const Header& h : list) {
			if (h.getOffset() == 0 && h.getSize() == 0) continue;
			out.push_back({name, h.getOffset(), h.getVaddr(), h.getSize()});
		}
	};
	add(".text", s.text);
	add(".data", s.data);
	add(".rodata", s.readOnly);
	add(".bss", s.bss);
	return out;
}

// Mirrors gui::collectSymbols(): call targets + entry, then printable runs in
// .rodata/.data. Runs once per decode, on the UI thread, after the worker stops.
void Session::collectSymbols() {
	symbols_.clear();
	if (!real_) return;

	std::set<uint64_t> targets;
	const uint64_t entry = entryPoint();
	if (entry != 0) targets.insert(entry);
	for (size_t i = 0; i < shown_; ++i)
		if (rowFlow(i) == Instruction::Flow::Call && rowTarget(i) != 0)
			targets.insert(rowTarget(i));
	for (uint64_t addr : targets)
		symbols_.push_back({SymbolInfo::Kind::Function, functionName(addr, entry), addr, hexUp(addr, 6)});

	for (const SectionInfo& sec : sections()) {
		if ((sec.name != ".rodata" && sec.name != ".data") || sec.size == 0) continue;
		constexpr uint64_t kMaxScan = 256 * 1024;
		const auto raw = bytes(sec.offset, static_cast<size_t>(std::min<uint64_t>(sec.size, kMaxScan)));
		std::string run;
		uint64_t runStart = 0;
		auto flush = [&] {
			if (static_cast<int>(run.size()) >= kMinStringLen) {
				std::string shown = run;
				if (static_cast<int>(shown.size()) > kMaxStringDisplay)
					shown = shown.substr(0, kMaxStringDisplay) + "...";
				symbols_.push_back({SymbolInfo::Kind::String, "\"" + shown + "\"",
				                    sec.offset + runStart, hexUp(sec.vaddr + runStart, 8)});
			}
			run.clear();
		};
		for (size_t i = 0; i < raw.size(); ++i) {
			if (printable(raw[i])) {
				if (run.empty()) runStart = i;
				run.push_back(static_cast<char>(raw[i]));
			} else {
				flush();
			}
		}
		flush();
	}
}

} // namespace tui
