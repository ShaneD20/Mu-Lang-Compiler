#include "common.h"
#include "vm.h"

VM vm;

void initVM() {}

void freeVM() {}

InterpretResult interpret(Chunk *chunk)
{
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}