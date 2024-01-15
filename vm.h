#ifndef mu_vm_h
#define mu_vm_h

#include "object.h" // Calls and Functions vm-include-object
#include "table.h"  // Hash Tables vm-include-table
#include "value.h"  // vm-include-value

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
// Calls and Functions frame-max

typedef struct {
  ObjClosure* closure; // call-frame-closure
  uint8_t* ip;
  Value* slots;
} CallFrame;

typedef struct {
// Calls and Functions frame-array
  CallFrame frames[FRAMES_MAX];
  int frameCount; 
// vm-stack
  Value stack[STACK_MAX];
  Value* stackTop;
// Global Variables vm-globals
  Table globals;
// Hash Tables vm-strings
  Table strings;
// Methods and Initializers vm-init-string
  ObjString* initString;
// Closures open-upvalues-field
  ObjUpvalue* openUpvalues;

// Garbage Collection vm-fields
  size_t bytesAllocated;
  size_t nextGC;

// Strings objects-root
  Obj* objects;
// Garbage Collection vm-gray-stack
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm; // Strings extern-vm

void initVM();
void freeVM();

InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
