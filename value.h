#ifndef mu_value_h
#define mu_value_h

//TODO string
#include "common.h"

typedef double Value;

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void initValueArray(ValueArray *array);

void writeValueArray(ValueArray *array, Value value);

void freeValueArray(ValueArray *array);

void printValue(Value value);

#endif