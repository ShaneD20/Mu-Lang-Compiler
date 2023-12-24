#ifndef mu_value_h
#define mu_value_h

//TODO string
#include "common.h"

/*
//typedef struct Obj obj;

// typedef struct ObjString ObjString;

#ifdef NAN_BOXING
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
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
typedef enum {
  VOID_VALUE,
  TF_VALUE,
  FLOAT_VALUE,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool TF;
    double number;
  } as;
} Value;

#define TF_VAL(value)   ((Value){TF_VALUE, {.boolean = value}})
#define VOID_VAL           ((Value){VOID_VALUE, {.number = 0}})
#define NUMBER_VAL(value) ((Value){FLOAT_VALUE, {.number = value}})
#define AS_TF(value)    ((value).as.TF)
#define AS_NUMBER(value)  ((value).as.number)
#define IS_TF(value)    ((value).type == TF_VALUE)
#define IS_VOID(value)     ((value).type == VOID_VALUE)
#define IS_NUMBER(value)  ((value).type == FLOAT_VALUE)
/*
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
#define AS_OBJ(value)     ((value).as.obj)
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#endif
*/
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