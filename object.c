#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJECT(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Object* allocateObject(size_t size, ObjectBase type) {
    Object* object = (Object*)reallocate(NULL, 0, size);
    object->type = type;
    return object;
}

static ObjectString* allocateString(char* runes, int length) {
    ObjectString* string = ALLOCATE_OBJECT(ObjectString, STRING_BASE);
    string->length = length;
    string->runes = runes;
    return string;
}

ObjectString* copyString(const char* runes, int length) {
    char* heapRunes = ALLOCATE(char, length + 1);
    memcpy(heapRunes, runes, length);
    heapRunes[length] = '\0';
    return alloateString(heapRunes, length);
}