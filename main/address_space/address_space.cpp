#include "address_space.h"
#include <stdexcept>

AddressSpace::AddressSpace(std::string filename) {
	this->binary = filename;

	this->f.open(this->binary, std::ios::binary | std::ios::ate);
	if (!this->f.is_open()) throw std::runtime_error("Cannot open executable.");

	this->maxOffset = this->f.tellg();
	this->f.close();

	this->f.open(this->binary, std::ios::binary);
	if (!this->f.is_open()) throw std::runtime_error("Executable cannot be read.");

}

AddressSpace::~AddressSpace() {
	if (this->f.is_open())
		f.close();
}

AddressSpace::AddressSpace(const AddressSpace& other) {
	this->binary = other.binary;
	this->maxOffset = other.maxOffset;
	this->f.open(this->binary, std::ios::binary);
	if (!this->f.is_open()) throw std::runtime_error("Executable cannot be read.");

}
AddressSpace AddressSpace::operator=(const AddressSpace& other) {

	this->binary = other.binary;
	this->maxOffset = other.maxOffset;
	this->f.open(this->binary, std::ios::binary);
	if (!this->f.is_open()) throw std::runtime_error("Executable cannot be read.");

	return *this;

}

const size_t AddressSpace::size() noexcept {
	return this->maxOffset;
}

void AddressSpace::initialize(uint64_t& offset, size_t size) {
	if (static_cast<size_t>(offset) + size > this->maxOffset) throw std::length_error("Invalid offset.");
	f.clear();
	this->f.seekg(offset, std::ios::beg);
}


template <typename T>
T AddressSpace::readType(uint64_t offset) {

	try {
		this->initialize(offset, sizeof(T));
	}
	catch (const std::length_error& e) { throw; }
	catch (const std::runtime_error& e) { throw; }
	T val;
	f.read(reinterpret_cast<char*>(&val), sizeof(T));
	return val;
}

uint8_t AddressSpace::read_u8(uint64_t offset) {
	return this->readType<uint8_t>(offset);

}
uint16_t AddressSpace::read_u16(uint64_t offset){
	return this->readType<uint16_t>(offset);

}
uint32_t AddressSpace::read_u32(uint64_t offset) {
	return this->readType<uint32_t>(offset);


}
uint64_t AddressSpace::read_u64(uint64_t offset) {
	return this->readType<uint64_t>(offset);

}