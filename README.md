# Disassembler & Dynamic Analysis Tool

An early-stage C++ binary analysis tool targeting **ELF** and **PE** executable formats. Automatically detects the binary format at load time and dispatches to the appropriate parser.

> **Status:** Active development. ELF x86/x86_64 section parsing is functional. PE support and instruction decoding are in progress.

---

## What Works Today

- **Format detection** — identifies ELF (`7F 45 4C 46`) and PE (`MZ` + PE signature) binaries from magic bytes and selects the correct parser automatically
- **ELF section parsing (x86 / x86_64)** — walks the section header table and populates headers for `.text`, `.data`, `.rodata`, `.bss`, `.symtab`, `.dynsym`, `.strtab`, `.dynstr`, `.plt`, `.got`, `.rel`/`.rela`, and `.eh_frame`
- **Architecture detection** — reports the target architecture (x86, x86_64, ARMv7, AArch64, etc.) from the ELF header
- **File-backed binary reader** — `AddressSpace` provides random-access reads (`read_u8/16/32/64`) directly from disk without loading the entire file into memory
- **PE parsing** — PE binaries are detected but the parser is a stub (`getArchitecture()` returns `"WIP"`)


### Not yet implemented

- **Instruction decoding / disassembly** — the `disassemble()` entry point exists but is not yet wired up

---

## Architecture

The project uses a polymorphic base class design with format-specific subclasses:

```
Disassembler (abstract base)
├── ELF_Disassembler
└── PE_Disassembler
```

**Key design decisions:**

- **Factory instantiation** — `make_disassembler()` inspects the first bytes of the file and returns the matching subclass. New formats are added by subclassing, not by branching inside existing code.
- **`AddressSpace`** — all binary reads go through a single file-backed reader. No raw buffer slicing or `reinterpret_cast` off an in-memory `vector`.
- **Parser dispatch** — bitness and architecture are handled by free functions in `parsers/` headers (e.g., `elf_32bit.h`, `elf_64bit.h`). Each bitness/arch combination is isolated in its own function.
- **`header` struct** — sections are represented as `{ uint64_t vaddr, offset, size }`, populated by the format-specific parsers onto the base class.

---

## Build

Builds with **Visual Studio** (MSVC) via the `.vcxproj` in `.vs-project/`. No CMake or cross-platform build system yet.

```
exec/dynamic-analysis-tool.exe <path-to-binary>
```

Expects exactly one argument: the path to an ELF or PE binary.

---

## Vision

The long-term goal is a complete binary analysis toolkit with both static and dynamic capabilities:

- **Control flow visualization** — a jump tree representing basic blocks and branch targets, rendered in both CLI and an interactive GUI
- **Dynamic analysis** — runtime instruction tracing, branch outcome tracking, import call logging, and memory access monitoring
- **Dual interface** — full CLI for scripting and a GUI with section browser, disassembly view, and jump tree visualizer

---

## Roadmap

**Near-term**
- [x] PE parser implementation (32-bit and 64-bit)
- [x] x86 / x86_64 instruction decoder
- [ ] ARM32 / AArch64 ELF support

**Longer-term**
- [ ] DWARF debug info parsing
- [ ] PE `.reloc` and `.pdata` support
- [ ] GUI jump tree with zoom/pan
- [ ] Dynamic tracer with breakpoint support
- [ ] AI-generated code explanation
- [ ] Export analysis report to file

---

## License

None.
