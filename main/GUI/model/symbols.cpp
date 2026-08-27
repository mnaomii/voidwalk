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
bool callTarget(const std::string& text, uint64_t* out) {
	std::istringstream is(text);
	std::string mnemonic, operand;
	if (!(is >> mnemonic >> operand)) return false;
	std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if (mnemonic != "call") return false;
	if (operand.rfind("0x", 0) != 0) return false; // register / [mem] / symbol
	try {
		*out = std::stoull(operand.substr(2), nullptr, 16);
	} catch (...) {
		return false;
	}
	return true;
}

bool printable(uint8_t b) {
	return b >= 0x20 && b <= 0x7e;
}

void collectStrings(const Session& session, const SectionInfo& sec,
                    std::vector<SymbolInfo>* out) {
	if (sec.size == 0) return;
	// Cap the scan so a 40 MB .data section can't stall a refresh.
	constexpr uint64_t kMaxScan = 256 * 1024;
	const uint64_t total = std::min<uint64_t>(sec.size, kMaxScan);
	const std::vector<uint8_t> raw = session.bytes(sec.offset, static_cast<size_t>(total));

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
void collectImports(const Session&, std::vector<SymbolInfo>*) {}

} // namespace

std::vector<SymbolInfo> collectSymbols(const Session& session) {
	std::vector<SymbolInfo> out;
	if (!session.loaded()) return out;

	// --- functions: call targets + the .text entry -------------------------
	std::set<uint64_t> targets;
	if (session.textVaddr() != 0)
		targets.insert(session.textVaddr());
	const size_t rows = session.rowCount();
	for (size_t i = 0; i < rows; ++i) {
		uint64_t target = 0;
		if (callTarget(session.rowText(i), &target))
			targets.insert(target);
	}
	for (uint64_t addr : targets) {
		const bool isEntry = addr == session.textVaddr();
		out.push_back({SymbolInfo::Kind::Function,
		               isEntry ? "entry" : "sub_" + hexAddr(addr, 6),
		               addr, hexAddr(addr, 6)});
	}

	collectImports(session, &out);

	// --- strings: .rodata then .data ---------------------------------------
	for (const SectionInfo& sec : session.sections()) {
		if (sec.name == ".rodata" || sec.name == ".data")
			collectStrings(session, sec, &out);
	}

	return out;
}

} // namespace gui
