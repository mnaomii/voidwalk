#pragma once
//
// AddressSpace suite - the file-backed random-access reader every other layer reads
// through. Covers little-endian width reads, size(), boundary reads, and the
// std::length_error that an out-of-range read must raise (it is a logic_error, not a
// runtime_error - a distinction main.cpp's catch clauses actually depend on).
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../../main/address-space/address_space.hpp"
#include <stdexcept>

class AddressSpace_Tests : public Tests {

    // Known 10-byte payload; every expected value below is derived from it.
    // bytes: 01 02 03 04 05 06 07 08 ff ee   (offsets 0..9)
    std::vector<uint8_t> payload() {
        return { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xff, 0xee };
    }

    template <class F>
    bool raisesLengthError(F&& f) {
        try { f(); return false; }
        catch (const std::length_error&) { return true; }
        catch (...) { return false; }   // any other type is the wrong exception
    }

    void testSize() {
        fixtures::TempBinary tmp(payload());
        AddressSpace as(tmp.path());
        expect_eq((long long)as.size(), 10, "size() == byte count");
    }

    void testLittleEndianReads() {
        fixtures::TempBinary tmp(payload());
        AddressSpace as(tmp.path());
        expect_eq((long long)as.read_u8(0),  0x01,               "read_u8 at 0");
        expect_eq((long long)as.read_u8(9),  0xee,               "read_u8 at last byte");
        expect_eq((long long)as.read_u16(0), 0x0201,             "read_u16 little-endian");
        expect_eq((long long)as.read_u16(8), 0xeeff,             "read_u16 across last two bytes");
        expect_eq((long long)as.read_u32(0), 0x04030201LL,       "read_u32 little-endian");
        expect_eq((long long)as.read_u32(6), 0xeeff0807LL,       "read_u32 ending at boundary");
        expect_eq((long long)as.read_u64(0), 0x0807060504030201LL,"read_u64 little-endian");
    }

    void testBoundaries() {
        fixtures::TempBinary tmp(payload());
        AddressSpace as(tmp.path());

        // Reads that end exactly at size() are valid.
        expect(!raisesLengthError([&]{ as.read_u8(9);  }), "read_u8 at size-1 is valid");
        expect(!raisesLengthError([&]{ as.read_u32(6); }), "read_u32 ending at size is valid");
        expect(!raisesLengthError([&]{ as.read_u64(2); }), "read_u64 ending at size is valid");

        // Reads that run past the end throw std::length_error (logic_error), not runtime_error.
        expect(raisesLengthError([&]{ as.read_u8(10);  }), "read_u8 past end throws length_error");
        expect(raisesLengthError([&]{ as.read_u16(9);  }), "read_u16 past end throws length_error");
        expect(raisesLengthError([&]{ as.read_u32(7);  }), "read_u32 past end throws length_error");
        expect(raisesLengthError([&]{ as.read_u64(3);  }), "read_u64 past end throws length_error");
    }

    void testMissingFile() {
        bool threw = false;
        try {
            AddressSpace as("this_path_should_not_exist_voidwalk_test.xyz");
        } catch (const std::exception&) { threw = true; }
        expect(threw, "opening a missing file throws");
    }

    void runAll() {
        running("testSize");             testSize();
        running("testLittleEndianReads");testLittleEndianReads();
        running("testBoundaries");       testBoundaries();
        running("testMissingFile");      testMissingFile();
    }

public:
    AddressSpace_Tests() { runAll(); }
};
