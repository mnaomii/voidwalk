#include "address_space.hpp"
#include <atomic>
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
	#include <csetjmp>
	#include <csignal>

#endif

namespace voidwalk {

#ifndef _WIN32
// Set only while this thread is inside readType's memcpy.
static thread_local sigjmp_buf* sigbusJump = nullptr;

// A read touched a page the file no longer has (it shrank on disk): jump back into
// readType, which throws FileChanged. Any other SIGBUS restores the default action,
// so the faulting instruction re-runs and kills the process as before.
static void onSigbus(int) {
	if (sigbusJump) siglongjmp(*sigbusJump, 1);
	signal(SIGBUS, SIG_DFL);
}
#endif

AddressSpace::AddressSpace(std::string filePath) : mappedData(nullptr), fileSize(0){ // using mmap - ability to rewind in the file efficiently

	
#ifdef _WIN32
	
	// open file / get descriptor
	HANDLE fileHandle = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE) throw std::length_error("[voidwalk] : Inaccessible file.\n");

	LARGE_INTEGER sizeInfo;
	if (!GetFileSizeEx(fileHandle, &sizeInfo))
	{
		CloseHandle(fileHandle); 
		throw std::length_error("[voidwalk] : Empty file.\n");
	}


	// get filesize in bytes
	fileSize = sizeInfo.QuadPart;

	// windows equivalent of mmap
	HANDLE mappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (mappingHandle == nullptr) throw std::length_error("[voidwalk] : Invalid file.\n");
	mappedData = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);

	// close handles
	CloseHandle(fileHandle); CloseHandle(mappingHandle);

	if (!mappedData) throw std::length_error("[voidwalk] : Cannot map file.\n");


#else // POSIX ( macOS, Linux )
	

	// open a file / get the descriptor
	int fileDescriptor = open(filePath.c_str(), O_RDONLY | O_NONBLOCK);
	if (fileDescriptor < 0) throw std::length_error("[voidwalk] : Inaccessible file.\n");

	struct stat fileStats;
	if ( fstat(fileDescriptor, &fileStats) < 0 || !S_ISREG(fileStats.st_mode)) { // reject empty / non-regular files
		close(fileDescriptor);
		throw std::length_error("[voidwalk] : Invalid file.\n");
	}

	// get the size in bytes from the stats
	this->fileSize = fileStats.st_size;
	if (fileSize == 0) {close(fileDescriptor); throw std::length_error("[voidwalk] : Empty file.\n");}

	mappedData = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);
	close(fileDescriptor);

	if (mappedData == MAP_FAILED) throw std::length_error("[voidwalk] : Cannot map file.\n");

	// SA_NODEFER: siglongjmp leaves the handler without unblocking SIGBUS, so it must
	// never be blocked in the first place, or the next truncation kills the process.
	struct sigaction sigbusAction{};
	sigbusAction.sa_handler = onSigbus;
	sigbusAction.sa_flags = SA_NODEFER;
	sigaction(SIGBUS, &sigbusAction, nullptr);

#endif

}

// unmaps the file from memory
AddressSpace::~AddressSpace() {
#ifdef _WIN32
	
	UnmapViewOfFile(mappedData);

#else // POSIX

	munmap(mappedData, fileSize);

#endif
}


size_t AddressSpace::size() noexcept {
	return fileSize;
}


template <typename T>
T AddressSpace::readType(uint64_t offset) {


	if (offset > fileSize || sizeof(T) > fileSize - offset) throw std::length_error("[voidwalk] : Invalid offset.\n");
	T value{};
#ifndef _WIN32
	sigjmp_buf jumpBuffer;
	if (sigsetjmp(jumpBuffer, 0)) {   // returns 1 here when onSigbus jumps back
		sigbusJump = nullptr;
		throw FileChanged("[voidwalk] : file changed on disk.\n");
	}
	sigbusJump = &jumpBuffer;
	std::atomic_signal_fence(std::memory_order_seq_cst); // keep the read after the arm
#endif
	std::memcpy(&value, static_cast<const char*>(mappedData) + offset, sizeof(T));
#ifndef _WIN32
	std::atomic_signal_fence(std::memory_order_seq_cst); // ...and before the disarm
	sigbusJump = nullptr;
#endif
	return value;
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

} // namespace voidwalk
