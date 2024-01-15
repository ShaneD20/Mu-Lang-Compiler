#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vm.h"

// provided a chunk of bytes, it will print instructions
void initChunk(Chunk* chunk_) { 
  chunk_->count = 0;
  chunk_->capacity = 0;
  chunk_->code_ = NULL;
  chunk_->lines_ = NULL;
  initValueArray(&chunk_->constantPool);
}
void freeChunk(Chunk* chunk_) { // good
  FREE_ARRAY(uint8_t, chunk_->code_, chunk_->capacity);
  FREE_ARRAY(int, chunk_->lines_, chunk_->capacity);
  freeValueArray(&chunk_->constantPool);
  initChunk(chunk_);
}

void writeChunk(Chunk* chunk_, uint8_t byte, int line) {
  if (chunk_->capacity < chunk_->count + 1) {
    int size = chunk_->capacity;
    chunk_->capacity = GROW_CAPACITY(size);
    chunk_->code_  = GROW_ARRAY(uint8_t, chunk_->code_, size, chunk_->capacity);
    chunk_->lines_ = GROW_ARRAY(int, chunk_->lines_, size, chunk_->capacity); // Book had oldCapacity??
  }

  chunk_->code_[chunk_->count] = byte;
  chunk_->code_[chunk_->count] = line;
  chunk_->count++;
}

int addConstant(Chunk* iChunk, Value value) {
  push(value);
  writeValueArray(&iChunk->constantPool, value);
  pop();
  return iChunk->constantPool.count - 1;
}