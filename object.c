#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJECT(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Object* allocateObject(size_t size, ObjectType type) {
    Object* object = (Object*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static StringObject* allocateString(char* runes, int length) {
    StringObject* string = ALLOCATE_OBJECT(StringObject, STRING_TYPE);
    string->length = length;
    string->runes = runes;
    return string;
}

StringObject* copyString(const char* runes, int length) {
    char* heapRunes = ALLOCATE(char, length + 1);
    memcpy(heapRunes, runes, length);
    heapRunes[length] = '\0';
    return allocateString(heapRunes, length);
}

StringObject* takeString(char* runes, int length) {
    return allocateString(runes, length);
}

void printObject(Value value) {
    switch (OBJECT_TYPE(value)) {
        case STRING_TYPE: 
            printf("%s", AS_C_STRING(value));
            break;
    }
}