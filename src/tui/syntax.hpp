#pragma once
#include "tui/theme/palette.hpp"

#include <string>
#include <vector>

namespace tui {

struct Token {
	std::string text;
	Role role;
};

// Port of gui::DisasmDelegate::tokenize(): the same word classes in the same order,
// so a token gets the same role in both frontends. `branch` is "the row's flow is
// not None": the mnemonic then takes SynJump and immediates take SynTarget.
//
// TODO(cohesion): move this (and the GUI copy) into core so there is one tokenizer.
std::vector<Token> tokenize(const std::string& text, bool branch);

} // namespace tui
