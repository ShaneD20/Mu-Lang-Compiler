#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vm.h" // Garbage Collection

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constantPool);
}

void freeChunk(Chunk* chunk) {
// chunkfree; code, lines, constantpool. chunk re-initialize
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  freeValueArray(&chunk->constantPool);
  initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
  push(value); // Garbage Collection add-constant-push
  writeValueArray(&chunk->constantPool, value);
  pop(); // Garbage Collection add-constant-pop
  return chunk->constantPool.count - 1;
}
