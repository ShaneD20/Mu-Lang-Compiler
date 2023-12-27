#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

VM vm;

void freeVM() {
  freeObjects();
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
  // vm.stackTop = vm.stack;
  vm.objects = NULL;
}
void initVM() {
  resetStack();
}
static bool isFalsey(Value value) {
  return (IS_TF(value) && !AS_TF(value));
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];

  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();

}

static void concatenate() {
  StringObject* right = AS_STRING(pop());
  StringObject* left = AS_STRING(pop());
  int length = left->length + right->length;

  char* runes = ALLOCATE(char, length + 1);
  memcpy(runes, left->runes, left->length);
  memcpy(runes + left->length, right->runes, right->length);
  runes[length] = '\0';

  StringObject* result = takeString(runes, length);
  push(OBJECT_VALUE(result));

}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip += 1)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
//lazily treats all numbers as doubles
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
  } while (false)
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
    uint8_t instruction; //TODO
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT : {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_VOID: push(VOID_VALUE);
        break;
      case OP_TRUE: push(TF_VALUE(true));
        break;
      case OP_FALSE: push(TF_VALUE(false));
        break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(TF_VALUE(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER: BINARY_OP(TF_VALUE, >);
        break;
      case OP_LESS: BINARY_OP(TF_VALUE, <);
        break;
      case OP_ADD: BINARY_OP(NUMBER_VALUE, +);
        break;
      case OP_CONCATENATE: { //   TODO get to be polish notation
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else {
          runtimeError("both operands must be strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VALUE, -); 
        break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VALUE, *); 
        break;
      case OP_DIVIDE: BINARY_OP(NUMBER_VALUE, /); 
        break;
      case OP_NOT: push(TF_VALUE(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VALUE(-AS_NUMBER(pop())));
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