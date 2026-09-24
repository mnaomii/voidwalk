#pragma once

// Placeholder for the AArch64 Instruction subclass.
//
// Empty on purpose: AArch64Decoder::decodeLine throws "Not implemented yet.", so
// nothing constructs an instruction for this architecture. When the decoder lands,
// the rendering half goes here - the x86 pair (x86_64_instruction.hpp +
// x86_64_mnemonic.hpp) is the shape to follow.
