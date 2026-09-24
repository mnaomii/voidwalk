#pragma once

// Placeholder for the ARM32 Instruction subclass.
//
// Empty on purpose: ARM32Decoder::decodeLine throws "Not implemented yet.", so
// nothing constructs an instruction for this architecture. When the decoder lands,
// the rendering half goes here - the x86 pair (x86_64_instruction.hpp +
// x86_64_mnemonic.hpp) is the shape to follow.
