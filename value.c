#include <stdio.h>
#include "memory.h"
#include "value.h"
//TODO string, object

void initValueArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}
// fortunately we don't need operations for insertion or removal

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count += 1;
}

void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

void printValue(Value value) {
  switch(value.type) {
    case TF_TYPE: printf(AS_TF(value) ? "true" : "false");
      break;
    case VOID_TYPE: printf("void");
      break;
    case FLOAT_TYPE: printf("%g", AS_NUMBER(value));
      break;
  }
  printf("%g", AS_NUMBER(value));
}