#include "address_space.hpp"
#include <stdexcept>


#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN   // skip niche headers
	#define NOMINMAX              // stop windows.h from clobbering std::min/std::max
	#include <windows.h>

#else

	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <unistd.h>

#endif

AddressSpace::AddressSpace(std::string filename) : base(nullptr), maxSize(0){ // using mmap - ability to rewind in the file efficiently

	
#ifdef _WIN32
	
	// open file / get descriptor
	HANDLE f = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (f == INVALID_HANDLE_VALUE) throw std::length_error("[voidwalk] : Invalid file.\n");

	LARGE_INTEGER size;
	if (!GetFileSizeEx(f, &size))
	{
		CloseHandle(f); 
		throw std::length_error("[voidwalk] : Invalid file.\n");
	}


	// get filesize in bytes
	maxSize = size.QuadPart;

	// windows equivalent of mmap
	HANDLE fMap = CreateFileMapping(f, nullptr, PAGE_READONLY, 0, 0, nullptr);
	base = MapViewOfFile(fMap, FILE_MAP_READ, 0, 0, 0);

	// close handles
	CloseHandle(f); CloseHandle(fMap);

	if (!base) throw std::length_error("[voidwalk] : cannot map file.\n");


#else // POSIX ( macOS, Linux )
	

	// open a file / get the descriptor
	FILE* f = open(filename.c_str(), O_RDONLY);
	if (f < 0) throw std::length_error("[voidwalk] : Invalid file.\n");

	struct stat st;
	stat(f, &st);

	// get the size in bytes from the stats
	this->maxSize = st.st_size;
	if (maxSize == 0) std::length_error("[voidwalk] : Invalid file.\n");

	base = mmap(nullptr, maxSize, PROT_READ, MAP_PRIVATE, f, 0);
	close(f);

	if (base == MAP_FAILED) throw std::length_error("[voidwalk] : cannot map file.\n");

#endif

}

// unmaps the file from memory
AddressSpace::~AddressSpace() {
#ifdef _WIN32
	
	UnmapViewOfFile(base);

#else // POSIX

	munmap(base, maxSize);

#endif
}


const size_t AddressSpace::size() noexcept {
	return maxSize;
}


template <typename T>
T AddressSpace::readType(uint64_t offset) {


	if (offset + sizeof(T) > maxSize ) throw std::length_error("Reading past bounds.\n");
	T val{};
	std::memcpy(&val, static_cast<const char*>(base) + offset, sizeof(T));
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