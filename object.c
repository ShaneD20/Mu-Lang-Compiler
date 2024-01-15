#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "table.h"
#include "vm.h"

#define ALLOCATE_OBJECT(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Object* allocateObject(size_t size, ObjectType type) {
    Object* iObject = (Object*)reallocate(NULL, 0, size);
    iObject->type = type;
    iObject->next_pointer = vm.objects_pointer;
    vm.objects_pointer = iObject;
    return iObject;
}

FunctionObject* newFunction() {
    FunctionObject* oFunction = ALLOCATE_OBJECT(FunctionObject, FUNCTION_TYPE);
    oFunction->arity = 0;
    oFunction->name_pointer = NULL;
    oFunction->upvalueCount = 0; //TODO revisist
    initChunk(&oFunction->chunk);
    return oFunction;
}

NativeObject* newNative(NativeFn function) {
    NativeObject* oNative = ALLOCATE_OBJECT(NativeObject, NATIVE_TYPE);
    oNative->function = function;
    return oNative;
}

static void printFunction(FunctionObject* iFunction) {
    if (iFunction->name_pointer == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s", iFunction->name_pointer->runes_pointer);
}

static uint32_t hashString(const char* pointer, int length) {
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i += 1) {
        hash ^= (uint8_t)pointer[i];
        hash *= 16777619;
    }
    return hash;
}

static StringObject* allocateString(char* iRunes, int length, uint32_t hash) {
    StringObject* string = ALLOCATE_OBJECT(StringObject, STRING_TYPE);
    string->length = length;
    string->runes_pointer = iRunes;
    string->hash = hash;
    tableSet(&vm.strings, string, VALUE_VOID);
    return string;
}

StringObject* copyString(const char* iRunes, int length) {
    uint32_t hash = hashString(iRunes, length);
    StringObject* interned = tableFindString(&vm.strings, iRunes, length, hash);

    if (interned != NULL) return interned;

    char* heapRunes = ALLOCATE(char, length + 1);
    memcpy(heapRunes, iRunes, length);
    heapRunes[length] = '\0';
    return allocateString(heapRunes, length, hash);
}

StringObject* takeString(char* iRunes, int length) {
    uint32_t hash = hashString(iRunes, length);
    StringObject* interned = tableFindString(&vm.strings, iRunes, length, hash);

    if (interned != NULL) {
        FREE_ARRAY(char, iRunes, length + 1);
        return interned;
    }

    return allocateString(iRunes, length, hash);
}

void printObject(Value value) {
    switch (OBJECT_TYPE(value)) {
        case STRING_TYPE: 
            printf("%s", AS_C_STRING(value));
            break;
        case BOUND_METHOD_TYPE :
            break;
        case CLASS_TYPE :
            break;
        case CLOSURE_TYPE :
            break;
        case FUNCTION_TYPE : {
            printFunction(AS_FUNCTION(value));
            break;
        }
        case INSTANCE_TYPE :
            break;
        case NATIVE_TYPE :
            break;
        case UPVALUE_TYPE :
            break;
    }
}