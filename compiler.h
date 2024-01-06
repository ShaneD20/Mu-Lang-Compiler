#include <stdbool.h>
#ifndef mu_compiler_h
#define mu_compiler_h

#include "object.h"
#include "vm.h"

bool compile(const char* iSource, Chunk* iChunk);
static void grouping(bool assignable);
static void unary(bool assignable);
static void binary(bool assignable);
static void number(bool assignable);

#endif