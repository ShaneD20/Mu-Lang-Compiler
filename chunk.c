#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vm.h"

// provided a chunk of bytes, it will print instructions
void initChunk(Chunk* iChunk) {
  iChunk->count = 0;
  iChunk->capacity = 0;
  iChunk->code_pointer = NULL;
  iChunk->lines_pointer = NULL;
  initValueArray(&iChunk->constants);
}
void freeChunk(Chunk* iChunk) {
  FREE_ARRAY(uint8_t, iChunk->code_pointer, iChunk->capacity);
  FREE_ARRAY(int, iChunk->lines_pointer, iChunk->capacity);
  freeValueArray(&iChunk->constants);
  initChunk(iChunk);
}

void writeChunk(Chunk* iChunk, uint8_t byte, int line) {
  if (iChunk->capacity <iChunk->count + 1) {
    int size = iChunk->capacity;
    iChunk->capacity = GROW_CAPACITY(size);
    iChunk->code_pointer  = GROW_ARRAY(uint8_t, iChunk->code_pointer, size, iChunk->capacity);
    iChunk->lines_pointer = GROW_ARRAY(int, iChunk->lines_pointer, size, iChunk->capacity); // Book had oldCapacity??
  }

  iChunk->code_pointer[iChunk->count] = byte;
  iChunk->code_pointer[iChunk->count] = line;
  iChunk->count += 1;
}

int addConstant(Chunk* iChunk, Value value) {
  push(value);
  writeValueArray(&iChunk->constants, value);
  pop();
  return iChunk->constants.count - 1;
}