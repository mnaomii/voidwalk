#pragma once
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace voidwalk {

// Append-only sequence stored as fixed-size blocks, indexed through a block table
// that is reserved up front. Nothing a reader can reach is ever reallocated.
//
// Why not std::vector. The decode worker appends while the UI thread reads rows
// [0, readyCount). A vector that reallocates mid-append frees the buffer that
// reader is walking - the reference it holds is short-lived, but the race is
// inside the indexing itself, so that does not help. The previous fix was to
// reserve one slot per .text byte before the sweep: correct, but it allocates
// ~8x the real instruction count, and on an ELF whose sh_size lies it asks for an
// absurd amount. Here the worst case is applied to the block table
// instead, which is ChunkSize times smaller; blocks are allocated as they fill,
// so memory tracks the real instruction count.
//
// Why not std::deque. push_back leaves references to existing elements valid, but
// operator[] reads an internal map pointer that deque may reallocate as it grows,
// so a concurrent indexed read is still a data race.
//
// Threading contract. One writer calling push_back; any number of readers indexing
// [0, n), where n is a count the writer published with a release store *after* the
// corresponding push_back returned. reserveFor() must be called before the first
// concurrent read. clear(), reserveFor() and shrinkToFit() are writer-only and must
// not run with a reader in flight - the same rule decode() already follows.
template <typename T, std::size_t ChunkSize = 8192>
class ChunkStore {
    static_assert(ChunkSize > 0, "ChunkStore needs a non-zero block size");

public:
    using value_type = T;
    using size_type = std::size_t;

    ChunkStore() = default;

    // Neither copyable nor movable: readers hold a `const ChunkStore&` obtained
    // from the owning Disassembler, and moving the blocks out from under one is
    // the very failure this class exists to rule out.
    ChunkStore(const ChunkStore&) = delete;
    ChunkStore& operator=(const ChunkStore&) = delete;

    // Sizes the block table for `maxElements`, so no later push_back can reallocate
    // it. Call before the first push_back of a run; a caller that does not know the
    // bound may skip it, but then it owes readers its own guarantee that the table
    // is not growing while they read.
    void reserveFor(size_type maxElements) {
        blocks_.reserve(maxElements / ChunkSize + 1);
    }

    void push_back(T value) {
        const size_type slot = size_ % ChunkSize;
        // A new block is only ever appended to the table; the blocks already in it
        // keep their addresses, which is what makes an in-flight read safe.
        if (slot == 0) blocks_.push_back(std::make_unique<Block>());
        (*blocks_.back())[slot] = std::move(value);
        ++size_;
    }

    T& operator[](size_type i) { return (*blocks_[i / ChunkSize])[i % ChunkSize]; }
    const T& operator[](size_type i) const { return (*blocks_[i / ChunkSize])[i % ChunkSize]; }

    T& front() { return (*this)[0]; }
    const T& front() const { return (*this)[0]; }
    T& back() { return (*this)[size_ - 1]; }
    const T& back() const { return (*this)[size_ - 1]; }

    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    // Drops every block and the elements in them. The block table keeps whatever
    // capacity reserveFor() gave it, so a re-run does not have to reserve again.
    void clear() {
        blocks_.clear();
        size_ = 0;
    }

    void shrinkToFit() {
        blocks_.shrink_to_fit();
    }

    // Forward iteration, enough for a range-for and for constructing a container
    // from the range. Indexed access is the hot path; this is for the tests and for
    // whole-sweep walks.
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator() = default;
        const_iterator(const ChunkStore* store, size_type i) : store_(store), i_(i) {}

        reference operator*() const { return (*store_)[i_]; }
        pointer operator->() const { return &(*store_)[i_]; }

        const_iterator& operator++() { ++i_; return *this; }
        const_iterator operator++(int) { const_iterator prev = *this; ++i_; return prev; }

        bool operator==(const const_iterator& other) const { return i_ == other.i_; }
        bool operator!=(const const_iterator& other) const { return i_ != other.i_; }

    private:
        const ChunkStore* store_ = nullptr;
        size_type i_ = 0;
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, size_); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

private:
    using Block = std::array<T, ChunkSize>;

    std::vector<std::unique_ptr<Block>> blocks_;
    size_type size_ = 0;
};

} // namespace voidwalk
