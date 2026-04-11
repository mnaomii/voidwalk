# Disassembler & Dynamic Analysis Tool

A work-in-progress binary analysis tool written in C++ supporting both **ELF** and **PE** executable formats. Features static disassembly, dynamic analysis, a control flow visualizer (jump tree), and both a CLI and GUI interface.

---

##  Features

### ~ Format Support
- ELF (Linux/Unix executables)
- PE (Windows executables)
- Magic byte detection at load time — automatically selects the correct parser

### ~ Static Analysis
- Section parsing (`.text`, `.data`, `.rodata`, `.bss`, `.idata`, `.dynsym`, and more)
- Import/export resolution
- Symbol table parsing (`.symtab`, `.dynsym` for ELF — survives stripping)
- Relocation entry parsing (`.rel` / `.rela` for ELF)
- Function boundary recovery via `.eh_frame`
- Hex dump output with address offsets

### ~ Control Flow Visualization
- **Jump tree** — binary tree structure visualizing all jump/branch instructions and their targets
- Nodes represent basic blocks; edges represent conditional and unconditional jumps
- Rendered in both CLI (ASCII art) and GUI (interactive graph view)

### ~ Dynamic Analysis
- Runtime tracing of executed instructions
- Tracking of branch outcomes (taken / not taken)
- Import call logging — records which external functions are called and when
- Memory access tracking (reads/writes to key regions)
- Stack frame monitoring

### ~ Dual Interface
- **CLI** — full feature access from the terminal, scriptable and pipe-friendly
- **GUI** — interactive interface with section browser, disassembly view, and jump tree visualizer

---

##  Architecture

The project is structured around a polymorphic base class design:

```
Disassembler (abstract base)
├── ELFDisassembler
└── PEDisassembler
```

Each child class implements format-specific parsing while sharing common infrastructure (hex reader, section container, output routines) from the base class. A factory function instantiates the correct parser based on detected magic bytes.

---

##  Project Structure

```
.
├── src/
│   ├── core/
│   │   ├── disassembler.h        # Abstract base class
│   │   ├── elf_disassembler.h/cpp
│   │   └── pe_disassembler.h/cpp
│   ├── analysis/
│   │   ├── hex_reader.h          # Binary file loader
│   │   ├── section.h             # Generic section container
│   │   └── dynamic_tracer.h/cpp  # Runtime analysis
│   ├── visualization/
│   │   └── jump_tree.h/cpp       # Control flow binary tree
│   ├── cli/
│   │   └── cli.cpp               # CLI entry point
│   └── gui/
│       └── gui.cpp               # GUI entry point
├── CMakeLists.txt
└── README.md
```

---

##  Jump Tree

The jump tree is a binary tree where:
- Each **node** is a basic block (a linear sequence of instructions with no branches)
- The **left child** represents the branch-not-taken path
- The **right child** represents the branch-taken path
- Leaf nodes are either return instructions or unresolved indirect jumps

**CLI output example:**
```
[0x00401000] entry
├── [taken]     [0x00401020] loop_body
│   ├── [taken]     [0x00401020] loop_body  (back edge)
│   └── [not taken] [0x00401045] loop_exit
└── [not taken] [0x00401060] error_handler
```

**GUI** renders this as an interactive, zoomable graph with color-coded edges (green = taken, red = not taken).

---

##  Getting Started

### Requirements
????

### Build

```bash
git clone https://github.com/mnaomii/dynamic-analysis-tool.git
cd disassembler
mkdir build && cd build
cmake ..
make
```

### CLI Usage

```bash
# Detect format and print section info
./disassembler --sections target.elf

# Disassemble .text section
./disassembler --disasm target.exe

# Print hex dump
./disassemler --hex target.elf

# Show jump tree for a function at address
./disassembler --jumptree 0x401000 target.exe

# Run dynamic analysis (requires execution)
./disassembler --trace target.elf
```

### GUI Usage

```bash
./disassembler target.exe --gui
```

Opens the interactive interface where you can browse sections, view disassembly, and explore the jump tree visually.

---

##  Sections Parsed

### ELF
| Section | Parsed | Purpose |
|---|---|---|
| `.text` | ✅ | Executable code |
| `.data` | ✅ | Initialized globals |
| `.rodata` | ✅ | Read-only data / strings |
| `.bss` | ✅ | Uninitialized globals |
| `.symtab` | ✅ | Symbol table |
| `.dynsym` | ✅ | Dynamic symbols (survives stripping) |
| `.plt` / `.got` | ✅ | Dynamic call stubs |
| `.rel` / `.rela` | ✅ | Relocations |
| `.eh_frame` | ✅ | Function boundary recovery |
| `.debug_*` | 🔜 | DWARF debug info (planned) |

### PE
| Section | Parsed | Purpose |
|---|---|---|
| `.text` | ✅ | Executable code |
| `.data` | ✅ | Initialized globals |
| `.rdata` | ✅ | Read-only data / strings |
| `.bss` | ✅ | Uninitialized globals |
| `.idata` | ✅ | Import table |
| `.edata` | ✅ | Export table |
| `.rsrc` | 🔜 | Resources (planned) |
| `.reloc` | 🔜 | Base relocations (planned) |
| `.tls` | 🔜 | Thread local storage (planned) |
| `.pdata` | 🔜 | Exception handlers (planned) |

---

##  Roadmap

- [ ] x86 / x86-64 instruction decoder
- [ ] ARM support
- [ ] DWARF debug info parsing
- [ ] PE `.reloc` and `.pdata` support
- [ ] GUI jump tree with zoom/pan
- [ ] Dynamic tracer with breakpoint support
- [ ] Export analysis report to file

---

##  LICENSE
None.
