#ifndef mu_value_h
#define mu_value_h

//TODO string
#include "common.h"

typedef enum {
  VOID_TYPE,
  TF_TYPE,
  FLOAT_TYPE,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool TF;
    double number;
  } as;
} Value;

// typedef struct Obj obj;
// typedef struct ObjString ObjString;
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)
#define TAG_VOID  1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

#ifdef NAN_BOXING

#define FALSE_VALUE ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VALUE  ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL     ((Value)(uint64_t)(QNAN | TAG_VOID))
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

#define IS_TF(value)    ((value).type == TF_TYPE)
#define IS_VOID(value)     ((value).type == VOID_TYPE)
#define IS_NUMBER(value)  ((value).type == FLOAT_TYPE)
#define AS_TF(value)    ((value).as.TF)
#define AS_NUMBER(value)  ((value).as.number)

#define TF_VALUE(value)   ((Value){TF_TYPE, {.TF = value}})
#define NUMBER_VALUE(value) ((Value){FLOAT_TYPE, {.number = value}})
#define VOID_VALUE           ((Value){VOID_TYPE, {.number = 0}})

//#define IS_OBJ(value)     ((value).type == VAL_OBJ)
//#define AS_OBJ(value)     ((value).as.obj)
//#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif