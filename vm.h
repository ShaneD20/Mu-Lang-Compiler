#ifndef mu_vm_h
#define mu_vm_h

#include "chunk.h"
#include "object.h"
#include "value.h"
#include "table.h"
#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip; // always points to the next instruction, not the one currently being handled
  Value stack[STACK_MAX];
  Value* stackTop;
  Table strings;
  Table globals;
  Object* objects; // for Garbage Collection
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();

InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif