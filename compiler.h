#include <stdbool.h>
#ifndef mu_compiler_h
#define mu_compiler_h

#include "vm.h"

bool compile(const char* source, Chunk* chunk);
void grouping();
void unary();
void binary();
void number();

#endif