#include "symbols.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace gui {

namespace {

std::string hexAddr(uint64_t v, int width = 8) {
	std::ostringstream os;
	os << std::hex << std::uppercase;
	std::string digits = [&] { os << v; return os.str(); }();
	while (static_cast<int>(digits.size()) < width)
		digits.insert(digits.begin(), '0');
	return digits;
}

// Pulls the numeric operand out of "call   0x00401160" / "je 0x401029".
// Returns false when the row is not a call, or the operand is a register or a
// memory expression (indirect calls have no static target to name).
//
// Hand-rolled rather than istringstream: this runs once per decoded row, and the
// stream version built two std::strings and a stream state block for every one of
// them — on a binary with 300k instructions that dominated the whole scan. The
// mnemonic is compared in place, so a non-call row (the overwhelming majority)
// costs a handful of character comparisons and allocates nothing.
bool callTarget(const std::string& text, uint64_t* out) {
	std::size_t i = text.find_first_not_of(" \t");
	if (i == std::string::npos) return false;
	const std::size_t mnemonicEnd = text.find_first_of(" \t", i);
	if (mnemonicEnd == std::string::npos) return false; // no operand at all

	// Compare against "call" without copying the mnemonic out.
	static constexpr char kCall[] = "call";
	if (mnemonicEnd - i != 4) return false;
	for (int c = 0; c < 4; ++c)
		if (std::tolower(static_cast<unsigned char>(text[i + c])) != kCall[c]) return false;

	const std::size_t opStart = text.find_first_not_of(" \t", mnemonicEnd);
	if (opStart == std::string::npos) return false;
	// register / [mem] / symbol — no static target to name.
	if (text.compare(opStart, 2, "0x") != 0) return false;

	uint64_t value = 0;
	std::size_t digits = 0;
	for (std::size_t p = opStart + 2; p < text.size(); ++p, ++digits) {
		const unsigned char c = static_cast<unsigned char>(text[p]);
		int d;
		if (c >= '0' && c <= '9')      d = c - '0';
		else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
		else break;
		if (digits >= 16) return false; // wider than a 64-bit address: not one
		value = (value << 4) | static_cast<uint64_t>(d);
	}
	if (digits == 0) return false;
	*out = value;
	return true;
}

bool printable(uint8_t b) {
	return b >= 0x20 && b <= 0x7e;
}

void collectStrings(const Snapshot& snapshot, const SectionInfo& sec,
                    std::vector<SymbolInfo>* out) {
	if (sec.size == 0) return;
	// Cap the scan so a 40 MB .data section can't stall a refresh.
	constexpr uint64_t kMaxScan = 256 * 1024;
	const uint64_t total = std::min<uint64_t>(sec.size, kMaxScan);
	const std::vector<uint8_t> raw = snapshot.bytes(sec.offset, static_cast<size_t>(total));

	std::string run;
	uint64_t runStart = 0;
	const auto flush = [&] {
		if (static_cast<int>(run.size()) >= kMinStringLen) {
			std::string shown = run;
			if (static_cast<int>(shown.size()) > kMaxStringDisplay)
				shown = shown.substr(0, kMaxStringDisplay) + "\u2026";
			out->push_back({SymbolInfo::Kind::String, "\"" + shown + "\"",
			                sec.vaddr + runStart, hexAddr(sec.vaddr + runStart)});
		}
		run.clear();
	};

	for (uint64_t i = 0; i < raw.size(); ++i) {
		if (printable(raw[static_cast<size_t>(i)])) {
			if (run.empty()) runStart = i;
			run.push_back(static_cast<char>(raw[static_cast<size_t>(i)]));
		} else {
			flush();
		}
	}
	flush();
}

// Seam for the import table. The PE/ELF loader does not parse imports yet, so
// this yields nothing today; when it does, push one entry per thunk with
// `detail` set to the module name and the sidebar picks them up unchanged.
void collectImports(const Snapshot&, std::vector<SymbolInfo>*) {}

} // namespace

std::vector<SymbolInfo> collectSymbols(const Snapshot& snapshot, const std::stop_token& stop) {
	std::vector<SymbolInfo> out;
	if (!snapshot.valid()) return out;

	// --- functions: call targets + the .text entry -------------------------
	std::set<uint64_t> targets;
	const uint64_t entry = snapshot.textVaddr();
	if (entry != 0)
		targets.insert(entry);
	const size_t rows = snapshot.rowCount();
	for (size_t i = 0; i < rows; ++i) {
		// Polled in blocks rather than per row: stop_requested() is an atomic load,
		// and against a parse this cheap it would otherwise be a visible share of
		// the loop. 4096 rows is well under a frame either way.
		if ((i & 0xfff) == 0 && stop.stop_requested()) return {};
		uint64_t target = 0;
		if (callTarget(snapshot.rowText(i), &target))
			targets.insert(target);
	}
	for (uint64_t addr : targets) {
		const bool isEntry = addr == entry;
		out.push_back({SymbolInfo::Kind::Function,
		               isEntry ? "entry" : "sub_" + hexAddr(addr, 6),
		               addr, hexAddr(addr, 6)});
	}

	collectImports(snapshot, &out);

	// --- strings: .rodata then .data ---------------------------------------
	for (const SectionInfo& sec : snapshot.sections()) {
		if (stop.stop_requested()) return {};
		if (sec.name == ".rodata" || sec.name == ".data")
			collectStrings(snapshot, sec, &out);
	}

	return out;
}

} // namespace gui
