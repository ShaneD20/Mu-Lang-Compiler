//> Chunks of Bytecode memory-h
#ifndef mu_memory_h
#define mu_memory_h

#include "common.h"
#include "object.h"

//> Strings
#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
//^ Strings

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

//> array
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
//^ array

//  Garbage Collection
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();
//^ Garbage Collection
void freeObjects(); 
#endif
