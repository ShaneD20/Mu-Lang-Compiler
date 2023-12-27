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

static uint32_t hashString(const char* key, int length) {
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i += 1) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

static StringObject* allocateString(char* runes, int length, uint32_t hash) {
    StringObject* string = ALLOCATE_OBJECT(StringObject, STRING_TYPE);
    string->length = length;
    string->runes = runes;
    string->hash = hash;
    return string;
}

StringObject* copyString(const char* runes, int length) {
    uint32_t hash = hashString(runes, length);
    char* heapRunes = ALLOCATE(char, length + 1);
    memcpy(heapRunes, runes, length);
    heapRunes[length] = '\0';
    return allocateString(heapRunes, length, hash);
}

StringObject* takeString(char* runes, int length) {
    uint32_t hash = hashString(runes, length);
    return allocateString(runes, length, hash);
}

void printObject(Value value) {
    switch (OBJECT_TYPE(value)) {
        case STRING_TYPE: 
            printf("%s", AS_C_STRING(value));
            break;
    }
}