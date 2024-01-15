#ifndef mu_debug_h
#define mu_debug_h
#include "chunk.h"

void disassembleChunk(Chunk* iChunk, const char* iName);
int disassembleInstruction(Chunk* iChunk, int offset);

#endif