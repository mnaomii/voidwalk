#include "disassembler.h"

void Disassembler::decode() {
	decodedInstructions.clear();
	instructionAddresses.clear();

	const uint64_t start = baseSections._text.getOffset();
	const uint64_t end = start + baseSections._text.getSize();

	uint64_t ptr = start;
	uint64_t vaddr = baseSections._text.getVaddr();

	while (ptr < end) {
		const uint64_t lineVaddr = vaddr;
		uint64_t next = decodeLine(ptr, vaddr);

		while (instructionAddresses.size() < decodedInstructions.size())
			instructionAddresses.push_back(lineVaddr);


		if (next <= ptr) break;

		vaddr += next - ptr;
		ptr = next;
	}
}
