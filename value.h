#ifndef mu_value_h
#define mu_value_h

#include <string.h>
#include "common.h"

typedef struct Object Object;
typedef struct StringObject StringObject;


typedef enum {
  VALUE_VOID,
  VALUE_TF,
  VALUE_FLOAT,
  VALUE_OBJECT,
} ValueType;

/*
#ifdef NAN_BOXING
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)
#define TAG_VOID  1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_TF(value)      (((value) | 1) == TRUE_VAL)
#define IS_VOID(value)       ((value) == NIL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_TF(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)


#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double valueToNum(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}
static inline Value numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

#else
*/
typedef struct {
  ValueType type;
  union {
    bool TF;
    double number;
    Object* object_;
  } as;
} Value;

#define IS_TF(value)    ((value).type == VALUE_TF)
#define IS_VOID(value)     ((value).type == VALUE_VOID)
#define IS_NUMBER(value)  ((value).type == VALUE_FLOAT)
#define IS_OBJECT(value) ((value).type == VALUE_OBJECT)

#define AS_OBJECT(value) ((value).as.object_)
#define AS_TF(value)    ((value).as.TF)
#define AS_NUMBER(value)  ((value).as.number)

#define TF_VALUE(value)     ((Value){VALUE_TF, {.TF = value}})
#define VOID_VALUE          ((Value){VALUE_VOID, {.number = 0}})
#define NUMBER_VALUE(value) ((Value){VALUE_FLOAT, {.number = value}})
#define OBJECT_VALUE(input) ((Value){VALUE_OBJECT, {.object_ = (Object*)input}})

// #endif

typedef struct {
  int capacity;
  int count;
  Value* values_;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* iArray);
void writeValueArray(ValueArray* iArray, Value value);
void freeValueArray(ValueArray* iArray);
void printValue(Value value);

#endif