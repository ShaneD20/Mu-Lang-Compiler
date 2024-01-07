#ifndef mu_vm_h
#define mu_vm_h

#include "chunk.h"
#include "object.h"
#include "value.h"
#include "table.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  FunctionObject* function_pointer;
  uint8_t* ip;
  Value* slots_pointer;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  Value* top_pointer;
  Table globals;
  Table strings;
  Object* objects_pointer; // for Garbage Collection
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