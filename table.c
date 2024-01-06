#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* iTable) {
    iTable->count = 0;
    iTable->capacity = 0;
    iTable->entries_pointer = NULL;
}

void freeTable(Table* iTable) {
    FREE_ARRAY(Entry, iTable->entries_pointer, iTable->capacity);
    initTable(iTable);
}

static Entry* findEntry(Entry* iEntries, int capacity, StringObject* iKey) {
    uint32_t index = iKey->hash % capacity;
    Entry* tombstone = NULL;
    /* 
        case for why loops should be expressions
        and shows the short-comings of for() while()
    */
    for (;;) {
        Entry* iEntry = &iEntries[index];
        if (iEntry->key_pointer == NULL) {
            if (IS_VOID(iEntry->value)) {
                return tombstone != NULL ? tombstone : iEntry;
            } else {
                if (tombstone == NULL) {
                    tombstone = iEntry;
                }
            } 
        } else if (iEntry->key_pointer == iKey) {
            return iEntry;
        }
        index = (index + 1) % capacity;
    }
}

static void adjustCapacity(Table* iTable, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);

    for (int i = 0; i < capacity; i++) {
        entries[i].key_pointer = NULL;
        entries[i].value = VOID_VALUE;
    }
    iTable->count = 0; // to manage tombstones
    for (int i = 0; i < iTable->capacity; i += 1) {
        Entry* iEntry = &iTable->entries_pointer[i];
        if (iEntry->key_pointer == NULL) continue;

        Entry* iDestination = findEntry(entries, capacity, iEntry->key_pointer);
        iDestination->key_pointer = iEntry->key_pointer;
        iDestination->value = iEntry->value;
        iTable->count += 1; // to manage tombstones
    }
    FREE_ARRAY(Entry, iTable->entries_pointer, iTable->capacity);
    iTable->entries_pointer = entries;
    iTable->capacity = capacity;
}

bool tableGet(Table* iTable, StringObject* iKey, Value* iValue) {
    if (iTable->count == 0) return false;
    
    Entry* iEntry = findEntry(iTable->entries_pointer, iTable->capacity, iKey);
    if (iEntry->key_pointer == NULL) return false;
    
    *iValue = iEntry->value;
    return true;
}

bool tableSet(Table* table, StringObject* key, Value value) {
    if (1 + table->count > TABLE_MAX_LOAD * table->capacity) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries_pointer, table->capacity, key);
    bool isNewKey = entry->key_pointer == NULL;
    if (isNewKey && IS_VOID(entry->value)) {
        table->count += 1;
    }
    entry->key_pointer = key;
    entry->value = value;
    return isNewKey;
}

bool deleteEntry(Table* table, StringObject* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries_pointer, table->capacity, key);
    if (entry->key_pointer == NULL) return false;

    entry->key_pointer = NULL;
    entry->value = TF_VALUE(true);
    return true;
}

void tableAddAll(Table* from, Table* iTable) {
    for (int i = 0; i < from->capacity; i += 1) {
        Entry* entry = &from->entries_pointer[i];

        if(entry->key_pointer != NULL) {
            tableSet(iTable, entry->key_pointer, entry->value);
        }
    }
}

StringObject* tableFindString(Table* iTable, const char* iRunes, int length, uint32_t hash) {
    if (iTable->count == 0) {
        return NULL;
    }  
    uint32_t index = hash % iTable->capacity;

    for(;;) {
        Entry* entry = &iTable->entries_pointer[index];
        if (entry->key_pointer == NULL) {
            if (IS_VOID(entry->value)) {
                return NULL; // stop if we find a non-tombstone
            }
        } else if (
            entry->key_pointer->length == length &&
            entry->key_pointer->hash == hash &&
            memcmp(entry->key_pointer->runes_pointer, iRunes, length) == 0) {
                return entry->key_pointer;
        }
        index = (index + 1) % iTable->capacity;
    }
}