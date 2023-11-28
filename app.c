#include <stdio.h>
#include <stdlib.h> // .h implies header file, provides an interface to the module
#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
  Chunk chunk;
  initChunk(&chunk);
  writeChunk(&chunk, OP_RETURN);
  disassembleChunk(&chunk, "test this chunk");
  freeChunk(&chunk);

  return EXIT_SUCCESS;
}
