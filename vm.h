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

//> TESTING for product types
// typedef struct {
//   uint8_t* ip;
//   Value* slots;
// } ProductType;
//^ TESTING

typedef struct {
// Array Calls and Functions
  CallFrame frames[FRAMES_MAX];
  int frameCount; 
// VM Stack
  Value stack[STACK_MAX];
  Value* stackTop;
// VM Tables
  Table globals; // Constants
  Table strings; // Strings
  ObjString* initString; // Methods and Initializers
  ObjUpvalue* openUpvalues; // Closures 

//> Garbage Collection fields
  size_t bytesAllocated;
  size_t nextGC;
// Strings Objects Root
  Obj* objects;
// Gray Stack
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
//^ Garbage Collection Fields
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
