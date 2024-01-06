#ifndef mu_table_h
#define mu_table_h

#include "common.h"
#include "object.h"

typedef struct {
    StringObject* key_pointer;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries_pointer;
} Table;

void initTable(Table* iTable);
void freeTable(Table* iTable);
bool tableGet(Table* iTable, StringObject* iKey, Value* iValue);
bool tableSet(Table* iTable, StringObject* iKey, Value value);
bool deleteEntry(Table* iTable, StringObject* iKey);
void tableAddAll(Table* from, Table* iTable);
StringObject* tableFindString(Table* iTable, const char* iRunes, int length, uint32_t hash);
#endif