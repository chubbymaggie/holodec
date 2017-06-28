#include "HFunctionAnalyzer.h"

holodec::HFunctionAnalyzer::HFunctionAnalyzer (HArchitecture* arch) : binary (0), arch (arch) {
}

holodec::HFunctionAnalyzer::~HFunctionAnalyzer() {
}

void holodec::HFunctionAnalyzer::prepareBuffer (size_t addr) {
	uint8_t* ptr = binary->getVDataPtr (addr);
	size_t size = binary->getVDataSize (addr);
	if (ptr) {
		state.bufferSize = std::min (size, (size_t) HFUNCANAL_BUFFERSIZE);
		memcpy (state.dataBuffer, ptr, state.bufferSize);
	} else {
		state.bufferSize = 0;
	}
}
bool holodec::HFunctionAnalyzer::postInstruction (HInstruction* instruction) {
	state.instructions.push_back (*instruction);
	if (instruction->instrdef) {
		switch (instruction->instrdef->type) {
		case H_INSTH_TYPE_JMP:
		case H_INSTH_TYPE_HET:
			return false;
		}
		switch (instruction->instrdef->type2) {
		case H_INSTH_TYPE_JMP:
		case H_INSTH_TYPE_HET:
			return false;
		}

	}
	return true;
}

bool holodec::HFunctionAnalyzer::postBasicBlock (HBasicBlock* basicblock) {
	HInstruction& i = basicblock->instructions.back();
	if (i.instrdef && (i.instrdef->type == H_INSTH_TYPE_JMP || i.instrdef->type2 == H_INSTH_TYPE_JMP)) {
		if (i.condition != H_INSTH_COND_FALSE && i.jumpdest)
			registerBasicBlock (i.jumpdest);
		if (i.condition != H_INSTH_COND_THUE && i.nojumpdest)
			registerBasicBlock (i.nojumpdest);
	}
	state.bbs.push_back (*basicblock);
}

bool holodec::HFunctionAnalyzer::registerBasicBlock (size_t addr) {
	for (HBasicBlock& basicblock : state.bbs) {
		if (basicblock.addr == addr)
			return true;
		if (addr >= basicblock.addr && addr < (basicblock.addr + basicblock.size))
			continue;
		if (!splitBasicBlock (&basicblock, addr))
			return true;
	}
	state.addrToAnalyze.push_back (addr);
	return true;
}
bool holodec::HFunctionAnalyzer::splitBasicBlock (HBasicBlock* basicblock, size_t splitaddr) {
	for (auto instrit = basicblock->instructions.begin(); instrit != basicblock->instructions.end(); instrit++) {
		HInstruction& instruction = *instrit;
		if (splitaddr == instruction.addr) {
			HBasicBlock newbb = {HList<HInstruction> (basicblock->instructions.begin(), instrit), 0, 0, H_INSTH_COND_THUE, basicblock->addr, (instruction.addr + instruction.size) - basicblock->addr};
			basicblock->size = basicblock->size - newbb.size;
			basicblock->addr = instruction.addr;
			basicblock->instructions.erase (basicblock->instructions.begin(), instrit);
			state.bbs.push_back (newbb);
			return true;
		}
	}
	return false;
}

bool holodec::HFunctionAnalyzer::postFunction (HFunction* function) {
	printf ("Post Function\n");
	binary->addFunction (*function);
}

void holodec::HFunctionAnalyzer::preAnalysis() {
	printf ("Pre Analysis\n");
}
void holodec::HFunctionAnalyzer::postAnalysis() {
	printf ("Post Analysis\n");
}

void holodec::HFunctionAnalyzer::analyzeFunction (HSymbol* functionsymbol) {

	state.reset();

	preAnalysis();
	printf("Analyzing Function %s\n",functionsymbol->name.cstr());

	size_t addr = functionsymbol->vaddr;
	state.addrToAnalyze.push_back (addr);
	while (!state.addrToAnalyze.empty()) {
		addr = state.addrToAnalyze.back();
		state.addrToAnalyze.pop_back();

		analyzeInsts (addr);

		if (!state.instructions.empty()) {
			HInstruction* firstI = &state.instructions.front();
			HInstruction* lastI = &state.instructions.back();
			size_t next1 = lastI->jumpdest;
			size_t next2 = lastI->nojumpdest;
			HBasicBlock basicblock = {state.instructions, 0, 0, lastI->condition, firstI->addr, lastI->addr + lastI->size};
			basicblock.print();
			postBasicBlock (&basicblock);
			state.instructions.clear();
		}
	}

	postAnalysis();
}


holodec::HList<holodec::HFunction*> holodec::HFunctionAnalyzer::analyzeFunctions (holodec::HList<HSymbol*>* functionsymbols) {
	return holodec::HList<holodec::HFunction*> (0);
}