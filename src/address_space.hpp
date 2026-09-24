#pragma once
#include <cstdint>
#include <string>
#include <cstring>

namespace voidwalk {

// A binary file mapped read-only into memory, addressed by file offset.
//
// The whole file is mapped once at construction (mmap / MapViewOfFile) and stays
// mapped for the object's lifetime, so seeking backwards costs nothing and no
// read ever copies the file. Every accessor is bounds-checked against the size
// the filesystem reported; nothing here trusts a header field.
//
// All failures - missing file, empty file, failed mapping, out-of-range read -
// are reported as std::length_error. Note that is a logic_error, NOT a
// runtime_error: a catch clause written for runtime_error will let it through.
// Callers that wrap file opening should catch std::exception.
//
// Threading: const after construction. Concurrent reads from any number of
// threads are safe; there are no writes.
class AddressSpace {
private:

	// base pointer to array of mmap
	void* base;

	// max file size in bytes from metadata
	size_t maxSize;



public:

	// Reads a T at `offset` in host byte order.
	//
	// Only instantiated for the four widths below - the definition lives in
	// address_space.cpp, so any other T fails to link.
	//
	// Throws std::length_error if [offset, offset + sizeof(T)) leaves the file.
	template <typename T>
	T readType(uint64_t offset);


	// Maps `filename` read-only for the object's lifetime.
	//
	// Throws std::length_error if the file cannot be opened or stat'd, if it is
	// empty, or if the mapping fails.
	AddressSpace(std::string filename);

	// Little-endian reads at a file offset, each bounds-checked.
	// Throw std::length_error when the read would run past end-of-file - which is
	// how the decoders detect a truncated instruction.
	uint8_t read_u8(uint64_t offset);
	uint16_t read_u16(uint64_t offset);
	uint32_t read_u32(uint64_t offset);
	uint64_t read_u64(uint64_t offset);

	// Size of the mapped file in bytes. Always > 0 for a constructed object.
	size_t size() noexcept;

	// Non-copyable: the mapping is owned, and the Disassembler holds a reference to
	// it for its whole life (see Session's lifetime rule).
	AddressSpace(const AddressSpace&) = delete;
	AddressSpace& operator=(const AddressSpace&) = delete;

	// Unmaps the file.
	~AddressSpace();
};

} // namespace voidwalk
