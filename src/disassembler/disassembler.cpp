#include "disassembler/disassembler.hpp"
#include "address_space.hpp"
#include "disassembler/format/section.hpp"

#include <iomanip>
#include <stdexcept>

namespace voidwalk {

void Disassembler::decode(std::stop_token stopToken) {


	decodedInstructions.clear();
	instructionAddresses.clear();

	size_t totalTextSize{};
	for (const auto textSection : commonSections.text)
		totalTextSize += textSection.getSize();

	decodedInstructions.reserveFor(totalTextSize);
	instructionAddresses.reserveFor(totalTextSize);

	readyCount.store(0, std::memory_order_relaxed);

	for (auto textSection : commonSections.text) {

		const uint64_t start = textSection.getOffset();
		const uint64_t end = start + textSection.getSize();

		uint64_t currentOffset = start;
		uint64_t vaddr = textSection.getVaddr();

		while (currentOffset < end) {
			if (stopToken.stop_requested()) break; // re-open / shutdown asked to bail

			const uint64_t lineVaddr = vaddr;
			uint64_t nextOffset = decodeLine(currentOffset, vaddr);

			while (instructionAddresses.size() < decodedInstructions.size())
				instructionAddresses.push_back(lineVaddr);


			readyCount.store(decodedInstructions.size(), std::memory_order_release);

			if (nextOffset <= currentOffset) break;

			emitDecodedLine();

			vaddr += nextOffset - currentOffset;
			currentOffset = nextOffset;

		}
	}
}



// Hands one instruction to the architecture's decoder. A null decoder means the
// container named a machine number we do not recognise at all - the message is the
// one the per-format switches used to throw for their `default` case.
uint64_t Disassembler::decodeLine(uint64_t fileOffset, uint64_t vaddr) {
	if (!decoder) throw std::runtime_error("Invalid architecture. Cannot parse.");
	return decoder->decodeLine(contents, fileOffset, vaddr, decodedInstructions);
}


	// prints the most recenlty decoded line to the embedded streams

void Disassembler::emitDecodedLine(bool showVaddr) {

	for(auto stream : outputStreams)
	{
		if (showVaddr)
			*stream << std::hex << std::setfill('0') << std::right << std::setw(8)
			<<   instructionAddresses[nextEmitIndex]  << ":  ";

		*stream << std::setfill(' ') << std::left << std::setw(24)
		<< decodedInstructions[nextEmitIndex]->getMachineCode() << ' '
		<< decodedInstructions[nextEmitIndex]->decodeLineString() << '\n';
	}
	nextEmitIndex++;
}

} // namespace voidwalk



