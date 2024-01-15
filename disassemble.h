//> Chunks of Bytecode debug-h
#ifndef mu_disassemble_h
#define mu_disassemble_h

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
