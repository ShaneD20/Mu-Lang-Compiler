#ifndef mu_memory_h
#define mu_memory_h

#include "common.h"
//TODO object

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : (capacity) << 1)

#define GROW_ARRAY(type, pointer, oldCount, newCount)    \
  (type*)reallocate(pointer, sizeof(type) * (oldCount), \
    sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

//strings
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif