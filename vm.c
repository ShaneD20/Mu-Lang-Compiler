#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include <stdio.h>

VM vm;

void initVM() {
  // resetStack();
}
void freeVM() {
  //TODO
}
void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop += 1;
}
Value pop() {
  vm.stackTop += -1; // moving down is enough to show a value as no longer in use.
  return *vm.stackTop;
}
static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
static void resetStack() {
  vm.stackTop = vm.stack;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip += 1)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
//lazily treats all numbers as doubles
#define BINARY_OP(valueType, op) \
    // do { \
    //   if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
    //     runtimeError("Operands must be numbers."); \
    //     return INTERPRET_RUNTIME_ERROR; \
    //   } \
    //   double b = AS_NUMBER(pop()); \
    //   double a = AS_NUMBER(pop()); \
    //   push(valueType(a op b)); \
    // } while (false)
// while (false) is to manage trailing semicolons 

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    /* Stack Tracing */
    printf(" ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot += 1) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    /* end Stack Tracking */
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT : {

        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_SUBTRACT: BINARY_OP(FLOAT_VALUE, -); 
          break;
      case OP_MULTIPLY: BINARY_OP(FLOAT_VALUE, *); 
        break;
      case OP_DIVIDE:   BINARY_OP(FLOAT_VALUE, /); 
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          // runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        // push(FLOAT_VALUE(-AS_NUMBER(pop())));
        break;
      case OP_RETURN : { //TODO change after implementing functions
        printValue(pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }
#undef READ_CONSTANT // undefining macros might seem solely fastidious but it helps the C preprocessor
#undef READ_BYTE
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();

  freeChunk(&chunk);
  return result;
}