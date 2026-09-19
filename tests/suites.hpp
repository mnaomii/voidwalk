#pragma once

// One entry point per suite, each defined in its own translation unit.
//
// Each function constructs its suite, whose constructor runs the checks. None
// return a result: failures accumulate in the shared Tests counters, which
// runTests() reads once at the end.

void run_address_space_tests();
void run_loader_tests();
void run_elf_sections_tests();
void run_pe_sections_tests();
void run_ia32_tests();
void run_amd64_tests();
void run_sweep_tests();
void run_malformed_tests();
void run_async_decode_tests();
void run_real_binary_tests();
