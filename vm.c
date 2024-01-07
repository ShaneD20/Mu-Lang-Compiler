#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"
// #include "table.h"

VM vm;

void freeVM() {
  freeTable(&vm.strings);
  freeTable(&vm.globals);
  freeObjects();
}
void push(Value value) {
  *vm.top_pointer = value;
  vm.top_pointer += 1;
}
Value pop() {
  vm.top_pointer += -1; // moving down is enough to show a value as no longer in use.
  return *vm.top_pointer;
}
static Value peek(int distance) {
  return vm.top_pointer[-1 - distance];
}
static void resetStack() {
  vm.top_pointer = vm.stack;
  vm.frameCount = 0;
  // vm.openUpValues = NULL;
}
void initVM() { //TODO compare
  resetStack();
  initTable(&vm.strings);
  initTable(&vm.globals);
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

  CallFrame* oFrame = &vm.frames[vm.frameCount -= 1];
  size_t instruction = oFrame->ip - oFrame->function_pointer->chunk.code_pointer - 1;
  int line = oFrame->function_pointer->chunk.lines_pointer[instruction];

  fprintf(stderr, "[line %d] in script.\n", line);
  resetStack();
}

static void concatenate() {
  StringObject* right = AS_STRING(pop());
  StringObject* left = AS_STRING(pop());
  int length = left->length + right->length;

  char* runes = ALLOCATE(char, length + 1);
  memcpy(runes, left->runes_pointer, left->length);
  memcpy(runes + left->length, right->runes_pointer, right->length);
  runes[length] = '\0';

  StringObject* result = takeString(runes, length);
  push(OBJECT_VALUE(result));

}

static InterpretResult run() {
  CallFrame* oFrame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*oFrame->ip += 1)
#define READ_SHORT() (oFrame->ip += 2, (uint16_t)((oFrame->ip[-2] << 8) | oFrame->ip[-1]))
#define READ_CONSTANT() (oFrame->function_pointer->chunk.constants.values_pointer[READ_BYTE()]) //lazily treats all numbers as doubles
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
#ifdef DEBUG_TRACE_EXECUTION //TODO compare all
    /* Stack Tracing */
    printf(" ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot += 1) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    /* end Stack Tracking */
    disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
#endif
  // start of run()
    uint8_t instruction; //TODO
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT : {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_VOID : push(VOID_VALUE);
        break;
      case OP_TRUE : push(TF_VALUE(true));
        break;
      case OP_FALSE : push(TF_VALUE(false));
        break;
      case OP_POP : pop();
        break;
      case OP_GET_LOCAL : {
        uint8_t slot = READ_BYTE();
        push(oFrame->slots_pointer[slot]);
        break;
      }
      case OP_SET_LOCAL : {
        uint8_t slot = READ_BYTE();
        oFrame->slots_pointer[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL : {
        StringObject* name = READ_STRING();
        Value value; //TODO ???
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->runes_pointer);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_SET_GLOBAL : {
        StringObject* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          deleteEntry(&vm.globals, name);
          runtimeError("Undefined variable '%s'.", name->runes_pointer);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_DEFINE_GLOBAL : {
        StringObject* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
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
      case OP_PRINT : { // hand off to debug
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_JUMP : {
        uint16_t offset = READ_SHORT();
        oFrame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE : { // TODO jump if true
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) {
          oFrame->ip += offset;
        }
        break;
      }
      case OP_LOOP : {
        uint16_t offset = READ_SHORT();
        oFrame->ip -= offset;
        break;
      }
      case OP_RETURN : { //TODO change after implementing functions
        // printValue(pop());
        // printf("\n");
        return INTERPRET_OK;
      }
    }
  }
#undef READ_CONSTANT // undefining macros might seem solely fastidious but it helps the C preprocessor
#undef READ_BYTE
#undef BINARY_OP
#undef READ_STRING
#undef READ_SHORT
}

InterpretResult interpret(const char* source) {
  FunctionObject* oFunction = compile(source);

  if (oFunction == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJECT_VALUE(oFunction));

  CallFrame* oFrame = &vm.frames[vm.frameCount += 1];
  oFrame->function_pointer = oFunction;
  oFrame->ip = oFunction->chunk.code_pointer;
  oFrame->slots_pointer = vm.stack;

  return run();
}