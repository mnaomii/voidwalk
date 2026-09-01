#pragma once
//
// Async decode suite - the contract between Disassembler::decode() and the frontends.
//
// Both the GUI and the TUI run decode() on a worker thread and render partial results
// while it is still going. That handoff is deliberately lock-free, and its rules are
// written down in disassembler.cpp:17-24:
//
//   * the vectors are reserved worst-case up front, so they never reallocate mid-sweep
//   * the worker publishes progress with a release-store to readyCount
//   * a reader that acquires readyCount may index [0, readyCount) safely
//
// Those rules are load-bearing for two frontends, and nothing tested them. A lock-free
// protocol that is only checked by reading it is a protocol that drifts: the invariant
// can be broken by a line added anywhere in decode(), far from the comment that states
// it, and single-threaded tests will never notice.
//
// So this suite runs an actual reader against an actual worker, in the same shape
// gui::Session uses, and asserts the protocol holds. The concurrent cases are isolated
// (see isolate.hpp) because the failure mode is a fault, not a wrong value.
//
#include "../base.hpp"
#include "../fixtures.hpp"
#include "../isolate.hpp"
#include "../../main/disassembler/ELF/elf_disassembler.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

class AsyncDecode_Tests : public Tests {

    // A .text big enough that a sweep is still running when a reader starts. Built by
    // repeating a realistic instruction block - real compiler-emitted shapes, so the
    // decoder does the same work per byte it does on a real binary.
    static std::vector<uint8_t> largeText(size_t repeats) {
        const std::vector<uint8_t> block = {
            0x55,                                     // PUSH RBP
            0x48, 0x89, 0xe5,                         // MOV RBP, RSP
            0x48, 0x83, 0xec, 0x20,                   // SUB RSP, 0x20
            0x89, 0xc3,                               // MOV EBX, EAX
            0x8b, 0x45, 0xf8,                         // MOV EAX, [RBP-8]
            0x83, 0xf8, 0x01,                         // CMP EAX, 1
            0x74, 0x02,                               // JE +2
            0x31, 0xc0,                               // XOR EAX, EAX
            0x48, 0x83, 0xc4, 0x20,                   // ADD RSP, 0x20
            0x5d,                                     // POP RBP
            0xc3,                                     // RET
        };
        std::vector<uint8_t> out;
        out.reserve(block.size() * repeats);
        for (size_t i = 0; i < repeats; ++i)
            out.insert(out.end(), block.begin(), block.end());
        return out;
    }

    struct Target {
        std::unique_ptr<fixtures::TempBinary> file;
        std::unique_ptr<AddressSpace> space;
        std::unique_ptr<ELF_Disassembler> disasm;
    };

    static Target makeTarget(size_t repeats) {
        Target t;
        const auto fx = fixtures::buildELF(true, 0x3E, largeText(repeats));
        t.file  = std::make_unique<fixtures::TempBinary>(fx.bytes);
        t.space = std::make_unique<AddressSpace>(t.file->path());
        t.disasm= std::make_unique<ELF_Disassembler>(*t.space, std::vector<std::ostream*>{});
        return t;
    }

    // --- the published count is monotonic and never overstates -------------
    // A reader trusts readyCount as an upper bound on valid indices. If it could ever
    // exceed the number of decoded instructions, every frontend would read past the end.
    void testReadyCountIsSaneAfterDecode() {
        auto t = makeTarget(200);
        t.disasm->decode();
        const size_t ready = t.disasm->readyInstructions();
        expect_eq((long long)ready, (long long)t.disasm->getDecodedInstructions().size(),
                  "readyInstructions() equals the decoded count when the sweep ends");
        expect_eq((long long)t.disasm->getInstructionAddresses().size(), (long long)ready,
                  "the address vector is as long as the published count");
        expect(ready > 1000, "the large fixture really does decode a few thousand instructions");
    }

    // --- cancellation --------------------------------------------------------
    // A re-open or a window close has to be able to stop a long sweep. decode() polls
    // the stop_token each line; an already-stopped token must end it almost immediately.
    void testStopTokenCancels() {
        auto t = makeTarget(20000);          // far more than we intend to finish

        std::stop_source src;
        src.request_stop();                   // stopped BEFORE decode is entered

        const auto begin = std::chrono::steady_clock::now();
        t.disasm->decode(src.get_token());
        const auto elapsed = std::chrono::steady_clock::now() - begin;

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        expect(t.disasm->getDecodedInstructions().size() <= 1,
               "a pre-stopped token stops the sweep at once (decoded "
               + std::to_string(t.disasm->getDecodedInstructions().size()) + ")");
        expect(ms < 1000, "a pre-stopped decode returns promptly (" + std::to_string(ms) + " ms)");
    }

    // Stopping mid-flight must also work, and must leave the partial results usable -
    // both frontends keep and display whatever decoded before the stop.
    void testStopMidSweepKeepsPartialResults() {
        auto t = makeTarget(20000);
        std::stop_source src;

        std::thread worker([&]{ t.disasm->decode(src.get_token()); });

        // Let it get going, then cancel.
        size_t seen = 0;
        for (int i = 0; i < 200 && seen < 50; ++i) {
            seen = t.disasm->readyInstructions();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        src.request_stop();
        worker.join();

        const size_t total = t.disasm->getDecodedInstructions().size();
        expect(total == t.disasm->getInstructionAddresses().size(),
               "after a mid-sweep stop the two vectors are still the same length");
        expect_eq((long long)t.disasm->readyInstructions(), (long long)total,
                  "after a mid-sweep stop the published count matches what was decoded");
    }

    // --- the lock-free handoff under a real concurrent reader ----------------
    // This mirrors gui::Session exactly: the worker sweeps while the reader polls
    // readyInstructions() and dereferences rows in [0, ready). If the backing storage
    // can move during the sweep, the reader is holding freed memory - which is a fault,
    // not a wrong answer, so the case has to run isolated to be observable.
    void testConcurrentReaderIsSafe() {
        if (!isolate::supported) { skip("concurrent reader", "no crash isolation"); return; }

        const auto outcome = isolate::run([]{
            const auto fx = fixtures::buildELF(true, 0x3E, largeText(20000));
            fixtures::TempBinary tmp(fx.bytes);
            AddressSpace as(tmp.path());
            ELF_Disassembler d(as, {});

            std::atomic<bool> running{true};
            std::thread worker([&]{
                try { d.decode(); } catch (...) {}
                running.store(false, std::memory_order_release);
            });

            // The reader side of gui::Session::rowText()/rowVaddr().
            volatile size_t sink = 0;
            while (running.load(std::memory_order_acquire)) {
                const size_t ready = d.readyInstructions();
                const auto& ins = d.getDecodedInstructions();
                const auto& addr = d.getInstructionAddresses();
                for (size_t i = ready > 500 ? ready - 500 : 0; i < ready; ++i) {
                    sink += addr[i];
                    sink += ins[i]->decodeLineString().size();
                }
            }
            worker.join();
        });

        // No xfail marker: the shrink_to_fit() calls that used to reallocate the vectors
        // out from under a live reader are gone, so the handoff is held to the real
        // standard. This is the regression test for that fix - it is what detected it.
        expect(outcome != isolate::Outcome::Crashed,
               std::string("reader polling [0, ready) during a live sweep -> ")
                   + isolate::describe(outcome));
    }

    // The published count must never run ahead of the data behind it. Sampling it from
    // another thread and checking it against the vector sizes at the end catches a
    // count published before the rows it covers were written.
    void testPublishedCountNeverOverstates() {
        if (!isolate::supported) { skip("published-count sampling", "no crash isolation"); return; }

        const auto outcome = isolate::run([]{
            const auto fx = fixtures::buildELF(true, 0x3E, largeText(5000));
            fixtures::TempBinary tmp(fx.bytes);
            AddressSpace as(tmp.path());
            ELF_Disassembler d(as, {});

            std::atomic<bool> running{true};
            std::atomic<size_t> maxSeen{0};
            std::thread reader([&]{
                size_t prev = 0;
                while (running.load(std::memory_order_acquire)) {
                    const size_t r = d.readyInstructions();
                    if (r < prev) std::_Exit(3);      // went backwards: not monotonic
                    prev = r;
                    maxSeen.store(r, std::memory_order_relaxed);
                }
            });

            try { d.decode(); } catch (...) {}
            running.store(false, std::memory_order_release);
            reader.join();

            if (maxSeen.load() > d.getDecodedInstructions().size()) std::_Exit(4);
        });

        expect(outcome != isolate::Outcome::Crashed,
               std::string("published count stays monotonic and within the decoded rows -> ")
                   + isolate::describe(outcome));
    }

    void runAll() {
        running("testReadyCountIsSaneAfterDecode");     testReadyCountIsSaneAfterDecode();
        running("testStopTokenCancels");                testStopTokenCancels();
        running("testStopMidSweepKeepsPartialResults"); testStopMidSweepKeepsPartialResults();
        running("testConcurrentReaderIsSafe");          testConcurrentReaderIsSafe();
        running("testPublishedCountNeverOverstates");   testPublishedCountNeverOverstates();
    }

public:
    AsyncDecode_Tests() { runAll(); }
};
