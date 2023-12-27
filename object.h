#ifndef mu_object_h
#define mu_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)  //OBJ_TYPE

#define IS_METHOD(value)  isObjectType(value, BOUND_METHOD_TYP)
#define IS_CLASS(value)        isObjType(value, CLASS_TYPE)
#define IS_CLOSURE(value) isObjectType(value, CLOSURE_TYPE)
#define IS_FUNCTION(value) isObjectType(value, FUNCTION_TYPE)
#define IS_INSTANCE(value) isObjectType(value, INSTANCE_TYPE)
#define IS_NATIVE(value)  isObjectType(value, NATIVE_TYPE)
#define IS_STRING(value)  isObjectType(value, STRING_TYPE)

#define AS_METHOD(value) ((BoundMethod*)AS_OBJECT(value))
#define AS_CLASS(value) (())
#define AS_CLOSURE(value)((ClosureObject)AS_OBJECT(value))
#define AS_FUNCTION(value)((FunctionObject)AS_OBJECT(value))
#define AS_NATIVE(value) ((NativeObject)AS_OBJECT(value))
#define AS_STRING(value) ((StringObject*)AS_OBJECT(value))
#define AS_C_STRING(value)(((StringObject*)AS_OBJECT(value))->runes)

typedef enum {
    BOUND_METHOD_TYPE,
    CLASS_TYPE,
    CLOSURE_TYPE,
    FUNCTION_TYPE,
    INSTANCE_TYPE,
    NATIVE_TYPE,
    STRING_TYPE, // OBJ_STRING in book, ETC
    UPVALUE_TYPE,
} ObjectType;

struct Object {
    ObjectType type;
    //bool isMarked;
    struct Object* next; // makes the object heap a linked list for garbage collection
};

typedef struct {
    Object object;
    int length;
    char* runes;
    uint32_t hash;
} StringObject; //ObjString etc

typedef struct {
    Object object;
    int arity;
    int upvalueCount;
    Chunk chunk;
    StringObject* name;
} FunctionObject;

// typedef struct {
//     Object object;
//     NativeFn function;
// } NativeObject;

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
    StringObject* name;
    // Table methods;
} ClassObject;

typedef struct {
    Object object;
    ClassObject* model;
    // Table fields;
} InstanceObject;

typedef struct {
    Object object;
    Value reciever;
    ClosureObject* method;
} BoundMethod;

// typedef class
// typedef instance

typedef Value (*NativeFn)(int argCount, Value* args);

BoundMethod* newBoundMethod(Value reciever, ClosureObject* method);
ClassObject* newClass(StringObject* name);
InstanceObject* newInstance(ClassObject* model);
ClosureObject* newClosure(FunctionObject* function);
FunctionObject* newFunction();
//NativeObject* newNative(NativeFn function);
StringObject* copyString(const char* runes, int length);
StringObject* takeString(char* runes, int length);
UpvalueObject* newUpvalue(Value* slot);
void printObject(Value value);

static inline bool isObjectType(Value value, ObjectType type) {
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif