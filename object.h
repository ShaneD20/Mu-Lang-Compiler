#ifndef mu_object_h
#define mu_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)

#define IS_METHOD(value)  isObjectType(value, BOUND_METHOD_BASE)
#define IS_CLOSURE(value) isObjectType(value, CLOSURE_BASE)
#define IS_FUNCTION(value) isObjectType(value, FUNCTION_BASE)
#define IS_NATIVE(value)  isObjectType(value, NATIVE_BASE)
#define IS_STRING(value)  isObjectType(value, STRING_BASE)

#define AS_METHOD(value) ((BoundMethod*)AS_OBJECT(value))
#define AS_CLOSURE(value)((ClosureObject)AS_OBJECT(value))
#define AS_FUNCTION(value)((FunctionObject)AS_OBJECT(value))
#define AS_NATIVE(value) ((NativeObject)AS_OBJECT(value))
#define AS_STRING(value) ((ObjectString*)AS_OBJECT(value))
#define AS_C_STRING(value)((ObjectString*)AS_OBJECT(value)->runes)

typedef enum {
    BOUND_METHOD_BASE,
    CLASS_BASE,
    CLOSURE_BASE,
    FUNCTION_BASE,
    INSTANCE_BASE,
    NATIVE_BASE,
    STRING_BASE, // OBJ_STRING in book
    UPVALUE_BASE,
} ObjectBase;

struct Object {
    ObjectBase type;
    //bool isMarked;
    //struct Object* next;
};
typedef struct {
    Object object;
    int length;
    char* runes;
} ObjectString;

typedef struct {
    Object object;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjectString* name;
} FunctionObject;

typedef struct {
    Object object;
    NativeFn function;
} NativeObject;

typedef struct {
    Object object;
    Value* location;
    Value closed;
    struct UpvalueObject* next;
} UpvalueObject;

typedef struct {
    Object object;
    FunctionObject* function;
    UpvalueObject** upvalues;
    int upvalueCount;
} ClosureObject;

typedef struct {
    Object object;
    Value reciever;
    ClosureObject* method;
} BoundMethod;

// typedef class
// typedef instance

typedef Value (*NativeFn)(int argCount, Value* args);

BoundMethod* newBoundMethod(Value reciever, ClosureObject* method);
// class
// Instance
ClosureObject newClosure(FunctionObject* function);
FunctionObject newFunction();
NativeObject newNative(NativeFn function);
StringObj copyString(const char* runes, int length);
StringObj takeString(char* runes, int length);
UpvalueObject newUpvalue(Value* slot);
void printObject(Value value);

static inline bool isObjectType(Value value, ObjectBase type) {
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif