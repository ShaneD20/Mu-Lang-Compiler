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

static void freeObject(Object* object) {
  switch (object->type) {
    case STRING_TYPE: {
      StringObject* string = (StringObject*)object;
      FREE_ARRAY(char, string->runes, string->length + 1);
      FREE(StringObject, object);
      break;
    }
  }
}

void freeObjects() {
  Object* object = vm.objects;
  while (object != NULL) {
    Object* next = object->next;
    freeObject(object);
    object = next;
  }
}