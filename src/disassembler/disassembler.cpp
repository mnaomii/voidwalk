#include "disassembler/disassembler.hpp"
#include "address_space.hpp"
#include "disassembler/format/section.hpp"

#include <iomanip>
#include <stdexcept>

namespace voidwalk {

void Disassembler::decode(std::stop_token stopToken) {


	decodedInstructions.clear();
	instructionAddresses.clear();


	readyCount.store(0, std::memory_order_relaxed);

	const uint64_t start = baseSections._text.getOffset();
	const uint64_t end = start + baseSections._text.getSize();


	const auto worst = static_cast<size_t>(baseSections._text.getSize());
	decodedInstructions.reserve(worst);
	instructionAddresses.reserve(worst);

	uint64_t ptr = start;
	uint64_t vaddr = baseSections._text.getVaddr();

	while (ptr < end) {
		if (stopToken.stop_requested()) break; // re-open / shutdown asked to bail

		const uint64_t lineVaddr = vaddr;
		uint64_t next = decodeLine(ptr, vaddr);

		while (instructionAddresses.size() < decodedInstructions.size())
			instructionAddresses.push_back(lineVaddr);


		readyCount.store(decodedInstructions.size(), std::memory_order_release);

		if (next <= ptr) break;

		emitDecodedLine();

		vaddr += next - ptr;
		ptr = next;

	}

}



// Hands one instruction to the architecture's decoder. A null decoder means the
// container named a machine number we do not recognise at all - the message is the
// one the per-format switches used to throw for their `default` case.
uint64_t Disassembler::decodeLine(uint64_t address, uint64_t vaddr) {
	if (!decoder) throw std::runtime_error("Invalid architecture. Cannot parse.");
	return decoder->decodeLine(contents, address, vaddr, decodedInstructions);
}


// prints the most recenlty decoded line to the embedded streams
void Disassembler::emitDecodedLine() {

	for(auto stream : outputStreams)
		*stream << std::hex << std::setfill('0') << std::setw(8)
		<< instructionAddresses[instrDecodePos] << ":  "
		<< std::setfill(' ') << std::left << std::setw(24)
		<< decodedInstructions[instrDecodePos]->getMachineCode() << ' '
		<< decodedInstructions[instrDecodePos]->decodeLineString() << '\n';

	instrDecodePos++;
}

} // namespace voidwalk



