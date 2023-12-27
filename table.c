#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, StringObject* key) {
    uint32_t index = key->hash % capacity;
    /* 
        case for why loops should be expressions
        and shows the short-comings of for() while()
    */
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == key || entry->key == NULL) { //TODO string == string
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);

    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = VALUE_VOID;
    }
    for (int i = 0; i < table->capacity; i += 1) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* destination = findEntry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->value = entry->value;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableGet(Table* table, StringObject* key, Value* value) {
    if (table->count == 0) {
        return false;
    }
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    *value = entry->value;
    return true;
}

bool tableSet(Table* table, StringObject* key, Value value) {
    if (1 + table->count > TABLE_MAX_LOAD * table->capacity) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey) {
        table->count += 1;
    }
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i += 1) {
        Entry* entry = &from->entries[i];

        if(entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}