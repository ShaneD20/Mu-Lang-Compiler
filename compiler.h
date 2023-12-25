#include <stdbool.h>
#ifndef mu_compiler_h
#define mu_compiler_h

#include "object.h"
#include "vm.h"

bool compile(const char* source, Chunk* chunk);
static void grouping();
static void unary();
static void binary();
static void number();

#endif