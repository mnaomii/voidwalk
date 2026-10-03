#include "tui/syntax.hpp"

#include <cctype>
#include <regex>
#include <unordered_set>

namespace tui {

namespace {

bool isPrefixWord(const std::string& w) {
	static const std::unordered_set<std::string> prefixes = {
		"lock", "rep", "repne", "bnd", "notrack", "cs", "ds", "es", "fs", "gs", "ss",
	};
	return prefixes.count(w) != 0;
}

bool isRegister(const std::string& w) {
	static const std::unordered_set<std::string> regs = {
		"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eip",
		"cs", "ds", "ss", "es", "fs", "gs",
		"ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
		"al", "bl", "cl", "dl", "ah", "bh", "ch", "dh",
		"spl", "bpl", "sil", "dil",
		"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "rip", "st",
	};
	static const std::regex numbered("^(r(8|9|1[0-5])[dwb]?|[xy]?mm([0-9]|1[0-5]))$");
	return regs.count(w) != 0 || std::regex_match(w, numbered);
}

bool isImmediate(const std::string& w) {
	if (w.rfind("0x", 0) == 0 || w.rfind("-0x", 0) == 0) return true;
	if (w.empty()) return false;
	std::size_t i = (w[0] == '-') ? 1 : 0;
	if (i == w.size()) return false;
	for (; i < w.size(); ++i)
		if (!std::isdigit(static_cast<unsigned char>(w[i]))) return false;
	return true;
}

bool isWordChar(char c) {
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

std::string lower(std::string s) {
	for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

} // namespace

std::vector<Token> tokenize(const std::string& text, bool branch) {
	std::vector<Token> out;
	bool mnemonicSeen = false;
	const std::size_t n = text.size();
	std::size_t i = 0;

	auto negHex = [&](std::size_t at) { return text[at] == '-' && at + 1 < n && text[at + 1] == '0'; };

	while (i < n) {
		const char c = text[i];
		if (c == ';') {
			out.push_back({text.substr(i), Role::SynPunct});
			break;
		}
		if (isWordChar(c) || negHex(i)) {
			const std::size_t start = i;
			if (c == '-') ++i;
			while (i < n && isWordChar(text[i])) ++i;
			const std::string word = text.substr(start, i - start);
			const std::string w = lower(word);

			Role role = Role::Text;
			if (!mnemonicSeen) {
				mnemonicSeen = !isPrefixWord(w);   // prefixes share the mnemonic's colour
				role = branch ? Role::SynJump : Role::SynMnemonic;
			}
			else if (isRegister(w)) role = Role::SynRegister;
			else if (isImmediate(w)) role = branch ? Role::SynTarget : Role::SynImmediate;
			else if (w == "dword" || w == "word" || w == "qword" || w == "byte" || w == "ptr")
				role = Role::Muted;

			out.push_back({word, role});
			continue;
		}
		const std::size_t start = i;
		while (i < n && !isWordChar(text[i]) && text[i] != ';' && !negHex(i)) ++i;
		out.push_back({text.substr(start, i - start), Role::SynPunct});
	}
	return out;
}

} // namespace tui
