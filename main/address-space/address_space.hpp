#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include <fstream>
#include <cstdint>
#include <string>

class AddressSpace {
private:
	std::string binary;
	size_t maxOffset;
	std::ifstream f;

	void initialize(uint64_t& offset, size_t size);

public:

	AddressSpace(std::string filename);

	uint8_t read_u8(uint64_t offset);
	uint16_t read_u16(uint64_t offset);
	uint32_t read_u32(uint64_t offset);
	uint64_t read_u64(uint64_t offset);

	const size_t size() noexcept;



	template <typename T>
	T readType(uint64_t offset);

	AddressSpace(const AddressSpace& other);
	AddressSpace operator=(const AddressSpace& other);

	~AddressSpace();
};
#endif