#include <stdio.h>
#include "memory.h"
#include "value.h"
#include "object.h"
#include <string.h>
//TODO string, object

void initValueArray(ValueArray* array) {
  array->values_pointer = NULL;
  array->capacity = 0;
  array->count = 0;
}
// fortunately we don't need operations for insertion or removal

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values_pointer = GROW_ARRAY(Value, array->values_pointer, oldCapacity, array->capacity);
  }

  array->values_pointer[array->count] = value;
  array->count += 1;
}

void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values_pointer, array->capacity);
  initValueArray(array);
}

void printValue(Value value) {
  switch(value.type) {
    case VALUE_TF: printf(AS_TF(value) ? "true" : "false");
      break;
    case VALUE_VOID: printf("void");
      break;
    case VALUE_FLOAT: printf("%g", AS_NUMBER(value));
      break;
    case VALUE_OBJECT: printObject(value);
      break;
  }
  printf("%g", AS_NUMBER(value));
}

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
    case VALUE_TF: return AS_TF(a) == AS_TF(b);
    case VALUE_VOID: return true;
    case VALUE_FLOAT: return AS_NUMBER(a) == AS_NUMBER(b);
    case VALUE_OBJECT: return AS_OBJECT(a) == AS_OBJECT(b);
    default: return false;
  }
}