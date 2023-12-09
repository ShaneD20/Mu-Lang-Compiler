#include <stdlib.h>

//TODO compiler
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  // vm.bytesAllocated += newSize - oldSize;

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);

  if (result == NULL) exit(1);

  return result;
}
