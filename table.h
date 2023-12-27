#ifndef mu_table_h
#define mu_table_h

#include "common.h"
#include "object.h"

typedef struct {
    StringObject* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, StringObject* key, Value value);
bool tableSet(Table* table, StringObject* key, Value value);
bool deleteEntry(Table* table, StringObject* key);
void tableAddAll(Table* from, Table* to);
StringObject* tableFindString(Table* table, const char* runes, int length);
#endif