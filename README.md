# voidwalk

Binary analysis for **ELF** and **PE** executables, built on its own **x86 / x86-64 disassembler** written from scratch (no disassembly library). Use it from a scriptable CLI, a terminal UI or a Qt 6 desktop GUI.

voidwalk started as a deep dive into how executables and machine code work, and is growing into a practical static and dynamic analysis toolkit for reverse engineers and security researchers.

**Platforms:** Linux and Windows (tested in CI). macOS is untested.
**Status:** active development.

---

## Features

**Loading**
- Detects ELF or PE automatically from the magic bytes
- Parses sections for ELF (32/64-bit) and PE (PE32/PE32+)
- Detects the architecture: x86, x86-64, ARM32, AArch64
- Memory-mapped, bounds-checked file access

**x86 / x86-64 disassembler**
- Prefixes and the full one-byte opcode map, including extension groups and the x87 FPU
- The common `0F` instructions (`Jcc`, `SETcc`, `CMOVcc`, `MOVZX`/`MOVSX`, bit operations, `SYSCALL`, …) plus `ENDBR32`/`ENDBR64`
- Long mode: REX, `R8`–`R15`, RIP-relative addressing, 64-bit operand defaults
- Branch and call targets resolved to absolute addresses; raw bytes shown for every instruction
- Sweeps the whole `.text` section, on a background thread in the TUI and GUI
- On `/bin/ls` (x86-64, Debian 13), all 22,523 instructions start at the same addresses as in GNU `objdump`

**Interfaces**
- **CLI:** disassemble to stdout and to files; hex dump
- **TUI:** disassembly, memory, registers and stack panes; open another file without restarting
- **GUI:**
  - fast, syntax-coloured disassembly
  - symbol sidebar (call targets and strings)
  - hex/ASCII memory view by file offset
  - go-to address, drag-and-drop to open, dark and light themes

**Not yet supported**
- AVX/AVX-512 (VEX/EVEX) and SSE instructions
- ARM32/AArch64 decoding (detected, but shown as raw bytes)
- Symbol tables, import tables and the entry point; only `.text` is disassembled
- The debugger, reassembly of instructions edited in the GUI, and the AI backend: their UI exists, their engines don't yet

---

## Requirements

| Dependency | Needed for | Notes |
|---|---|---|
| CMake ≥ 3.21 | everything | |
| C++20 compiler with `<format>` and `std::jthread` | everything | GCC 13+, Visual Studio 2022, or Clang with a standard library that has both |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) 6.1.9 | TUI | Downloaded automatically if not installed (needs `git` and network access) |
| Qt 6 (Widgets, Svg, Keychain) | GUI | Install it yourself: `qt6-base-dev qt6-svg-dev libqt6keychain1` on Debian/Ubuntu. On Windows, set `CMAKE_PREFIX_PATH` to your Qt install |

The test suite needs neither Qt nor FTXUI.

---

## Building

```sh
git clone https://github.com/mnaomii/voidwalk.git
cd voidwalk
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Binaries are written to `exec/`. A Visual Studio solution is also included in `.vs-project/`.

| CMake option | Default | Effect |
|---|---|---|
| `VOIDWALK_BUILD_GUI` | `ON` | Qt 6 GUI (turn off if Qt isn't installed) |
| `VOIDWALK_BUILD_TUI` | `ON` | FTXUI terminal UI |
| `VOIDWALK_BUILD_TESTS` | `ON` | `voidwalk-tests` binary |
| `VOIDWALK_SANITIZE` | `OFF` | AddressSanitizer + UndefinedBehaviorSanitizer |
| `VOIDWALK_WERROR` | `OFF` | Treat warnings as errors |

---

## Usage

```text
voidwalk                              open the GUI
voidwalk --gui [binary]               open the GUI, optionally loading <binary>
voidwalk --tui <binary>               open the terminal UI
voidwalk --print <binary> [out...]    disassemble to stdout (and to each extra file given)
voidwalk --dump-hex <binary>          hex dump
voidwalk --help                       show usage
```

Example: `voidwalk --print /bin/ls listing.asm` writes the disassembly to the terminal and to `listing.asm`.

---

## Testing

```sh
cmake --build build --target voidwalk-tests
ctest --test-dir build --output-on-failure
```

The suite has more than 200 checks. They cover the file reader, ELF/PE parsing, IA-32 and AMD64 decoding, whole-sweep integrity, malformed input, the threaded decode, and a real system binary. Test inputs are generated when the suite runs, and CI runs it on Ubuntu and Windows on every push. For conclusive memory-safety results, configure with `-DVOIDWALK_SANITIZE=ON`.

---

## Architecture

```
main/
├── address-space/   memory-mapped file access
├── disassembler/    ELF/PE parsers, x86 decoder and opcode tables
├── miscellaneous/   format detection + disassembler factory
├── TUI/  GUI/       frontends
console-utils/       CLI commands
tests/               test suite
```

- **Format subclasses:** `make_disassembler()` picks `ELF_Disassembler` or `PE_Disassembler` from the magic bytes. Each subclass routes decoding by architecture, so new formats and architectures are added as subclasses.
- **Shared tables:** opcode maps are `constexpr` tables, used by both the decoder and the text renderer.
- **Sessions:** the frontends reach the core only through a `Session`, which runs the decode on a worker thread.

---

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

---

## Roadmap

**Near-term**
- [x] PE parser implementation (32-bit and 64-bit)
- [x] IA-32 instruction decoding support
- [x] AMD64 instruction decoding support
- [x] TUI
- [x] Tests
- [x] GUI
- [x] Console scripting interface

**Longer-term**
- [ ] Program-header fallback for stripped ELF binaries
- [ ] DWARF debug info parsing
- [ ] ARM32 Support
- [ ] AArch64 Support
- [ ] Memory Optimizations
- [ ] PE `.reloc` and `.pdata` support
- [ ] GUI jump tree with zoom/pan
- [ ] Dynamic tracer with breakpoint support
- [ ] AI-generated code explanation
- [ ] Export analysis report to file
- [ ] Simulated stack visualiser

---

## Vision

A complete static and dynamic analysis toolkit, usable from the CLI, the terminal and the desktop:

- **Debugger & dynamic analysis:** breakpoints, stepping, instruction tracing, branch and import-call logging, and memory-access monitoring. This is what the Run/Step controls and the register and stack panes in both UIs are for.
- **Emulation for foreign architectures:** run user-mode programs built for another architecture (for example, ARM binaries on an x86 machine, and the reverse) in an emulator, so you can step through and inspect them like native code.
- **Control-flow visualisation:** basic blocks and branch targets as a jump tree, in the CLI and as a zoomable, pannable view in the GUI.
- **Opt-in AI code insight:** explanations of the disassembly on screen, off unless you enable it. The GUI's assistant pane and its settings already exist.
