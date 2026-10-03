#include "gui/symbols.hpp"

#include <algorithm>
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
			                sec.offset + runStart, hexAddr(sec.vaddr + runStart)});
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

std::string functionName(uint64_t addr, uint64_t entry) {
	return addr == entry ? "entry" : "sub_" + hexAddr(addr, 6);
}

std::vector<SymbolInfo> collectSymbols(const Snapshot& snapshot, const std::stop_token& stop) {
	std::vector<SymbolInfo> out;
	if (!snapshot.valid()) return out;

	// --- functions: call targets + the entry point --------------------------
	std::set<uint64_t> targets;
	const uint64_t entry = snapshot.entryPoint();
	if (entry != 0)
		targets.insert(entry);
	const size_t rows = snapshot.rowCount();
	for (size_t i = 0; i < rows; ++i) {
		// Polled in blocks rather than per row: stop_requested() is an atomic load,
		// and against a parse this cheap it would otherwise be a visible share of
		// the loop. 4096 rows is well under a frame either way.
		if ((i & 0xfff) == 0 && stop.stop_requested()) return {};
		if (snapshot.rowFlow(i) == Instruction::Flow::Call && snapshot.rowTarget(i) != 0)
			targets.insert(snapshot.rowTarget(i));
	}
	for (uint64_t addr : targets)
		out.push_back({SymbolInfo::Kind::Function, functionName(addr, entry), addr, hexAddr(addr, 6)});

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
