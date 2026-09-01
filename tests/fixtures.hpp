#pragma once
//
// Self-contained fixtures for the test suite.
//
// Every binary the tests need (a raw instruction stream, a minimal ELF, a minimal PE)
// is *synthesised in memory here and written to a throwaway temp file*. Nothing under
// .testing/ is required - that directory is .gitignored and never present in CI. This
// keeps the whole suite hermetic: `git clone` + build + run, no fixture download step.
//
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <atomic>
#include <filesystem>
#include <stdexcept>

#include "../main/address-space/address_space.hpp"

namespace fixtures {

// ---------------------------------------------------------------------------
// Little-endian byte buffer with append + patch, for hand-laying headers.
// ---------------------------------------------------------------------------
struct ByteBuf {
    std::vector<uint8_t> data;

    size_t size() const { return data.size(); }

    void u8(uint8_t v)  { data.push_back(v); }
    void u16(uint16_t v){ u8(v & 0xff); u8((v >> 8) & 0xff); }
    void u32(uint32_t v){ for (int i = 0; i < 4; ++i) u8((v >> (8 * i)) & 0xff); }
    void u64(uint64_t v){ for (int i = 0; i < 8; ++i) u8((v >> (8 * i)) & 0xff); }

    void bytes(const std::vector<uint8_t>& b) { data.insert(data.end(), b.begin(), b.end()); }
    void ascii(const std::string& s)          { data.insert(data.end(), s.begin(), s.end()); }
    void zeros(size_t n)                       { data.insert(data.end(), n, 0u); }
    void pad_to(size_t off)                    { if (data.size() < off) zeros(off - data.size()); }

    // Overwrite an already-reserved region at an absolute offset.
    void patch16(size_t off, uint16_t v){ data[off] = v & 0xff; data[off+1] = (v >> 8) & 0xff; }
    void patch32(size_t off, uint32_t v){ for (int i = 0; i < 4; ++i) data[off+i] = (v >> (8*i)) & 0xff; }
    void patch64(size_t off, uint64_t v){ for (int i = 0; i < 8; ++i) data[off+i] = (v >> (8*i)) & 0xff; }
};

// ---------------------------------------------------------------------------
// A temp file that removes itself. AddressSpace opens a file by name and holds
// it open, so the removal happens after any AddressSpace over it is destroyed.
// ---------------------------------------------------------------------------
class TempBinary {
    std::filesystem::path p_;
public:
    explicit TempBinary(const std::vector<uint8_t>& content) {
        static std::atomic<unsigned> counter{0};
        const unsigned id = counter.fetch_add(1);
        p_ = std::filesystem::temp_directory_path() /
             ("voidwalk_test_" + std::to_string(id) + ".bin");
        std::ofstream out(p_, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("TempBinary: cannot open temp file for writing");
        out.write(reinterpret_cast<const char*>(content.data()),
                  static_cast<std::streamsize>(content.size()));
        out.close();
    }
    ~TempBinary() {
        std::error_code ec;
        std::filesystem::remove(p_, ec);   // best-effort; never throws
    }
    TempBinary(const TempBinary&) = delete;
    TempBinary& operator=(const TempBinary&) = delete;

    std::string path() const { return p_.string(); }
};

// ---------------------------------------------------------------------------
// Instruction-stream fixture: the raw bytes plus generous trailing padding so
// the 15-byte-max byte-eater never reads off the end of the file (an off-the-end
// read throws length_error, which the decoder turns into "no progress").
// ---------------------------------------------------------------------------
inline std::vector<uint8_t> code(std::vector<uint8_t> bytes, bool pad = true) {
    if (pad) bytes.insert(bytes.end(), 16, 0x00);
    return bytes;
}

// ---------------------------------------------------------------------------
// Section-parse fixtures. The synthesised value of each populated field is
// fixed and known so a suite can assert against these constants.
// ---------------------------------------------------------------------------
struct SectionExpect { uint64_t vaddr, offset, size; };

struct ElfFixture {
    std::vector<uint8_t> bytes;
    SectionExpect text;   // the one populated base section we assert on
};

// Minimal ELF (class picked by is64) carrying a null section, a .text and a
// .shstrtab. e_machine is caller-chosen so the same builder feeds both the ELF
// section suite (real machines) and the loader suite.
//
// textBody defaults to "nop;nop;nop;ret". Pass your own to get a whole-file fixture
// whose .text is an exact instruction stream - that is what lets the sweep and
// robustness suites drive Disassembler::decode() (not just decodeLine) over chosen
// bytes, including hostile ones.
inline ElfFixture buildELF(bool is64, uint16_t e_machine,
                           std::vector<uint8_t> textBody = { 0x90, 0x90, 0x90, 0xc3 }) {
    ByteBuf b;
    const uint64_t textVaddr = is64 ? 0x401000ull : 0x08048000ull;
    const uint16_t shentsize = is64 ? 64 : 40;

    // .shstrtab blob; names are null-terminated, index 0 is the empty name.
    // "\0.text\0.shstrtab\0"
    const uint32_t nameText   = 1;
    const uint32_t nameShstr  = 7;
    std::vector<uint8_t> shstr;
    auto pushName = [&](const std::string& s){ for (char c : s) shstr.push_back((uint8_t)c); shstr.push_back(0); };
    shstr.push_back(0);        // index 0
    pushName(".text");         // index 1
    pushName(".shstrtab");     // index 7

    // ---- ELF header (52 bytes for 32-bit, 64 for 64-bit) ----
    const size_t ehsize = is64 ? 64 : 52;
    b.u8(0x7f); b.u8('E'); b.u8('L'); b.u8('F');        // magic
    b.u8(is64 ? 2 : 1);                                 // EI_CLASS
    b.u8(1);                                            // EI_DATA = little endian
    b.pad_to(ehsize);
    b.patch16(0x12, e_machine);                         // e_machine

    // ---- section bodies ----
    const uint64_t textOff  = b.size();
    b.bytes(textBody);
    const uint64_t shstrOff = b.size();
    b.bytes(shstr);

    // 4-byte align, then the section header table.
    b.pad_to((b.size() + 3) & ~size_t(3));
    const uint64_t shoff = b.size();

    // Three section headers: [0]=null, [1]=.text, [2]=.shstrtab.
    auto writeSH32 = [&](uint32_t name, uint32_t type, uint32_t addr, uint32_t off, uint32_t sz){
        b.u32(name); b.u32(type); b.u32(0 /*flags*/); b.u32(addr);
        b.u32(off);  b.u32(sz);   b.zeros(shentsize - 24);
    };
    auto writeSH64 = [&](uint32_t name, uint32_t type, uint64_t addr, uint64_t off, uint64_t sz){
        b.u32(name); b.u32(type); b.u64(0 /*flags*/); b.u64(addr);
        b.u64(off);  b.u64(sz);   b.zeros(shentsize - 40);
    };
    if (is64) {
        writeSH64(0, 0, 0, 0, 0);
        writeSH64(nameText,  1 /*PROGBITS*/, textVaddr, textOff, textBody.size());
        writeSH64(nameShstr, 3 /*STRTAB*/,   0,         shstrOff, shstr.size());
    } else {
        writeSH32(0, 0, 0, 0, 0);
        writeSH32(nameText,  1, (uint32_t)textVaddr, (uint32_t)textOff, (uint32_t)textBody.size());
        writeSH32(nameShstr, 3, 0,                   (uint32_t)shstrOff, (uint32_t)shstr.size());
    }

    // ---- patch the ELF header's section-table fields ----
    if (is64) {
        b.patch64(0x28, shoff);       // e_shoff
        b.patch16(0x3A, shentsize);   // e_shentsize
        b.patch16(0x3C, 3);           // e_shnum
        b.patch16(0x3E, 2);           // e_shstrndx
    } else {
        b.patch32(0x20, (uint32_t)shoff);  // e_shoff
        b.patch16(0x2e, shentsize);        // e_shentsize
        b.patch16(0x30, 3);                // e_shnum
        b.patch16(0x32, 2);                // e_shstrndx
    }

    return ElfFixture{ std::move(b.data), SectionExpect{ textVaddr, textOff, textBody.size() } };
}

struct PeFixture {
    std::vector<uint8_t> bytes;
    SectionExpect text, data, rodata, bss;
};

// Minimal PE (PE32 if !is64, else PE32+) with four sections that exercise every
// classify() bucket: CODE / DATA / RODATA / BSS, keyed by Characteristics flags.
inline PeFixture buildPE(bool is64) {
    ByteBuf b;
    const uint32_t elfanew  = 0x40;
    const uint16_t machine  = is64 ? 0x8664 : 0x014c;
    const uint16_t optSize  = is64 ? 0xF0 : 0xE0;
    const uint64_t imageBase= is64 ? 0x140000000ull : 0x00400000ull;

    // ---- DOS header ----
    b.u8('M'); b.u8('Z');
    b.pad_to(0x3C);
    b.u32(elfanew);          // e_lfanew @ 0x3C
    b.pad_to(elfanew);

    // ---- PE signature + COFF header (20 bytes), starting at elfanew+4 ----
    b.u32(0x00004550);       // "PE\0\0"
    b.u16(machine);          // Machine
    b.u16(4);                // NumberOfSections
    b.u32(0);                // TimeDateStamp
    b.u32(0);                // PointerToSymbolTable
    b.u32(0);                // NumberOfSymbols
    b.u16(optSize);          // SizeOfOptionalHeader
    b.u16(0);                // Characteristics

    // ---- optional header (zero-filled, ImageBase patched in) ----
    const size_t opt = b.size();        // == elfanew + 24
    b.zeros(optSize);
    if (is64) b.patch64(opt + 24, imageBase);   // PE32+: ImageBase @ opt+24 (u64)
    else      b.patch32(opt + 28, (uint32_t)imageBase); // PE32: ImageBase @ opt+28 (u32)

    // ---- section table (40 bytes each) ----
    auto writeSection = [&](const std::string& name, uint32_t vsize, uint32_t vaddr,
                            uint32_t rawSize, uint32_t rawPtr, uint32_t chars){
        std::string n = name; n.resize(8, '\0');
        for (int i = 0; i < 8; ++i) b.u8((uint8_t)n[i]);
        b.u32(vsize);        // +8  VirtualSize
        b.u32(vaddr);        // +12 VirtualAddress (RVA)
        b.u32(rawSize);      // +16 SizeOfRawData
        b.u32(rawPtr);       // +20 PointerToRawData
        b.u32(0);            // +24 PointerToRelocations
        b.u32(0);            // +28 PointerToLinenumbers
        b.u16(0);            // +32 NumberOfRelocations
        b.u16(0);            // +34 NumberOfLinenumbers
        b.u32(chars);        // +36 Characteristics
    };
    writeSection(".text",  0x1000, 0x1000, 0x200, 0x400, 0x60000020); // EXECUTE|CNT_CODE|READ  -> CODE
    writeSection(".data",  0x1000, 0x2000, 0x300, 0x600, 0xC0000040); // INIT_DATA|READ|WRITE   -> DATA
    writeSection(".rdata", 0x1000, 0x3000, 0x100, 0x900, 0x40000040); // INIT_DATA|READ         -> RODATA
    writeSection(".bss",   0x1000, 0x4000, 0x050, 0x000, 0xC0000080); // UNINIT_DATA|READ|WRITE -> BSS

    PeFixture f;
    f.bytes  = std::move(b.data);
    f.text   = { imageBase + 0x1000, 0x400, 0x200 };
    f.data   = { imageBase + 0x2000, 0x600, 0x300 };
    f.rodata = { imageBase + 0x3000, 0x900, 0x100 };
    f.bss    = { imageBase + 0x4000, 0x000, 0x050 };
    return f;
}

// ---------------------------------------------------------------------------
// Hostile-input fixtures.
//
// A disassembler's normal workload includes files that are truncated, packed,
// deliberately malformed, or simply not what their headers claim. These builders
// produce that class of input so the robustness suite can assert the only
// acceptable behaviours: parse it, or reject it. Never crash, never hang.
// ---------------------------------------------------------------------------

// Patch one field of an otherwise valid ELF. `off` is an absolute file offset -
// the caller uses the same header offsets the parser reads.
inline std::vector<uint8_t> elfWith64(bool is64, uint16_t machine, size_t off, uint64_t value) {
    auto fx = buildELF(is64, machine);
    ByteBuf b; b.data = std::move(fx.bytes);
    if (off + 8 <= b.size()) b.patch64(off, value);
    return std::move(b.data);
}
inline std::vector<uint8_t> elfWith32(bool is64, uint16_t machine, size_t off, uint32_t value) {
    auto fx = buildELF(is64, machine);
    ByteBuf b; b.data = std::move(fx.bytes);
    if (off + 4 <= b.size()) b.patch32(off, value);
    return std::move(b.data);
}
inline std::vector<uint8_t> elfWith16(bool is64, uint16_t machine, size_t off, uint16_t value) {
    auto fx = buildELF(is64, machine);
    ByteBuf b; b.data = std::move(fx.bytes);
    if (off + 2 <= b.size()) b.patch16(off, value);
    return std::move(b.data);
}

// Offsets of the ELF64 header fields the section parser trusts, so the robustness
// suite can name what it is corrupting instead of hard-coding magic numbers.
namespace elf64 {
    constexpr size_t e_machine   = 0x12;
    constexpr size_t e_shoff     = 0x28;
    constexpr size_t e_shentsize = 0x3A;
    constexpr size_t e_shnum     = 0x3C;
    constexpr size_t e_shstrndx  = 0x3E;
}

// The file offset of section header `idx`'s `field` byte, for an ELF64 fixture -
// used to corrupt a specific section's sh_offset / sh_size after the fact.
inline size_t elf64SectionField(const std::vector<uint8_t>& elf, uint16_t idx, size_t field) {
    uint64_t shoff = 0; uint16_t shentsize = 0;
    for (int i = 0; i < 8; ++i) shoff |= uint64_t(elf[elf64::e_shoff + i]) << (8 * i);
    for (int i = 0; i < 2; ++i) shentsize |= uint16_t(elf[elf64::e_shentsize + i]) << (8 * i);
    return static_cast<size_t>(shoff + uint64_t(idx) * shentsize + field);
}
namespace elf64_sh {
    constexpr size_t sh_addr   = 0x10;
    constexpr size_t sh_offset = 0x18;
    constexpr size_t sh_size   = 0x20;
}

// Truncate a fixture to `n` bytes - the "download stopped halfway" case, which
// every header read past the cut must survive.
inline std::vector<uint8_t> truncated(std::vector<uint8_t> v, size_t n) {
    if (n < v.size()) v.resize(n);
    return v;
}

// ---------------------------------------------------------------------------
// Instruction streams that have historically broken the byte-eater. Named so a
// failing check says WHAT shape of input broke it, not just an opcode blob.
// ---------------------------------------------------------------------------
namespace streams {

// 15 legacy prefixes fill instructionBytes[15] exactly; the byte after it is the
// one with nowhere left to go.
inline std::vector<uint8_t> prefixRun(uint8_t tail, int count = 15) {
    std::vector<uint8_t> v(static_cast<size_t>(count), 0x66);
    v.push_back(tail);
    return v;
}

inline std::vector<uint8_t> maxPrefixesThenRex()    { return prefixRun(0x48); } // REX.W
inline std::vector<uint8_t> maxPrefixesThenOpcode() { return prefixRun(0x90); } // NOP

} // namespace streams

// ---------------------------------------------------------------------------
// A real, compiler-produced binary to sweep end to end.
//
// Synthetic fixtures cannot catch a decoder that stops after one instruction on
// real code - the property only shows up over thousands of consecutive, genuinely
// emitted instructions. The suite therefore looks for a native executable on the
// host. It is allowed to find nothing: the integration suite then SKIPS rather
// than fails, so a minimal CI container stays green.
// ---------------------------------------------------------------------------
inline std::string findSystemBinary() {
#ifdef _WIN32
    const char* candidates[] = {
        "C:\\Windows\\System32\\notepad.exe",
        "C:\\Windows\\System32\\cmd.exe",
        "C:\\Windows\\System32\\kernel32.dll",
    };
#else
    const char* candidates[] = {
        "/bin/ls", "/usr/bin/ls",
        "/bin/cat", "/usr/bin/cat",
        "/bin/sh",
    };
#endif
    for (const char* c : candidates) {
        std::error_code ec;
        // Must be a regular file with content; a symlink to one is fine (status follows it).
        if (std::filesystem::is_regular_file(c, ec) &&
            std::filesystem::file_size(c, ec) > 4096 && !ec)
            return c;
    }
    return {};
}

} // namespace fixtures
