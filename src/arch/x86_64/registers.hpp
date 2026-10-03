#pragma once
#include <cstdint>

namespace voidwalk {

    template<size_t T>
    class ExtendedType
    {

        std::array<uint8_t,T> bytes{};
        size_t byteCount{};

        public:
            ExtendedType() = default;

            // adds another byte to the representation, stores it in little endian
            ExtendedType& operator+=(const uint8_t& value){ if (byteCount < T) bytes[byteCount++] = value; return *this; }

            uint64_t qword(const size_t index) const
            {
                uint64_t result{};
                memcpy(&result, bytes.data() + index * 8, 8 );
                return result;
            }
    };

// Emulated x86/x86-64 register file. Every field reads zero until a debugger
// engine exists to populate it.
struct Registers_x86_64 { // emulating the current values of the registers.
    uint64_t rax, rdx, rcx, rbx, rsp, rbp, rsi, rdi, rip; // registers + eip - current instruction pointer;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t ds, cs, fs, gs, ss, es; // segments

    uint8_t flags;

    // mmx

    uint64_t mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7;

    // sse
    ExtendedType<16> xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
                    xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;

    // AVX/VEX

    ExtendedType<32> ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7,
                    ymm8, ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15;

    // AVX512/EVEX

    ExtendedType<64> zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7,
                    zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15,
                    zmm16, zmm17, zmm18, zmm19, zmm20, zmm21, zmm22, zmm23,
                    zmm24, zmm25, zmm26, zmm27, zmm28, zmm29, zmm30, zmm31;
};

// The vector registers in index order, so frontends can list them in a loop.
inline constexpr std::array<uint64_t Registers_x86_64::*, 8> kMmRegs{
    &Registers_x86_64::mm0, &Registers_x86_64::mm1, &Registers_x86_64::mm2, &Registers_x86_64::mm3,
    &Registers_x86_64::mm4, &Registers_x86_64::mm5, &Registers_x86_64::mm6, &Registers_x86_64::mm7,
};
inline constexpr std::array<ExtendedType<16> Registers_x86_64::*, 16> kXmmRegs{
    &Registers_x86_64::xmm0, &Registers_x86_64::xmm1, &Registers_x86_64::xmm2, &Registers_x86_64::xmm3,
    &Registers_x86_64::xmm4, &Registers_x86_64::xmm5, &Registers_x86_64::xmm6, &Registers_x86_64::xmm7,
    &Registers_x86_64::xmm8, &Registers_x86_64::xmm9, &Registers_x86_64::xmm10, &Registers_x86_64::xmm11,
    &Registers_x86_64::xmm12, &Registers_x86_64::xmm13, &Registers_x86_64::xmm14, &Registers_x86_64::xmm15,
};
inline constexpr std::array<ExtendedType<32> Registers_x86_64::*, 16> kYmmRegs{
    &Registers_x86_64::ymm0, &Registers_x86_64::ymm1, &Registers_x86_64::ymm2, &Registers_x86_64::ymm3,
    &Registers_x86_64::ymm4, &Registers_x86_64::ymm5, &Registers_x86_64::ymm6, &Registers_x86_64::ymm7,
    &Registers_x86_64::ymm8, &Registers_x86_64::ymm9, &Registers_x86_64::ymm10, &Registers_x86_64::ymm11,
    &Registers_x86_64::ymm12, &Registers_x86_64::ymm13, &Registers_x86_64::ymm14, &Registers_x86_64::ymm15,
};
inline constexpr std::array<ExtendedType<64> Registers_x86_64::*, 32> kZmmRegs{
    &Registers_x86_64::zmm0, &Registers_x86_64::zmm1, &Registers_x86_64::zmm2, &Registers_x86_64::zmm3,
    &Registers_x86_64::zmm4, &Registers_x86_64::zmm5, &Registers_x86_64::zmm6, &Registers_x86_64::zmm7,
    &Registers_x86_64::zmm8, &Registers_x86_64::zmm9, &Registers_x86_64::zmm10, &Registers_x86_64::zmm11,
    &Registers_x86_64::zmm12, &Registers_x86_64::zmm13, &Registers_x86_64::zmm14, &Registers_x86_64::zmm15,
    &Registers_x86_64::zmm16, &Registers_x86_64::zmm17, &Registers_x86_64::zmm18, &Registers_x86_64::zmm19,
    &Registers_x86_64::zmm20, &Registers_x86_64::zmm21, &Registers_x86_64::zmm22, &Registers_x86_64::zmm23,
    &Registers_x86_64::zmm24, &Registers_x86_64::zmm25, &Registers_x86_64::zmm26, &Registers_x86_64::zmm27,
    &Registers_x86_64::zmm28, &Registers_x86_64::zmm29, &Registers_x86_64::zmm30, &Registers_x86_64::zmm31,
};

} // namespace voidwalk
