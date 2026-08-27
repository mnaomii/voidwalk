#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include <cstdint>
#include <string>
#include <cstring>

class AddressSpace {
private:

	// base pointer to array of mmap
	void* base;

	// max file size in bytes from metadata
	size_t maxSize;



public:

	template <typename T>
	T readType(uint64_t offset);


	AddressSpace(std::string filename);

	uint8_t read_u8(uint64_t offset);
	uint16_t read_u16(uint64_t offset);
	uint32_t read_u32(uint64_t offset);
	uint64_t read_u64(uint64_t offset);

	const size_t size() noexcept;

	AddressSpace(const AddressSpace&) = delete;
	AddressSpace& operator=(const AddressSpace&) = delete;
	~AddressSpace();
};
#endif