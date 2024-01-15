//> Strings object-h
#ifndef mu_object_h
#define mu_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

// is ...
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

// as ...
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

// structs, enums ...
typedef enum {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE
} ObjType;

struct Obj {
  ObjType type; 
  bool isMarked; // Garbage Collection is-marked-field
  struct Obj* next;
};

typedef struct {
  Obj obj;
  int arity;
  int upvalueCount; // Closures upvalue-count
  Chunk chunk;
  ObjString* name;
} ObjFunction;

//> Calls and Functions obj-native
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;
//^ Calls and Functions obj-native

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash; // Hash Tables obj-string-hash
};

// Closures
typedef struct ObjUpvalue {
  Obj obj;
  Value* location;
  Value closed;
  struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
} ObjClosure;
//^ Closures

//> Classes and Instances
typedef struct {
  Obj obj;
  ObjString* name;
  Table methods; // Methods and Initializers class-methods
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* klass;
  Table fields; // [fields]
} ObjInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;
//^ Classes and Instances

// functions ...
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
