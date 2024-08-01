#ifndef mu_value_h
#define mu_value_h

#include <string.h>
#include "common.h"

//> Strings forward-declare object, string
typedef struct Obj Obj;
typedef struct ObjString ObjString;
//^ Strings forward-declare-obj

//> Optimization nan-boxing
#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000) // The bits that must be set to indicate a quiet NaN.

#define TAG_NAN   0
#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

// is ...
#define IS_NUMBER(value)(((value) & QNAN) != QNAN)
  //^ If the NaN bits exist then NaN
#define IS_OBJ(value)   (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
  //^ object pointer is a NaN with set sign bit
#define IS_BOOL(value)  (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)   ((value) == NIL_VAL)

// as ...
#define AS_BOOL(value)  ((value) == TRUE_VAL)
#define AS_OBJ(value)   ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define AS_NUMBER(value)valueToNum(value)

// value ...
#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double valueToNum(Value value) {
  double number;
  memcpy(&number, &value, sizeof(Value));
  return number;
}
static inline Value numToValue(double number) {
  Value value;
  memcpy(&value, &number, sizeof(double));
  return value;
}

#else

//< Optimization nan-boxing
//> Types of Values value-type
typedef enum {  // TODO when adding more types
  VAL_BOOL,
  VAL_NIL, // [user-types]
  VAL_NUMBER,
  VAL_OBJ, //> Strings val-obj
} ValueType;

//> Types of Values value
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj; //> Strings union-object
  } as; // [as]
} Value;

// is macros
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
//^ Strings is-obj

// as macros
#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
//^ Strings obj-val

#endif
//^ Optimization end-if-nan-boxing

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

//> array-fns-h
bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
//^ array-fns-h
void printValue(Value value);

#endif
